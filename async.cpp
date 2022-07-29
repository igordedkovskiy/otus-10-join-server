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


template<typename Q> class MThreadingContext
{
public:
    template<typename F, typename... Args> MThreadingContext(Q& q, F&& work, Args&&... args):
        m_queue{q},
        m_thread{std::thread{work, std::ref(m_mutex), std::ref(m_cv), std::ref(m_ready), args...}}
    {
        m_thread.detach();
    }

    ~MThreadingContext()
    {
        wait();
    }

    //template<typename F, typename... Args> void run(F&& work, Args&&... args)
    //{
    //    m_thread = std::thread{work, std::ref(m_mutex), std::ref(m_cv), std::ref(m_ready), args...};
    //    m_thread.detach();
    //}

    void wait()
    {
        std::unique_lock lk{m_mutex};
        while(!m_ready || !m_queue.empty())
            m_cv.wait(lk);
    }

private:
    Q& m_queue;
    bool m_ready{true};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
};

void log_work(std::mutex& m, std::condition_variable& cv, bool& ready);
void fwork(std::mutex& m, std::condition_variable& cv, bool& ready);
class MThreading
{
public:
    static MThreading& get()
    {
        static MThreading v;
        return v;
    }

    ~MThreading()
    {
//        wait();
    }

    void wait()
    {
        m_f2.wait();
        m_f1.wait();
        m_log.wait();
    }

//private:
    MThreading() = default;

    using log_queue_t = std::deque<CmdCollector::cmds_t>;
    using fqueue_t = std::deque<std::pair<std::string, CmdCollector::cmds_t>>;

    log_queue_t m_to_log_q;
    fqueue_t m_to_files_q;

    std::mutex m_logq_mutex;
    std::mutex m_filesq_mutex;

    std::size_t m_fcntr{0};

    MThreadingContext<log_queue_t> m_log{m_to_log_q, log_work};
    MThreadingContext<fqueue_t> m_f1{m_to_files_q, fwork}, m_f2{m_to_files_q, fwork};
};

//MThreading worker;

void log_work(std::mutex& m, std::condition_variable& cv, bool& ready)
{
    auto get = [](CmdCollector::cmds_t& cmds)
    {
        {
            std::scoped_lock lk{MThreading::get().m_logq_mutex};
            if(MThreading::get().m_to_log_q.empty())
                return false;
            cmds = std::move(MThreading::get().m_to_log_q.front());
            MThreading::get().m_to_log_q.pop_front();

//            std::scoped_lock lk{worker.m_logq_mutex};
//            if(worker.m_to_log_q.empty())
//                return false;
//            cmds = std::move(worker.m_to_log_q.front());
//            worker.m_to_log_q.pop_front();
        }
        return true;
    };

    auto& stream{std::cout};
    while(true)
    {
        {
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
        cv.notify_all();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }
}

void fwork(std::mutex& m, std::condition_variable& cv, bool& ready)
{
    auto get = [](std::string& fname, CmdCollector::cmds_t& cmds)
    {
        {
            std::scoped_lock lk{MThreading::get().m_filesq_mutex};
            if(MThreading::get().m_to_files_q.empty())
                return false;
            fname = std::move(MThreading::get().m_to_files_q.front().first);
            cmds = std::move(MThreading::get().m_to_files_q.front().second);
            MThreading::get().m_to_files_q.pop_front();

//            std::scoped_lock lk{worker.m_filesq_mutex};
//            if(worker.m_to_files_q.empty())
//                return false;
//            fname = std::move(worker.m_to_files_q.front().first);
//            cmds = std::move(worker.m_to_files_q.front().second);
//            worker.m_to_files_q.pop_front();
        }
        return true;
    };

    std::string fname;
    while(true)
    {
        {
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
        cv.notify_all();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }
}


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
                std::lock_guard lk{MThreading::get().m_logq_mutex};
                MThreading::get().m_to_log_q.push_back(m_commands.get_cmds());
//                std::lock_guard lk{worker.m_logq_mutex};
//                worker.m_to_log_q.push_back(m_commands.get_cmds());

//                m_log.get_ready();
            }

            std::stringstream fname;
            fname << "bulk-" << (unsigned long long)m_handler << '-' << m_commands.block_start_time(0) << '-' << ++MThreading::get().m_fcntr << ".log";
//            fname << "./bulk/bulk-" << (unsigned long long)m_handler << '-' << m_commands.block_start_time(0) << '-' << ++MThreading::get().m_fcntr << ".log";
//            fname << "./bulk/bulk-" << (unsigned long long)m_handler << '-' << m_commands.block_start_time(0) << '-' << ++worker.m_fcntr << ".log";
            {
                std::lock_guard lk{MThreading::get().m_filesq_mutex};
                MThreading::get().m_to_files_q.emplace_back(std::make_pair(fname.str(), std::move(m_commands.get_cmds())));
//                std::lock_guard lk{worker.m_filesq_mutex};
//                worker.m_to_files_q.emplace_back(std::make_pair(fname.str(), std::move(m_commands.get_cmds())));

                //                m_f1.get_ready();
//                m_f2.get_ready();
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
    MThreading::get().wait();
//    worker.wait();
}


}

}

