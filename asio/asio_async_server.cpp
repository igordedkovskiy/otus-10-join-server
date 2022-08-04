//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <deque>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

template<typename T> struct TD;

using endpoint_t = boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>;
struct rc_data
{
    rc_data(const char* data, std::size_t size, endpoint_t&& epoint):
        m_data{data, size},
        m_endpoint{std::forward<decltype(epoint)>(epoint)}
    {}

    const std::string m_data;
    const endpoint_t m_endpoint;
};

template<typename F>
class session: public std::enable_shared_from_this<session<F>>
{
public:
    session(tcp::socket socket, F& f):
        m_f{f},
        m_socket(std::move(socket))
    {}

    void start()
    {
        do_read();
    }

private:
    F& m_f;
    void do_read()
    {
        auto self(this->shared_from_this());
//        auto enqueue = [](char* data, std::size_t length, tcp::socket& socket,
//                std::mutex& m, std::condition_variable& cv, bool& received)
//        {
//            {
//                std::scoped_lock lk{m};
//                messages_queue.push_back({data, length, socket.remote_endpoint()});
//                received = true;
//            }
//            cv.notify_one();
//        };
        auto process = [this, self](boost::system::error_code ec, std::size_t length)
        {
            if(!ec)
            {
                m_f(m_data, length, m_socket);
                //{
                //    std::scoped_lock lk{cv_m};
                //    messages_queue.push_back({m_data, length, m_socket.remote_endpoint()});
                //    received = true;
                //}
                //cv.notify_one();

                //const auto& el{messages_queue.front()};
                //std::cout << "endpoint info [ip:port]: " << el.m_endpoint << std::endl;
                //std::cout << "receive1 " << el.m_data.size() << "=" << el.m_data << std::endl;
                //std::cout << "receive2 " << length << "=" << std::string{m_data, length} << std::endl;
                //std::cout << "receive3 " << length << "=" << std::string{m_data, length} << std::endl;
                do_write(length);
            }
        };
        m_socket.async_read_some(boost::asio::buffer(m_data, data_max_length), process);
    }
    
    void do_write(std::size_t length)
    {
        auto self(this->shared_from_this());
        auto process = [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
            if(!ec)
                do_read();
        };
        boost::asio::async_write(m_socket, boost::asio::buffer(m_data, length), process);
    }
    
//with coroutines
//    template <typename SyncReadStream, typename DynamicBuffer>
//    auto async_read_some(SyncReadStream &s, DynamicBuffer &&buffers) {
//        struct Awaiter {
//            SyncReadStream &s;
//            DynamicBuffer buffers;
//
//            std::error_code ec;
//            size_t sz;
//
//            bool await_ready() { return false; }
//            void await_suspend(std::experimental::coroutine_handle<> coro) {
//                s.async_read_some(std::move(buffers),
//                                  [this, coro](auto ec, auto sz) mutable {
//                                      this->ec = ec;
//                                      this->sz = sz;
//                                      coro.resume();
//                                  });
//            }
//            auto await_resume() { return std::make_pair(ec, sz); }
//        };
//        return Awaiter{s, std::forward<DynamicBuffer>(buffers)};
//    }
//
//    void do_read() {
//        auto self(shared_from_this());
//        const auto[ec, length] = co_await async_read_some(
//            socket_, boost::asio::buffer(data_, max_length));
//
//        if (!ec) {
//            do_write(length);
//        }
//    }
//
//    //    template <typename SyncReadStream, typename DynamicBuffer>
//    auto async_write_some(SyncReadStream &s, DynamicBuffer &&buffers) {
//        struct Awaiter {
//            SyncReadStream &s;
//            DynamicBuffer buffers;
//
//            std::error_code ec;
//            size_t sz;
//
//            bool await_ready() { return false; }
//            auto await_resume() { return std::make_pair(ec, sz); }
//            void await_suspend(std::experimental::coroutine_handle<> coro) {
//                boost::asio::async_write(
//                    s, std::move(buffers), [this, coro](auto ec, auto sz) mutable {
//                        this->ec = ec;
//                        this->sz = sz;
//                        coro.resume();
//                    });
//            }
//        };
//        return Awaiter{s, std::forward<DynamicBuffer>(buffers)};
//    }
//
//    void do_write() {
//        auto self(shared_from_this());
//        const auto[ec, length] = co_await async_write_some(
//            socket_, boost::asio::buffer(data_, max_length));
//
//        if (!ec) {
//            do_read(length);
//        }
//    }
//
//        auto do_write(std::size_t length) {
//            auto self(shared_from_this());
//            struct Awaiter {
//                std::shared_ptr<session> ssn;
//                std::size_t length;
//                std::error_code ec;
//
//                bool await_ready() { return false; }
//                auto await_resume() { return ec; }
//                void await_suspend(std::experimental::coroutine_handle<> coro) {
//                    const auto[ec, sz] = co_await async_write(
//                        ssn->socket_, boost::asio::buffer(ssn->data_, length));
//                    this->ec = ec;
//                    coro.resume();
//                }
//            };
//            return Awaiter{self, length};
//        }
//
//    void do_read() {
//        auto self(shared_from_this());
//        while (true) {
//            const auto[ec, sz] = co_await async_read_some(
//                socket_, boost::asio::buffer(data_, max_length));
//            if (!ec) {
//                auto ec = co_await do_write(sz);
//                if (ec) {
//                    std::cout << "Error writing to socket: " << ec << std::endl;
//                    break;
//                }
//            } else {
//                std::cout << "Error reading from socket: " << ec << std::endl;
//                break;
//            }
//        }
//    }

    tcp::socket m_socket;
    static constexpr std::size_t data_max_length{1024};
    char m_data[data_max_length];
};

template<typename F>
class server
{
public:
    server(boost::asio::io_context& io_context, short port, F& f):
        m_f{f},
        m_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }

private:
    void do_accept()
    {
        auto process = [this](boost::system::error_code ec, tcp::socket socket)
        {
            if(!ec)
                std::make_shared<session<F>>(std::move(socket), m_f)->start();
            do_accept();
        };
        m_acceptor.async_accept(process);
    }

    F& m_f;
    tcp::acceptor m_acceptor;
};


int main(int argc, char* argv[])
{
    try
    {
        if(argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        using messages_queue_t = std::deque<rc_data>;
        messages_queue_t messages_queue;
        bool received{false};
        std::condition_variable cv;
        std::mutex cv_m;

        auto enqueue = [&messages_queue, &received, &cv, &cv_m](char* data, std::size_t length, tcp::socket& socket)
        {
            {
                std::scoped_lock lk{cv_m};
                messages_queue.push_back({data, length, socket.remote_endpoint()});
                received = true;
            }
            cv.notify_one();
        };

        auto server_main = [&enqueue](char* argv[])
        {
            boost::asio::io_context io_context;
            server server(io_context, std::atoi(argv[1]), enqueue);
            io_context.run();
        };

        std::thread t{server_main, argv};
        t.detach();

        while(true)
        {
            while(!received)
            {
                std::unique_lock lk{cv_m};
                //while(!received)
                cv.wait(lk, [&received]{ return received; });
            }
            const auto& el{messages_queue.front()};
            std::cout << "\nendpoint: " << el.m_endpoint << std::endl;
            std::cout << "data received: " << el.m_data << std::endl;

            std::scoped_lock lk{cv_m};
            received = false;
            messages_queue.pop_front();
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
    }
    return 0;
}
