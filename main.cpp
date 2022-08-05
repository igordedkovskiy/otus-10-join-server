// homework #8: async command processing (bulk).

#include <iostream>
#include <chrono>
#include <memory>
#include <deque>
#include <unordered_map>
#include <string_view>

#include <utility>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include "async.h"
#include "preprocessor.hpp"
#include "asio_async_server.hpp"

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

        async_server::Queues data;

        auto server_main = [&data](char* argv[])
        {
            boost::asio::io_context io_context;
            async_server::server server(io_context, std::atoi(argv[1]), data);
            io_context.run();
        };

        const std::size_t bulk_size{std::stoul(argv[2])};
        //const std::size_t bulk_size{3};
        std::cout << "bulk size: " << bulk_size << std::endl;

        std::thread t{server_main, argv};
        t.detach();

        preprocess::Preprocessor prep;

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
            > endpoints_handlers;

        const auto handler_for_static{async::connect_with(bulk_size)};
        while(true)
        {
            data.wait();
            while(!data.empty<async_server::rc_data>())
            {
                const auto pack{data.pop<async_server::rc_data>()};
                std::cout << "\nendpoint: " << pack.m_endpoint << std::endl;
                std::cout << "data received: " << pack.m_data << std::endl;
                const auto addr{std::move(pack.m_endpoint)};
                auto el{endpoints_handlers.by<endpoint>().find(addr)};
                async::handler_t h{nullptr};
                if(el == endpoints_handlers.left.end())
                {
                    h = async::connect_with(bulk_size);
                    endpoints_handlers.insert({std::move(addr), h});
                }
                else
                    h = el->second;

                auto where = prep.run(h, pack.m_data.c_str(), pack.m_data.size()-1);
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

            while(!data.empty<async_server::rc_status>())
            {
                const auto pack{data.pop<async_server::rc_status>()};
                std::cout << "\nendpoint " << pack.m_endpoint;
                if(pack.m_socket_is == async_server::rc_status::SocketStatus::CLOSED)
                    std::cout << " is closed" << std::endl;

                auto el{endpoints_handlers.by<endpoint>().find(pack.m_endpoint)};
                if(el != endpoints_handlers.left.end())
                {
                    async::disconnect(el->second);
                    el = endpoints_handlers.left.erase(el);
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << "\n";
    }
    return 0;
}
