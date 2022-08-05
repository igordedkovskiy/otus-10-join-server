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
#include <type_traits>
#include <boost/asio.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include "async.h"
#include "preprocessor.hpp"

namespace async_server
{

using boost::asio::ip::tcp;

using size_type = async::size_type;

using endpoint_t = boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>;
struct rc_data
{
    rc_data() = default;
    rc_data(const char* data, std::size_t size, const endpoint_t& epoint);

    const std::string m_data;
    const std::string m_endpoint;
};

struct rc_status
{
    enum class SocketStatus: std::uint8_t { UNDEFINED, OPENED, CLOSED };

    rc_status() = default;
    rc_status(const endpoint_t& epoint, SocketStatus st);

    const std::string m_endpoint;
    const SocketStatus m_socket_is{SocketStatus::UNDEFINED};
};

template<typename T>
class Queue
{
public:
    void push(T&& d)
    {
        {
            std::scoped_lock lk{m_mutex};
            m_queue.push_back(std::forward<decltype(d)>(d));
            m_received = true;
        }
        m_cv.notify_one();
    }

    T pop()
    {
        std::scoped_lock lk{m_mutex};
        if(m_queue.empty())
            return T{};
        T data{m_queue.front()};
        m_queue.pop_front();
        return data;
    }

    T front()
    {
        std::scoped_lock lk{m_mutex};
        if(m_queue.empty())
            return T{};
        T data{m_queue.front()};
        return data;
    }

    void wait()
    {
        while(!m_received)
        {
            std::unique_lock lk{m_mutex};
            m_cv.wait(lk, [this]{ return m_received; });
        }
        m_received = false;
    }

    bool empty()
    {
        std::unique_lock lk{m_mutex};
        return m_queue.empty();
    }

private:
    using messages_queue_t = std::deque<T>;
    messages_queue_t m_queue;
    bool m_received{false};
    std::condition_variable m_cv;
    std::mutex m_mutex;
};


class Retransmittor
{
public:
    Retransmittor() = default;
    Retransmittor(size_type bulk_size);

    void on_read(rc_data&& data);
    void on_socket_close(std::string address);
//    void on_write(); ???

    void run();
private:
    size_type m_bulk_size{3};

    Queue<rc_data> m_storage;
    preprocess::Preprocessor m_prep;

    struct endpoint {}; // just a stub
    struct handler {}; // just a stub
    boost::bimap<
            boost::bimaps::unordered_set_of<
                boost::bimaps::tagged<std::string, endpoint>
            >,
            boost::bimaps::tagged<
                async::handler_t,
                handler
            >
        > m_endpoints_handlers;
};



class session: public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket, Retransmittor& r);
    ~session();

    void start();

private:
    void do_read();

    void do_write(std::size_t length);

    void close();

    tcp::socket m_socket;
    static constexpr std::size_t data_max_length{1024};
    char m_data[data_max_length];
    Retransmittor& m_retransmittor;
};

class server
{
public:
    server(boost::asio::io_context& io_context, short port, Retransmittor& r);

private:
    void do_accept();

    tcp::acceptor m_acceptor;
    Retransmittor& m_retransmittor;
};

}
