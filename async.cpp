// homework #8: async commands processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <cstdint>

#include "CmdCollector.hpp"
#include "async.h"
#include "read_input.hpp"

#include "async_impl.hpp"

using namespace async;

namespace
{

Handlers handlers;
MThreading context;

struct Process
{
    Process(handler_t& h, CmdCollector& c):
        m_handler{h},
        m_commands{c}
    {}

    void operator()(std::string&& read)
    {
        m_commands.process_cmd(std::move(read));
        read.clear();
        if(m_commands.input_block_finished())
        {
            {
                std::lock_guard lk{context.m_logq_mutex};
                context.m_to_log_q.push_back(m_commands.get_cmds());
                context.m_log.m_ready = false;
            }

            std::stringstream fname;
            fname << "bulk-" << (unsigned long long)m_handler << '-' << m_commands.block_start_time(0)
                  << '-' << ++context.m_fcntr << ".log";
            {
                std::lock_guard lk{context.m_filesq_mutex};
                context.m_to_files_q.emplace_back(std::make_pair(fname.str(), std::move(m_commands.get_cmds())));
                context.m_f1.m_ready = false;
                context.m_f2.m_ready = false;
            }
            m_commands.clear_commands();
        }
    }

    handler_t& m_handler;
    CmdCollector& m_commands;
};

}


extern "C"
{

handler_t connect(size_type bulk_size)
{
    return handlers.create(std::make_unique<CmdCollector>(CmdCollector{bulk_size}));
}

int disconnect(handler_t h)
{
    if(!h)
        return 0;
    if(h == std::numeric_limits<decltype(h)>::max())
        return 0;
    auto el{handlers.find(h)};
    if(el == handlers.end())
        return 0;

    auto& commands{el->second};
    commands->finish_block();
    Process process{h, *commands.get()};
    process("{");

    handlers.destroy(h);

    context.wait();
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
    auto el{handlers.find(h)};
    if(el == handlers.end())
        return;

    std::stringstream cmd_stream{data};
    for(size_type cntr{0}; cntr < data_size; ++cntr)
        cmd_stream << data[cntr];

    Process process{h, *(el->second.get())};
    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);
}

void wait()
{
    context.wait();
}



}

