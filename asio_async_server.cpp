#include "asio_async_server.hpp"

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
                std::cout << "eof!" << std::endl;
                m_retransmittor.on_socket_close({m_socket.remote_endpoint().address().to_string() +
                                                 std::to_string(m_socket.remote_endpoint().port()) });
                close();
            }
            else if(ec == boost::asio::error::connection_reset)
                std::cout << "connection reset!" << std::endl;
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

rc_status::rc_status(const endpoint_t &epoint, SocketStatus st):
    m_endpoint{epoint.address().to_string() + std::to_string(epoint.port())},
    m_socket_is{st}
{}


Retransmittor::Retransmittor(size_type bulk_size):
    m_bulk_size{bulk_size}
{}

void Retransmittor::on_read(rc_data&& data)
{
    m_storage.push(std::forward<decltype(data)>(data));
}

void Retransmittor::on_socket_close(std::string address)
{
    auto el{m_endpoints_handlers.by<endpoint>().find(address)};
    if(el != m_endpoints_handlers.left.end())
    {
        async::disconnect(el->second);
        el = m_endpoints_handlers.left.erase(el);
    }
}

void Retransmittor::run()
{
    const auto handler_for_static{async::connect_with(m_bulk_size)};
    while(true)
    {
        m_storage.wait();
        while(!m_storage.empty())
        {
            const auto pack{m_storage.pop()};
            std::cout << "\nendpoint: " << pack.m_endpoint << std::endl;
            std::cout << "data received: " << pack.m_data << std::endl;
            const auto addr{std::move(pack.m_endpoint)};
            auto el{m_endpoints_handlers.by<endpoint>().find(addr)};
            async::handler_t h{nullptr};
            if(el == m_endpoints_handlers.left.end())
            {
                h = async::connect_with(m_bulk_size);
                m_endpoints_handlers.insert({std::move(addr), h});
            }
            else
                h = el->second;

            auto where = m_prep.run(h, pack.m_data.c_str(), pack.m_data.size()-1);
            std::size_t start{0};
            for(auto [end, type]:where)
            {
                if(type == preprocess::Preprocessor::BlockType::STATIC)
                    async::receive(handler_for_static, pack.m_data.c_str() + start, end - start);
                else
                    async::receive(h, pack.m_data.c_str() + start, end - start);
                start = end;
            }
        }

//        while(!storage.empty<async_server::rc_status>())
//        {
//            const auto pack{storage.pop<async_server::rc_status>()};
//            std::cout << "\nendpoint " << pack.m_endpoint;
//            if(pack.m_socket_is == async_server::rc_status::SocketStatus::CLOSED)
//                std::cout << " is closed" << std::endl;

//            auto el{endpoints_handlers.by<endpoint>().find(pack.m_endpoint)};
//            if(el != endpoints_handlers.left.end())
//            {
//                async::disconnect(el->second);
//                el = endpoints_handlers.left.erase(el);
//            }
//        }
    }
    async::disconnect(handler_for_static);
    for(auto it = m_endpoints_handlers.right.begin(); it != m_endpoints_handlers.right.end(); ++it)
    {
        async::disconnect(it->first);
        it = m_endpoints_handlers.right.erase(it);
    }
}
