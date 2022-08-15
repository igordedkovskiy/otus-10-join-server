#pragma once

#include <cstdarg>
#include <cstdlib>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>

#include "asio_async_server.hpp"
#include "OtusDB.hpp"

namespace async_server
{

class Retransmittor
{
public:
    using ParseError = std::string;

    Retransmittor() = default;
    ~Retransmittor() = default;

    using read_t = std::pair<std::string, bool>;
    read_t on_read(rc_data&& data);
    void on_socket_close(size_type address);
//    void on_write(); ???

private:
    otus_db::OtusDB m_db;
};

}
