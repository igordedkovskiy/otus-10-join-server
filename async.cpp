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
        return m_block_finished || m_type == BlockType::STATIC;
    }

    enum class BlockType { STATIC, DYNAMIC };
    BlockType m_type{BlockType::STATIC};
    bool m_block_finished{false};
    std::time_t m_block_time;
    size_type m_block_id{0};
    std::vector<std::string> m_block;
};

struct commands_queue_t
{
    commands_queue_t(size_type bulk_size): m_block_size{bulk_size} { m_queue.push_back(block_t{}); }

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
        //else if(cmd == std::string{0x05})
        //{
        //    if(m_queue.back().m_type == block_t::BlockType::STATIC)
        //        m_queue.back().m_block_finished = true;
        //}
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
    std::deque<block_t> m_queue;
};

struct Handlers
{
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

    std::unordered_map<handler_t, commands_queue_t> m_streams;
    handler_t m_last{std::numeric_limits<handler_t>::max()};
};

Handlers handlers;
std::mutex handlers_mutex;

std::queue<std::vector<std::string>> logq;

}

extern "C"
{

handler_t connect(size_type bulk_size)
{
    std::lock_guard<std::mutex> guard{handlers_mutex};
    return handlers.create(commands_queue_t{bulk_size});
}

int disconnect(handler_t h)
{
    std::lock_guard<std::mutex> guard{handlers_mutex};
    handlers.destroy(h);
    return 1;
}

void receive(handler_t h, commands_t commands, size_type num_of_commands)
{
    if(!h)
        return;
    if(h == std::numeric_limits<decltype(h)>::max())
        return;
    if(!num_of_commands)
        return;
    auto el{handlers.m_streams.find(h)};
    if(el == std::end(handlers.m_streams))
        return;

    std::stringstream cmd_stream;
    for(size_type cntr{0}; cntr < num_of_commands; ++cntr)
        cmd_stream << commands[cntr] << '\n';

    auto& cmd_collector{el->second};
    auto process_input = [&cmd_collector](std::string&& read)
    {
        cmd_collector.process_cmd(std::move(read));
    };

    read_input<decltype(process_input), ParseErr>(cmd_stream, std::cerr, process_input);

    auto print = [](auto& stream, auto& cmds)
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
    };

    auto& q{cmd_collector.m_queue};
    auto process_commands_queue = [h, &q, &print]()
    {
        auto get_block = [&q]()
        {
            std::lock_guard<std::mutex> guard{handlers_mutex};
            if(q.empty())
                return std::make_pair(false, block_t{});
            if(!q.front().is_finished())
                return std::make_pair(false, block_t{});
            auto block{std::move(q.front())};
            q.pop_front();
            logq.push(block.m_block);
            return std::make_pair(true, std::move(block));
        };

        const auto th_id{std::this_thread::get_id()};
        while(true)
        {
            auto [res, block] = get_block();
            if(!res)
                break;
            const auto& cmds{block.m_block};
            std::stringstream fname;
            fname << "bulk-" << h << '-' << th_id << '-'
                  << block.m_block_time << '-' << block.m_block_id << ".log";
            std::fstream file{fname.str(), std::fstream::out | std::fstream::app};
            print(file, cmds);
        }
    };

    auto process_log = [&print, &q]()
    {
        while(!q.empty())
        {
            auto& logqr = logq;
            while(!logqr.empty())
            {
                const auto cmds{std::move(logq.front())};
                logq.pop();
                print(std::cout, cmds);
            }
            using namespace std::chrono;
            std::this_thread::sleep_for(2ms);
        }
    };


    std::thread t1{process_commands_queue};
    std::thread t2{process_commands_queue};
    std::thread log{process_log};
    t1.join();
    t2.join();
    log.join();
}

}
