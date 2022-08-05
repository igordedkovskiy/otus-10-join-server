// homework #8: async command processing (bulk).

#include <iostream>
#include <utility>

#include <boost/asio.hpp>

#include "asio_async_server.hpp"
#include "retransmittor.hpp"

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    try
    {
        if(argc != 3)
        {
            std::cerr << "Usage: bulk_server <port> <bulk_size>\n";
            return 1;
        }
        const std::size_t bulk_size{std::stoul(argv[2])};
        async_server::Retransmittor retransmittor{bulk_size};
        auto server_main = [&retransmittor](char* argv[])
        {
            boost::asio::io_context io_context;
            async_server::server server(io_context, std::atoi(argv[1]), retransmittor);
            io_context.run();
        };

        std::thread t{server_main, argv};
        t.detach();
        retransmittor.run();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
    }
    return 0;
}
