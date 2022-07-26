// homework #8: async commands processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <deque>
#include <queue>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstdint>
#include <chrono>

#include "async.hpp"
#include "read_input.hpp"

namespace
{

enum class ParseErr: std::uint8_t
{
    incorrect_format
};

struct block_t
{
    bool is_finished() const noexcept
    {
        return m_block_finished;
    }

    bool is_static() const noexcept
    {
        return m_type == BlockType::STATIC;
    }

    enum class BlockType { STATIC, DYNAMIC };
    BlockType m_type{BlockType::STATIC};
    bool m_block_finished{false};
    std::time_t m_block_time;
    size_type m_block_id{0};

    using block_of_cmds_t = std::vector<std::string>;
    block_of_cmds_t m_block;
};

struct commands_queue_t
{
    commands_queue_t(size_type bulk_size):
        m_block_size{bulk_size}
    {
        m_queue.push_back(block_t{});
    }

    void process_cmd(std::string&& cmd)
    {
        if(cmd == "{")
        {
            if(++m_braces == 1)
                m_queue.back().m_block_finished = true;
        }
        else if(cmd == "}")
        {
            if(m_braces == 0)
                throw ParseErr::incorrect_format;
            if(--m_braces == 0)
                m_queue.back().m_block_finished = true;
        }
        else
        {
            if(m_queue.back().m_block_finished)
            {
                m_queue.push_back(block_t{});
                if(m_braces > 0)
                    m_queue.back().m_type = block_t::BlockType::DYNAMIC;
            }
            auto& q = m_queue.back();
            q.m_block.emplace_back(std::move(cmd));
            if(q.m_block.size() == 1)
            {
                q.m_block_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                q.m_block_id = m_prev_block_id + 1;
                ++m_prev_block_id;
            }

            if(m_braces == 0)
            {
                if(q.m_block.size() == m_block_size)
                    q.m_block_finished = true;
            }
        }
    }

    enum class ParseErr: std::uint8_t { incorrect_format };
    size_type m_block_size{0};
    size_type m_braces{0};
    size_type m_prev_block_id{0};

    using blocks_queue_t = std::deque<block_t>;
    blocks_queue_t m_queue;
};


class Handlers
{
public:
    static Handlers& get()
    {
        static Handlers handlers;
        return handlers;
    }

    auto find(handler_t h)
    {
        return m_streams.find(h);
    }

    auto end()
    {
        return m_streams.end();
    }

    handler_t create(commands_queue_t&& collector)
    {
        m_last = !m_streams.empty() ? m_last + 1 : 1;
        m_streams.insert(std::make_pair(m_last, std::move(collector)));
        return m_last;
    }

    void destroy(handler_t h)
    {
        auto el{m_streams.find(h)};
        if(el != std::end(m_streams))
            m_streams.erase(el);
        if(h == m_last)
            --m_last;
    }

    size_type size() const noexcept
    {
        return m_streams.size();
    }

private:
    Handlers() = default;
    using streams_t = std::unordered_map<handler_t, commands_queue_t>;
    streams_t m_streams;
    handler_t m_last{std::numeric_limits<handler_t>::max()};
};


class HandlersThreadSafe
{
public:
    static HandlersThreadSafe& get()
    {
        static HandlersThreadSafe handlers;
        return handlers;
    }

    auto find(handler_t h)
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        return Handlers::get().find(h);
    }

    auto end()
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        return Handlers::get().end();
    }

    handler_t create(commands_queue_t&& collector)
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        return Handlers::get().create(std::move(collector));
    }

    void destroy(handler_t h)
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        Handlers::get().destroy(h);
    }

    size_type size()
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        return Handlers::get().size();
    }

private:
    HandlersThreadSafe() = default;
    std::mutex m_mutex;
};


class LogQueue
{
public:
    static LogQueue& get()
    {
        static LogQueue logger;
        return logger;
    }

    void push(block_t::block_of_cmds_t block)
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        m_log.push(block);
    }
    void pop()
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        m_log.pop();
    }

    bool empty()
    {
//        std::lock_guard<std::mutex> guard{m_mutex};
        return m_log.empty();
    }

    auto front()
    {
        std::lock_guard<std::mutex> guard{m_mutex};
        return m_log.front();
    }

    void lock()
    {
        m_mutex.lock();
    }
    void unlock()
    {
        m_mutex.unlock();
    }

private:
    LogQueue() = default;
    using log_queue_t = std::queue<std::vector<std::string>>;
    log_queue_t m_log;
    std::mutex m_mutex;
};


void print(auto& stream, const auto& cmds)
{
    stream << "bulk: ";
    std::size_t cntr = 0;
    for(const auto& cmd:cmds)
    {
        stream << cmd;
        if(++cntr < cmds.size())
            stream << ", ";
    }
    stream << '\n';
    stream.flush();
}

