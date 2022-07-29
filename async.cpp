// homework #8: async commands processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <cstdint>
#include <type_traits>

#include "CmdCollector.hpp"
#include "async.h"
#include "read_input.hpp"

namespace
{

struct TestLog
{
    static TestLog& get()
    {
        static TestLog t;
        return t;
    }
    std::fstream& log()
    {
        return m_file;
    }
    ~TestLog()
    {
        if(m_file.is_open())
            m_file.close();
    }
    std::fstream m_file;//{std::fstream{"./bulk/01.log", std::fstream::out | std::fstream::app}};
};

using size_type = async::size_type;
using handler_t = async::handler_t;

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

    handler_t create(std::unique_ptr<CmdCollector> collector)
    {
        const auto ptr{collector.get()};
        m_handlers.insert(std::make_pair(ptr, std::move(collector)));
        return ptr;
    }

    void destroy(handler_t h)
    {
        auto el{m_handlers.find(h)};
        if(el != std::end(m_handlers))
            m_handlers.erase(el);
    }

    size_type size() const noexcept
    {
        return m_handlers.size();
    }

private:
    Handlers() = default;

    using handlers_t = std::unordered_map<handler_t, std::unique_ptr<CmdCollector>>;
    handlers_t m_handlers;
};

void log_work(std::mutex& m, std::condition_variable& cv, bool& ready);
void fwork(std::mutex& m, std::condition_variable& cv, bool& ready);
struct Process
{
    Process(handler_t& h, CmdCollector& c):
        m_handler{h},
        m_commands{c}
    {}

    static void run()
    {
        m_log.run(log_work);
        m_f1.run(fwork);
        m_f2.run(fwork);
    }

    static void wait()
    {
        m_f1.wait();
        m_f2.wait();
        m_log.wait();
    }

    void operator()(std::string&& read)
    {
        TestLog::get().log() << "enter ";
        TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
        m_commands.process_cmd(std::move(read));
        read.clear();
        if(m_commands.input_block_finished())
        {
            {
                std::lock_guard lk{m_logq_mutex};
                m_to_log_q.push_back(m_commands.get_cmds());
//                m_log.get_ready();
            }

            std::stringstream fname;
            fname << "./bulk/bulk-" << (unsigned long long)m_handler << '-' << m_commands.block_start_time(0) << '-' << ++m_fcntr << ".log";
            {
                std::lock_guard lk{m_filesq_mutex};
                m_to_files_q.emplace_back(std::make_pair(fname.str(), std::move(m_commands.get_cmds())));
//                m_f1.get_ready();
//                m_f2.get_ready();
            }
            m_commands.clear_commands();
        }
        TestLog::get().log() << "leave ";
        TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
    }



    handler_t& m_handler;
    CmdCollector& m_commands;


    class MThreadingContext
    {
    public:
        ~MThreadingContext()
        {
            wait();
        }

        template<typename F, typename... Args> void run(F&& work, Args&&... args)
        {
            TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
            m_thread = std::thread{work, std::ref(m_mutex), std::ref(m_cv), std::ref(m_ready), args...};
            m_thread.detach();
        }

        void wait()
        {
            TestLog::get().log() << "enter ";
            TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
            std::unique_lock lk{m_mutex};
            while(!m_ready)
            //while(!m_ready || !Process::m_to_files_q.empty() || !Process::m_to_log_q.empty())
                m_cv.wait(lk);
            TestLog::get().log() << "leave ";
            TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
        }

        void get_ready()
        {
            std::scoped_lock lk{m_mutex};
            m_ready = false;
        }

    private:
        bool m_ready{true};
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::thread m_thread;
    };

    using log_queue_t = std::deque<CmdCollector::cmds_t>;
    using fqueue_t = std::deque<std::pair<std::string, CmdCollector::cmds_t>>;
    static log_queue_t m_to_log_q;
    static fqueue_t m_to_files_q;
    static std::mutex m_logq_mutex;
    static std::mutex m_filesq_mutex;

    static std::size_t m_fcntr;
    static MThreadingContext m_f1;
    static MThreadingContext m_f2;
    static MThreadingContext m_log;
};

