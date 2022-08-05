#include <iostream>

#include "retransmittor.hpp"

using namespace async_server;

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
            //std::cout << "\nendpoint: " << pack.m_endpoint << std::endl;
            //std::cout << "data received: " << pack.m_data << std::endl;
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

            auto where = m_data_preprocessor.run(h, pack.m_data.c_str(), pack.m_data.size()-1);
            std::size_t start{0};
            for(auto [end, type]:where)
            {
                if(type == DataPreprocessor::BlockType::STATIC)
                    async::receive(handler_for_static, pack.m_data.c_str() + start, end - start);
                else
                    async::receive(h, pack.m_data.c_str() + start, end - start);
                start = end;
            }
        }
    }
    async::disconnect(handler_for_static);
    for(auto it = m_endpoints_handlers.right.begin(); it != m_endpoints_handlers.right.end(); ++it)
    {
        async::disconnect(it->first);
        it = m_endpoints_handlers.right.erase(it);
    }
}

void Retransmittor::Queue::push(rc_data &&d)
{
    {
        std::scoped_lock lk{m_mutex};
        m_queue.push_back(std::forward<decltype(d)>(d));
        m_received = true;
    }
    m_cv.notify_one();
}

rc_data Retransmittor::Queue::pop()
{
    std::scoped_lock lk{m_mutex};
    if(m_queue.empty())
        return rc_data{};
    rc_data data{m_queue.front()};
    m_queue.pop_front();
    return data;
}

rc_data Retransmittor::Queue::front()
{
    std::scoped_lock lk{m_mutex};
    if(m_queue.empty())
        return rc_data{};
    rc_data data{m_queue.front()};
    return data;
}

void Retransmittor::Queue::wait()
{
    while(!m_received)
    {
        std::unique_lock lk{m_mutex};
        m_cv.wait(lk, [this]{ return m_received; });
    }
    m_received = false;
}

bool Retransmittor::Queue::empty()
{
    std::unique_lock lk{m_mutex};
    return m_queue.empty();
}

Retransmittor::DataPreprocessor::where_to_cut_t Retransmittor::DataPreprocessor::run(handler_t h, const char *data, size_type data_size, bool update_state)
{
    auto el{m_map.find(h)};
    if(el == std::end(m_map))
    {
        auto e{m_map.insert(std::make_pair(h, helper{}))};
        if(!e.second)
            throw;
        el = e.first;
    }

    auto properties{el->second};
    auto& [braces_cntr, curent_type]{properties};

    where_to_cut_t result;

    for(size_type cntr{0}; cntr < data_size; ++cntr)
    {
        if(data[cntr] == '{')
        {
            if(braces_cntr == 0 && cntr > 1)
                result.emplace_back(std::make_pair(cntr, BlockType::STATIC));

            ++braces_cntr;
            curent_type = BlockType::DYNAMIC;
        }
        else if(data[cntr] == '}')
        {
            if(braces_cntr == 0)
                throw;
            if(--braces_cntr == 0)
            {
                curent_type = BlockType::STATIC;
                result.emplace_back(std::make_pair(cntr + 1, BlockType::DYNAMIC));
            }
        }
    }

    if(update_state)
        el->second = properties;
    if(result.empty())
        result.emplace_back(std::make_pair(data_size, curent_type));
    return result;
}
