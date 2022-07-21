// homework #6: command processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <unordered_map>

#include "async.hpp"
#include "CmdCollector.hpp"
#include "read_input.hpp"

namespace
{

struct Handlers
{
    handler_t create(CmdCollector&& collector)
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

    std::unordered_map<handler_t, CmdCollector> m_streams;
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
    auto print = [h, &cmd_collector]()
    {
        std::stringstream fname;
        fname << "bulk" << cmd_collector.block_start_time(0) << '-' << h << ".log";
        std::fstream file{fname.str(), std::fstream::out | std::fstream::app};
        file << "bulk: ";
        std::cout << "bulk: ";

        std::size_t cntr = 0;
        for(const auto& cmd:cmd_collector.get_cmd())
        {
            file << cmd;
            std::cout << cmd;
            if(++cntr < cmd_collector.block_size())
            {
                file << ", ";
                std::cout << ", ";
            }

        }
        std::cout << '\n';
        file << '\n';
        cmd_collector.clear_commands();
    };

    auto process = [&cmd_collector, &print](std::string&& read)
    {
        cmd_collector.process_cmd(std::move(read));
        read.clear();
        if(cmd_collector.input_block_finished())
            print();
    };

    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);
    cmd_collector.finish_block();
    if(cmd_collector.input_block_finished())
        print();
}

}