Process::log_queue_t Process::m_to_log_q;
Process::fqueue_t Process::m_to_files_q;
std::mutex Process::m_logq_mutex;
std::mutex Process::m_filesq_mutex;

Process::MThreadingContext Process::m_f1{};
Process::MThreadingContext Process::m_f2{};
Process::MThreadingContext Process::m_log{};
std::size_t Process::m_fcntr = 0;

void log_work(std::mutex& m, std::condition_variable& cv, bool& ready)
{
    auto get = [](CmdCollector::cmds_t& cmds)
    {
//        try
        {
            std::scoped_lock lk{Process::m_logq_mutex};
            if(Process::m_to_log_q.empty())
            {
                //TestLog::get().log() << "log queue is empty\n";
                return false;
            }
            cmds = std::move(Process::m_to_log_q.front());
            Process::m_to_log_q.pop_front();
        }
//        catch(std::exception& e)
//        {
//            TestLog::get().log() << "Exception: " << e.what() << '\n';
//            return false;
//        }
        return true;
    };

    auto& stream{std::cout};
    while(true)
    {
        {
            TestLog::get().log() << "thread id = " << std::this_thread::get_id() << " ";
            TestLog::get().log() << "new cycle ";
            TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
            std::scoped_lock lk{m};
            ready = false;
            CmdCollector::cmds_t cmds;
            while(get(cmds))
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
            }
            ready = true;
        }
        TestLog::get().log() << "thread id = " << std::this_thread::get_id() << " ";
        TestLog::get().log() << "leave cycle ";
        TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
        cv.notify_one();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }
}

void fwork(std::mutex& m, std::condition_variable& cv, bool& ready)
{
    auto get = [](std::string& fname, CmdCollector::cmds_t& cmds)
    {
//        try
        {
            std::scoped_lock lk{Process::m_filesq_mutex};
            if(Process::m_to_files_q.empty())
            {
                //TestLog::get().log() << "fqueue is empty\n";
                return false;
            }
            fname = std::move(Process::m_to_files_q.front().first);
            cmds = std::move(Process::m_to_files_q.front().second);
            Process::m_to_files_q.pop_front();
        }
//        catch(std::exception& e)
//        {
//            TestLog::get().log() << "Exception: " << e.what() << '\n';
//            return false;
//        }
        return true;
    };

    std::string fname;
    while(true)
    {
        {
            TestLog::get().log() << "thread id = " << std::this_thread::get_id() << " ";
            TestLog::get().log() << "new cycle ";
            TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
            std::scoped_lock lk{m};
            ready = false;
            CmdCollector::cmds_t cmds;
            while(get(fname, cmds))
            {
                auto stream{std::fstream{fname, std::fstream::out | std::fstream::app}};
                stream << "bulk: ";
                std::size_t cntr = 0;
                for(const auto& cmd:cmds)
                {
                    stream << cmd;
                    if(++cntr < cmds.size())
                        stream << ", ";
                }
                stream << '\n';
                stream.close();
            }
            ready = true;
        }
        TestLog::get().log() << "thread id = " << std::this_thread::get_id() << " ";
        TestLog::get().log() << "leave cycle ";
        TestLog::get().log() << __PRETTY_FUNCTION__ << std::endl;
        cv.notify_one();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }
}

}



extern "C"
{

namespace async
{

handler_t connect(size_type bulk_size)
{
    return Handlers::get().create(std::make_unique<CmdCollector>(CmdCollector{bulk_size}));
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
    commands->finish_block();
    Process process{h, *commands.get()};
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

    Process process{h, *(el->second.get())};
    read_input<decltype(process), CmdCollector::ParseErr>(cmd_stream, std::cerr, process);
}

void wait()
{
    auto wait_for_queue = [](const auto& q, std::mutex& m)
    {
        while(true)
        {
            std::scoped_lock lk{m};
            if(q.empty())
                return;
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
    };
    wait_for_queue(Process::m_to_log_q, Process::m_logq_mutex);
    wait_for_queue(Process::m_to_files_q, Process::m_filesq_mutex);
    Process::wait();
}

void run()
{
    Process::run();
}

}

}

