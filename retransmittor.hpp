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
//#include <boost/bimap.hpp>
//#include <boost/bimap/unordered_set_of.hpp>
//#include <boost/pool/pool_alloc.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "asio_async_server.hpp"

namespace async_server
{

class Retransmittor
{
    class DB
    {
    public:
        using err_t = int;

        DB(std::string&& name);

        ~DB();

        bool is_valid() const;

        void close();

        bool execute_query(const std::string& sql);

        err_t last_error_code() const;

        err_t last_extended_error_code() const;

        const std::string& last_error_msg() const;

    private:
        sqlite3* m_handle{nullptr};
        const std::string m_name;
        std::string m_last_err_msg;
        err_t m_last_err_code{0};
        err_t m_last_ext_err_code{0};
    };

public:
    using ParseError = std::string;
    enum class ExecError: std::uint8_t
    {
        incorrect_format
    }

    Retransmittor() = default;
    Retransmittor(std::string&& db_name);
    ~Retransmittor() = default;

    void on_read(rc_data&& data);
    void on_socket_close(size_type address);
//    void on_write(); ???

private:
    DB m_db{"cpp_sqlite_db.sqlite"};

    /// \note The allocator is specified because gcc-11 fails to compile the code
    ///       with default allocator and c++20.
    ///       In gcc-12 the problem is fixed.
//    boost::bimap<boost::bimaps::unordered_set_of<size_type>
//                ,async::handler_t
//                ,boost::fast_pool_allocator<std::pair<size_type, async::handler_t>>
//                > m_endpoints_handlers;
};

}
