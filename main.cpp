// homework #9: async server - bulk server.

#include <iostream>
#include <utility>

#include <boost/asio.hpp>

#include "asio_async_server.hpp"
#include "retransmittor.hpp"

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));
    if(argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    try
    {
        async_server::Retransmittor retransmittor;
        std::locale::global(std::locale(""));
        boost::asio::io_context io_context;
        async_server::server server(io_context, std::atoi(argv[1]), retransmittor);
        io_context.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
