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
    enum class SocketStatus: std::uint8_t { UNDEFINED, OPENED, CLOSED };

    rc_data(const char* data, std::size_t size, const endpoint_t& epoint):
        m_data{data, size},
        m_endpoint{epoint.address().to_string() + std::to_string(epoint.port())}
    {}

    const std::string m_data;
    const std::string m_endpoint;
    const SocketStatus m_socket_is{SocketStatus::UNDEFINED};
};

template<typename F>
class session: public std::enable_shared_from_this<session<F>>
{
public:
    session(tcp::socket socket, F& f):
        m_socket(std::move(socket)),
        m_f{f}
    {}

    ~session()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        if(m_socket.is_open())
        {
//            std::cout << "close socket" << std::endl;
            m_socket.shutdown(boost::asio::socket_base::shutdown_both);
            m_socket.close();
        }
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        auto self(this->shared_from_this());
        auto process = [this, self](boost::system::error_code ec, std::size_t length)
        {
            if(!ec)
            {
                m_f(m_data, length, m_socket);
                do_write(length);
            }
            else
            {
                if(ec == boost::asio::error::eof)
                    std::cout << "eof!" << std::endl;
                else if(ec == boost::asio::error::connection_reset)
                    std::cout << "connection reset!" << std::endl;
                if(m_socket.is_open())
                {
                    std::cout << __PRETTY_FUNCTION__ << std::endl;
                    std::cout << "close socket" << std::endl;
                    m_socket.shutdown(boost::asio::socket_base::shutdown_both);
                    m_socket.close();
                }
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
    
    tcp::socket m_socket;
    static constexpr std::size_t data_max_length{1024};
    char m_data[data_max_length];
    F& m_f;
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
