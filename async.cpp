// homework #6: command processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <queue>
#include <unordered_map>
#include <thread>
#include <cstdint>
#include <chrono>

#include "async.hpp"
#include "CmdCollector.hpp"
#include "read_input.hpp"

namespace
{

struct block_t
{
    enum class InputType { STATIC, DYNAMIC };
    InputType m_type{InputType::STATIC};
    std::time_t m_block_time;
    size_type m_block_id{0};
    std::vector<std::string> m_block;
};

struct commands_queue_t
{
    void process_cmd(std::string &&cmd)
    {
        if(cmd == "{")
        {
            m_queue.back().m_type = block_t::InputType::DYNAMIC;
            if(++m_braces == 1)
            {
                if(m_cmds.size() > 0)
                    m_block_finished = true;
            }
        }
        else if(cmd == "}")
        {
            if(m_braces == 0)
                throw ParseErr::incorrect_format;
            if(--m_braces == 0)
            {
                m_queue.back().m_type = block_t::InputType::STATIC;
                m_block_finished = true;
            }
        }
        else
        {
            m_cmds.emplace_back(std::move(cmd));
            if(m_cmds.size() == 1)
            {
                m_queue.back().m_block_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                ++m_queue.back().m_block_id;
            }
            if(m_braces == 0)
            {
                if(m_cmds.size() == m_capacity)
                    m_block_finished = true;
            }
        }
    }

    enum class ParseErr: std::uint8_t { incorrect_format };
    std::size_t m_braces{0};
    std::queue<block_t> m_queue;
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

}

extern "C"
{

handler_t connect(size_type bulk_size)
{
    return handlers.create(CmdCollector{bulk_size});
}

int disconnect(handler_t h)
{
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
    auto process = [&cmd_collector](std::string&& read)
    {
        cmd_collector.process_cmd(std::move(read));
        read.clear();
    };

    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);

    auto process_cmd_queue = [](commands_queue_t& q)
    {
        ;
    };

    std::thread t1{process_cmd_queue};
    std::thread t2{process_cmd_queue};
    //for(auto& q:cmd_collector.get_cmd())
    //{
    //    for(auto& cmd:q)
    //    {
    //        // print
    //        // write to file
    //    }
    //}
    t1.detach();
    t2.detach();
}

}