auto get_block(auto& q)
{
    if(q.empty())
        return std::make_pair(false, block_t{});
    if(!q.front().is_finished())
        return std::make_pair(false, block_t{});
    auto block{std::move(q.front())};
    q.pop_front();
//    LogQueue::get().push(block.m_block);
    return std::make_pair(true, std::move(block));
}

auto get_block_on_disconnect(auto& q)
{
    if(q.empty())
        return std::make_pair(false, block_t{});
    if(!q.front().is_static())
        return std::make_pair(false, block_t{});
    auto block{std::move(q.front())};
    q.pop_front();
//    LogQueue::get().push(block.m_block);
    return std::make_pair(true, std::move(block));
}

std::pair<bool, block_t> (*get_block_f)(commands_queue_t::blocks_queue_t&);
auto process_queue(handler_t h, commands_queue_t& cmd_collector, decltype(get_block_f) get_block)
{
    auto& q{cmd_collector.m_queue};
    auto process_commands_queue = [h, &q, &get_block]()
    {
        const auto th_id{std::this_thread::get_id()};
        while(true)
        {
            auto [res, block] = get_block(q);
            if(!res)
                break;
            const auto& cmds{block.m_block};
            std::stringstream fname;
            fname << "./bulk/bulk-" << h << '-' << th_id << '-'
                  << block.m_block_time << '-' << block.m_block_id << ".log";
            std::fstream file{fname.str(), std::fstream::out | std::fstream::app};
            print(file, cmds);
//            LogQueue::get().push(std::move(cmds));
        }
    };

//    auto process_log_queue = [h]()
//    {
//        while(!LogQueue::get().empty())
//        {
//            std::cout << "bulk: ";
//            for(const auto& cmd:LogQueue::get().front())
//                std::cout << cmd << ", ";
//            std::cout << '\n';
//            LogQueue::get().pop();
//        }
//    };

    std::thread t1{process_commands_queue};
    std::thread t2{process_commands_queue};
//    std::thread log{process_log_queue};
    t1.join();
    t2.join();
//    log.join();
}

}

extern "C"
{

handler_t connect(size_type bulk_size)
{
    return Handlers::get().create(commands_queue_t{bulk_size});
}

int disconnect(handler_t h)
{
    if(!h)
        return 0;
    if(h == std::numeric_limits<decltype(h)>::max())
        return 0;
    auto el{Handlers::get().find(h)};
    if(el == Handlers::get().end())
        return 0;

    process_queue(h, el->second, get_block_on_disconnect);

    // auto t = find_thread(h)
    // wait for t to finish

    Handlers::get().destroy(h);
    return 1;
}

void receive(handler_t h, const char* data, size_type data_size)
{
    if(!h)
        return;
    if(h == std::numeric_limits<decltype(h)>::max())
        return;
    if(!data_size)
        return;
    auto el{Handlers::get().find(h)};
    if(el == Handlers::get().end())
        return;

    std::stringstream cmd_stream{data};
    for(size_type cntr{0}; cntr < data_size; ++cntr)
        cmd_stream << data[cntr];

    auto& cmd_collector{el->second};
    auto process_input = [&cmd_collector](std::string&& read)
    {
        cmd_collector.process_cmd(std::move(read));
    };

    read_input<decltype(process_input), ParseErr>(cmd_stream, std::cerr, process_input);
    process_queue(h, cmd_collector, get_block);
}


handler_t connect_ths(size_type bulk_size)
{
    return HandlersThreadSafe::get().create(commands_queue_t{bulk_size});
}

int disconnect_ths(handler_t h)
{
    if(!h)
        return 0;
    if(h == std::numeric_limits<decltype(h)>::max())
        return 0;
    auto el{HandlersThreadSafe::get().find(h)};
    if(el == HandlersThreadSafe::get().end())
        return 0;

    process_queue(h, el->second, get_block_on_disconnect);

    HandlersThreadSafe::get().destroy(h);
    return 1;
}

void receive_ths(handler_t h, const char* data, size_type data_size)
{
    if(!h)
        return;
    if(h == std::numeric_limits<decltype(h)>::max())
        return;
    if(!data_size)
        return;
    auto el{HandlersThreadSafe::get().find(h)};
    if(el == HandlersThreadSafe::get().end())
        return;

    std::stringstream cmd_stream{data};
    for(size_type cntr{0}; cntr < data_size; ++cntr)
        cmd_stream << data[cntr];

    auto& cmd_collector{el->second};
    auto process_input = [&cmd_collector](std::string&& read)
    {
        cmd_collector.process_cmd(std::move(read));
    };

    read_input<decltype(process_input), ParseErr>(cmd_stream, std::cerr, process_input);
    process_queue(h, cmd_collector, get_block);
}

}

