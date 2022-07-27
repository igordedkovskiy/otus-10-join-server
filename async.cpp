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


class SyncContext
{
public:
    void run(void(*print)(std::ostream&, CmdCollector::cmds_t, std::mutex&, std::condition_variable&, std::atomic<bool>&), std::ostream& stream, CmdCollector::cmds_t cmds)
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
};

SyncContext f1, f2, log;
std::fstream file1, file2;
std::size_t fcntr{0};

struct Process
{
    Process(CmdCollector& c): commands{c} {}

    void operator()(std::string&& read)
    {
        auto print = [](std::ostream& stream, CmdCollector::cmds_t cmds,
                std::mutex& m, std::condition_variable& cv, std::atomic<bool>& done)
        {
            {
                std::unique_lock lk{m};
                //std::cout << '\n';
                //std::cout << __PRETTY_FUNCTION__ << '\n';
                //std::cout << std::this_thread::get_id() << std::endl;
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

        commands.process_cmd(std::move(read));
        read.clear();
        if(commands.input_block_finished())
        {
            const auto cmds{std::move(commands.get_cmds())};

            log.wait();
            log.run(print, std::cout, cmds);

            std::stringstream fname;
            fname << "bulk-" << commands.block_start_time(0) << '-' << ++fcntr << ".log";

            if(f1.ready())
            {
                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                f1.run(print, file1, std::move(cmds));
            }
            else if(f2.ready())
            {
                file2 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                f2.run(print, file2, std::move(cmds));
            }
            else
            {
                f1.wait();
                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
                f1.run(print, file1, std::move(cmds));
            }

            commands.clear_commands();
        }
    }

    CmdCollector& commands;
};

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

//    auto print = [](std::ostream& stream, CmdCollector::cmds_t cmds,
//            std::mutex& m, std::condition_variable& cv, std::atomic<bool>& done)
//    {
//        {
//            std::unique_lock lk{m};
//            //std::cout << '\n';
//            //std::cout << __PRETTY_FUNCTION__ << '\n';
//            //std::cout << std::this_thread::get_id() << std::endl;
//            stream << "bulk: ";
//            std::size_t cntr = 0;
//            for(const auto& cmd:cmds)
//            {
//                stream << cmd;
//                if(++cntr < cmds.size())
//                    stream << ", ";
//            }
//            stream << '\n';
//            done = true;
//        }
//        cv.notify_all();
//    };

    auto& commands{el->second};
//    auto process = [&commands, &print](std::string&& read)
//    {
//        commands.process_cmd(std::move(read));
//        read.clear();
//        if(commands.input_block_finished())
//        {
//            const auto cmds{std::move(commands.get_cmds())};

//            log.wait();
//            log.run(print, std::cout, cmds);

//            std::stringstream fname;
//            fname << "bulk-" << commands.block_start_time(0) << '-' << ++fcntr << ".log";

//            if(f1.ready())
//            {
//                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f1.run(print, file1, std::move(cmds));
//            }
//            else if(f2.ready())
//            {
//                file2 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f2.run(print, file2, std::move(cmds));
//            }
//            else
//            {
//                f1.wait();
//                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f1.run(print, file1, std::move(cmds));
//            }

//            commands.clear_commands();
//        }
//    };
    commands.finish_block();
    Process process{commands};
    process("{");

    f1.wait();
    f2.wait();
    log.wait();

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

//    auto print = [](std::ostream& stream, CmdCollector::cmds_t cmds,
//            std::mutex& m, std::condition_variable& cv, std::atomic<bool>& done)
//    {
//        {
//            std::unique_lock lk{m};
//            //std::cout << '\n';
//            //std::cout << __PRETTY_FUNCTION__ << '\n';
//            //std::cout << std::this_thread::get_id() << std::endl;
//            stream << "bulk: ";
//            std::size_t cntr = 0;
//            for(const auto& cmd:cmds)
//            {
//                stream << cmd;
//                if(++cntr < cmds.size())
//                    stream << ", ";
//            }
//            stream << '\n';
//            done = true;
//        }
//        cv.notify_all();
//    };

    auto& commands{el->second};
//    auto process = [&commands, &print](std::string&& read)
//    {
//        commands.process_cmd(std::move(read));
//        read.clear();
//        if(commands.input_block_finished())
//        {
//            const auto cmds{std::move(commands.get_cmds())};

//            log.wait();
//            log.run(print, std::cout, cmds);

//            std::stringstream fname;
//            fname << "bulk-" << commands.block_start_time(0) << '-' << ++fcntr << ".log";

//            if(f1.ready())
//            {
//                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f1.run(print, file1, std::move(cmds));
//            }
//            else if(f2.ready())
//            {
//                file2 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f2.run(print, file2, std::move(cmds));
//            }
//            else
//            {
//                f1.wait();
//                file1 = std::fstream{fname.str(), std::fstream::out | std::fstream::app};
//                f1.run(print, file1, std::move(cmds));
//            }

//            commands.clear_commands();
//        }
//    };

    Process process{commands};
    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);
}

}

