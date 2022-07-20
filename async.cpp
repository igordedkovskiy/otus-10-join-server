// homework #6: command processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>

#include "async.hpp"
#include "CmdCollector.hpp"
#include "read_input.hpp"

namespace
{

struct Handlers
{
    handler_t find_free() const
    {
        handler_t cntr{0};
        for(auto f:m_free)
        {
            if(f)
                return cntr;
            ++cntr;
        }
        return -1;
    }

    handler_t add(CmdCollector&& collector)
    {
        const auto h{find_free()};
        if(h < 0)
        {
            m_streams.emplace_back(std::move(collector));
            m_free.emplace_back(false);
            return static_cast<decltype(handler_t)>(m_streams.size());
        }
        else
        {
            m_streams[h] = std::move(collector);
            m_free[h] = false;
        }
        return h;
    }

    std::vector<handler_t, CmdCollector> m_streams;
    std::vector<bool> m_free;
};

Handlers handlers;

}


handler_t connect(std::size_t in_size)
{
    return handlers.add(CmdCollector{in_size});
}

void disconnect(handler_t commands)
{
    commands.finish_block();
}

void receive(handler_t h, commands_t& commands)
{
    auto process = [&commands, &context](std::string&& read)
    {
        context.process_cmd(std::move(read));
        read.clear();
        if(context.input_block_finished())
        {
            for(const auto& cmd:commands.get_cmd())
                commands.push_back(cmd);
            commands.clear_commands();
        }
    };

    read_input<decltype(process), CmdCollector::ParseErr>(std::cin, std::cerr, process);
    context.finish_block();
    if(context.input_block_finished())
    {
        for(const auto& cmd:commands.get_cmd())
            commands.push_back(cmd);
        commands.clear_commands();
    }
}
