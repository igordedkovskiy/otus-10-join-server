#include <iostream>
#include <memory>

#include "asio_async_server.hpp"
#include "retransmittor.hpp"

using namespace async_server;

session::session(tcp::socket socket, Retransmittor& r):
    m_socket(std::move(socket)),
    m_retransmittor{r}
{}

session::~session()
{
    close();
}

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
            m_retransmittor.on_read({m_data, length, m_socket.remote_endpoint()});
            do_write(length);
        }
        else
        {
            if(ec == boost::asio::error::eof)
            {
                //std::cout << "eof!" << std::endl;
                m_retransmittor.on_socket_close({m_socket.remote_endpoint().address().to_string() +
                                                 std::to_string(m_socket.remote_endpoint().port()) });
                close();
            }
            //else if(ec == boost::asio::error::connection_reset)
            //    std::cout << "connection reset!" << std::endl;
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

void session::close()
{
    if(m_socket.is_open())
    {
        m_socket.shutdown(boost::asio::socket_base::shutdown_both);
        m_socket.close();
    }
}

server::server(boost::asio::io_context &io_context, short port, Retransmittor& r):
    m_acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
    m_retransmittor{r}
{
    do_accept();
}

void server::do_accept()
{
    auto process = [this](boost::system::error_code ec, tcp::socket socket)
    {
        if(!ec)
            std::make_shared<session>(std::move(socket), m_retransmittor)->start();
        do_accept();
    };
    m_acceptor.async_accept(process);
}

rc_data::rc_data(const char *data, std::size_t size, const endpoint_t &epoint):
    m_data{data, size},
    m_endpoint{epoint.address().to_string() + std::to_string(epoint.port())}
{}
