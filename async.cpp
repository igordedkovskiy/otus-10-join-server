// homework #8: async commands processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <deque>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdint>

#include "CmdCollector.hpp"
#include "async.hpp"
#include "read_input.hpp"

namespace
{

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
        return m_handlers.find(h);
    }

    auto end()
    {
        return m_handlers.end();
    }

    handler_t create(CmdCollector&& collector)
    {
        m_last = !m_handlers.empty() ? m_last + 1 : 1;
        m_handlers.insert(std::make_pair(m_last, collector));
        return m_last;
    }

    void destroy(handler_t h)
    {
        auto el{m_handlers.find(h)};
        if(el != std::end(m_handlers))
            m_handlers.erase(el);
        if(h == m_last)
            --m_last;
    }

    size_type size() const noexcept
    {
        return m_handlers.size();
    }

private:
    Handlers() = default;

    using handlers_t = std::unordered_map<handler_t, CmdCollector>;
    handlers_t m_handlers;
    handler_t m_last{std::numeric_limits<handler_t>::max()};
};


class MThreadingContext
{
public:
    void run(void(*print)(std::ostream&, CmdCollector::cmds_t, std::mutex&, std::condition_variable&, std::atomic<bool>&),
             std::ostream& stream, CmdCollector::cmds_t cmds)
    {
        m_thread = std::thread{print, std::ref(stream), std::move(cmds), std::ref(m_mutex), std::ref(m_cv), std::ref(m_done)};
        m_done = false;
        m_thread.detach();
    }

    void wait()
    {
        if(!m_done)
        {
            std::unique_lock lk(m_mutex);
            m_cv.wait(lk);
        }
    }

    bool ready() const noexcept
    {
        return m_done;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_done{true};
    std::thread m_thread;
    std::fstream file;
};


struct Process
{
    Process(handler_t& h, CmdCollector& c):
        m_handler{h},
        m_commands{c}
    {}

    static void wait()
    {
        m_f1.wait();
        m_file1.close();
        m_f2.wait();
        m_file2.close();
        m_log.wait();
    }

    void operator()(std::string&& read)
    {
        auto print = [](std::ostream& stream, CmdCollector::cmds_t cmds,
                std::mutex& m, std::condition_variable& cv, std::atomic<bool>& done)
        {
            {
                std::unique_lock lk{m};
                stream << "bulk: ";
                std::size_t cntr = 0;
                for(const auto& cmd:cmds)
                {
                    stream << cmd;
                    if(++cntr < cmds.size())
                        stream << ", ";
                }
                stream << '\n';
                done = true;
            }
            cv.notify_all();
        };

        m_commands.process_cmd(std::move(read));
        read.clear();
        if(m_commands.input_block_finished())
        {
            const auto cmds{std::move(m_commands.get_cmds())};

            m_log.wait();
            m_log.run(print, std::cout, cmds);

            std::stringstream fname;
            fname << "bulk-" << m_handler << '-' << m_commands.block_start_time(0) << '-' << ++m_fcntr << ".log";

            if(m_f1.ready())
            {
                m_file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                m_f1.run(print, m_file1, std::move(cmds));
            }
            else if(m_f2.ready())
            {
                m_file2 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                m_f2.run(print, m_file2, std::move(cmds));
            }
            else
            {
                m_f1.wait();
                m_file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                m_f1.run(print, m_file1, std::move(cmds));
            }

            m_commands.clear_commands();
        }
    }

    handler_t& m_handler;
    CmdCollector& m_commands;

    static MThreadingContext m_f1, m_f2, m_log;
    static std::fstream m_file1, m_file2;
    static std::size_t m_fcntr;
};

MThreadingContext Process::m_f1{};
MThreadingContext Process::m_f2{};
MThreadingContext Process::m_log{};
std::fstream Process::m_file1{};
std::fstream Process::m_file2{};
std::size_t Process::m_fcntr = 0;

}

extern "C"
{

handler_t connect(size_type bulk_size)
{
    return Handlers::get().create(CmdCollector{bulk_size});
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

    auto& commands{el->second};
    commands.finish_block();
    Process process{h, commands};
    process("{");

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

    Process process{h, el->second};
    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);
}

void wait()
{
    Process::wait();
}

}
