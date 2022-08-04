//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <deque>
#include <boost/asio.hpp>

namespace async_server
{

using boost::asio::ip::tcp;

using endpoint_t = boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>;
struct rc_data
{
    rc_data() = default;
    rc_data(const char* data, std::size_t size, endpoint_t&& epoint);

    const std::string m_data;
    const endpoint_t m_endpoint;
};


class Queue
{
public:
    void push(rc_data&& d);

    rc_data pop();

    rc_data front();

    void wait();

    bool empty();

private:
    using messages_queue_t = std::deque<rc_data>;
    messages_queue_t m_messages_queue;
    bool m_received{false};
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

class session: public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket, Queue& queue);

    void start();

private:
    void do_read();

    void do_write(std::size_t length);

    tcp::socket m_socket;
    static constexpr std::size_t data_max_length{1024};
    char m_data[data_max_length];
    Queue& m_queue;
};

class server
{
public:
    server(boost::asio::io_context& io_context, short port, Queue& queue);

private:
    void do_accept();

    tcp::acceptor m_acceptor;
    Queue& m_queue;
};

}
