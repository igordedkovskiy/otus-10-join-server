#include "asio_async_server.hpp"

namespace async_server
{

rc_data::rc_data(const char *data, std::size_t size, endpoint_t &&epoint):
    m_data{data, size},
    m_endpoint{std::forward<decltype(epoint)>(epoint)}
{}

session::session(tcp::socket socket, Queue& queue):
    m_socket(std::move(socket)),
    m_queue{queue}
{}

void session::start()
{
    do_read();
}

void session::do_read()
{
    auto self(shared_from_this());
    auto process = [this, self](boost::system::error_code ec, std::size_t length)
    {
        if(!ec)
        {
//            {
//                std::scoped_lock lk{cv_m};
//                messages_queue.push_back({m_data, length, m_socket.remote_endpoint()});
//                received = true;
//            }
//            cv.notify_one();
            m_queue.push({m_data, length, m_socket.remote_endpoint()});
            do_write(length);
        }
    };
    m_socket.async_read_some(boost::asio::buffer(m_data, data_max_length), process);
}

void session::do_write(std::size_t length)
{
    auto self(shared_from_this());
    auto process = [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if(!ec)
            do_read();
    };
    boost::asio::async_write(m_socket, boost::asio::buffer(m_data, length), process);
}

server::server(boost::asio::io_context &io_context, short port, Queue &queue):
    m_acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
    m_queue{queue}
{
    do_accept();
}

void server::do_accept()
{
    auto process = [this](boost::system::error_code ec, tcp::socket socket)
    {
        if(!ec)
            std::make_shared<session>(std::move(socket), m_queue)->start();
        do_accept();
    };
    m_acceptor.async_accept(process);
}

void Queue::push(rc_data &&d)
{
    {
        std::scoped_lock lk{m_mutex};
        m_messages_queue.push_back(std::forward<decltype(d)>(d));
        m_received = true;
    }
    m_cv.notify_one();
}

rc_data Queue::pop()
{
    std::scoped_lock lk{m_mutex};
    if(m_messages_queue.empty())
        return rc_data{};
    rc_data data{m_messages_queue.front()};
    m_messages_queue.pop_front();
    return data;
}

rc_data Queue::front()
{
    std::scoped_lock lk{m_mutex};
    if(m_messages_queue.empty())
        return rc_data{};
    rc_data data{m_messages_queue.front()};
    return data;
}

void Queue::wait()
{
    while(!m_received)
    {
        std::unique_lock lk{m_mutex};
        m_cv.wait(lk, [this]{ return m_received; });
    }
    m_received = false;
}

bool Queue::empty()
{
    std::unique_lock lk{m_mutex};
    return m_messages_queue.empty();
}

}
