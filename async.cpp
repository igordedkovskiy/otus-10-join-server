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

    handler_t create(CmdCollector&& collector)
    {
        const auto h{find_free()};
        if(h < 0)
        {
            m_streams.emplace_back(std::move(collector));
            m_free.emplace_back(false);
            return m_streams.size();
        }
        else
        {
            m_streams[h] = std::move(collector);
            m_free[h] = false;
        }
        return h;
    }

    void destroy(handler_t h)
    {
        h++;
    }

    size_type size() const noexcept
    {
        return m_streams.size();
    }

    std::vector<CmdCollector> m_streams;
    std::vector<bool> m_free;
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
//    commands.finish_block();
    return h++;
}

void receive(handler_t h, commands_t commands, size_type& num_of_commands)
{
//    auto process = [&commands, &context](std::string&& read)
//    {
//        context.process_cmd(std::move(read));
//        read.clear();
//        if(context.input_block_finished())
//        {
//            for(const auto& cmd:commands.get_cmd())
//                commands.push_back(cmd);
//            commands.clear_commands();
//        }
//    };

//    read_input<decltype(process), CmdCollector::ParseErr>(std::cin, std::cerr, process);
//    context.finish_block();
//    if(context.input_block_finished())
//    {
//        for(const auto& cmd:commands.get_cmd())
//            commands.push_back(cmd);
//        commands.clear_commands();
//    }

    num_of_commands = handlers.size();
    commands_t cmds = new char*[num_of_commands];
    size_type index{0};
    for(const auto& cmd:handlers.get(h).get_cmd())
    {
        cmds[index] = new char[cmd.size() + 1];
        strcopy(cmds[index], cmd.get(h).c_str());
        ++index;
    }
}

}
