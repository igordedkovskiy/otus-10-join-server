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

        async_server::Queue queue;

        auto server_main = [&queue](char* argv[])
        {
            boost::asio::io_context io_context;
            async_server::server server(io_context, std::atoi(argv[1]), queue);
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
            queue.wait();
            while(!queue.empty())
            {
                const auto pack{queue.pop()};
                std::cout << "\nendpoint: " << pack.m_endpoint << std::endl;
                std::cout << "\nendpoint: " << pack.m_endpoint.address() << ':' << pack.m_endpoint.port() << std::endl;
                std::cout << "data received: " << pack.m_data << std::endl;
                const auto addr{pack.m_endpoint.address().to_string() + std::to_string(pack.m_endpoint.port())};
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
                std::size_t prev_pos{0};
                for(auto [pos, type]:where)
                {
                    if(type == preprocess::Preprocessor::BlockType::STATIC)
                        async::receive(handler_for_static, pack.m_data.c_str(), pos - prev_pos);
                    else
                        async::receive(h, pack.m_data.c_str(), pos - prev_pos);
                    prev_pos = pos;
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
