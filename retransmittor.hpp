#pragma once

#include <iostream>
#include <cstdarg>
#include <cstdlib>
#include <deque>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/pool/pool_alloc.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "async.h"
#include "asio_async_server.hpp"

namespace async_server
{

class Retransmittor
{
    class DB
    {
    public:
        using err_t = int;

        DB(const std::string& name):
            m_name{name}
        {
            if(sqlite3_open(m_name.c_str(), &m_handle))
            {
                std::cerr << "Can't open database: " << sqlite3_errmsg(m_handle) << std::endl;
                sqlite3_close(m_handle);
                m_handle = nullptr;
            }
            else
                std::cout << m_name << " database opened successfully!" << std::endl;
        }

        ~DB()
        {
            close();
        }

        bool is_valid() const noexcept { return m_handle; }

        void close() { if(m_handle) sqlite3_close(m_handle); }

        bool execute_query(const std::string& sql)
        {
            auto print_results = [](void *, int columns, char **data, char **names) -> int
            {
                for(int i = 0; i < columns; ++i)
                    std::cout << names[i] << " = " << (data[i] ? data[i] : "NULL") << std::endl;
                std::cout << std::endl;
                return 0;
            };

            char* errmsg{nullptr};
            m_last_err_code = sqlite3_exec(m_handle, sql.c_str(), print_results, 0, &errmsg);
            m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
            if(m_last_err_code != SQLITE_OK)
            {
                std::cerr << "Can't execute query: " << errmsg << std::endl;
                std::cerr << "Error code: " << m_last_err_code << std::endl;
                m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
                std::cerr << "Extended error code: " << m_last_ext_err_code << std::endl;
                if(m_last_err_code == SQLITE_CONSTRAINT)
                {
                    if(m_last_ext_err_code == SQLITE_CONSTRAINT_PRIMARYKEY)
                        std::cerr << "ERR duplicate: ?" << std::endl;
                    std::cerr << sql << std::endl;
                }
                m_last_err_msg = errmsg;
                sqlite3_free(errmsg);
                return false;
            }
            return true;
        }

        err_t last_error_code() const noexcept
        {
            return m_last_err_code;
        }

        err_t last_extended_error_code() const noexcept
        {
            return m_last_ext_err_code;
        }

        const std::string& last_error_msg() const noexcept
        {
            return m_last_err_msg;
        }

    private:
        sqlite3* m_handle{nullptr};
        const std::string m_name;
        std::string m_last_err_msg;
        err_t m_last_err_code{0};
        err_t m_last_ext_err_code{0};
    };

public:
    Retransmittor() = default;
    Retransmittor(size_type bulk_size);
    ~Retransmittor();

    void on_read(rc_data&& data);
    void on_socket_close(size_type address);
//    void on_write(); ???

private:
    size_type m_bulk_size{3};
    handler_t m_static_bulks_handler{nullptr};
    DB m_db{"cpp_sqlite_db.sqlite"};

    /// \note The allocator is specified because gcc-11 fails to compile the code
    ///       with default allocator and c++20.
    ///       In gcc-12 the problem is fixed.
    boost::bimap<boost::bimaps::unordered_set_of<size_type>
                ,async::handler_t
                ,boost::fast_pool_allocator<std::pair<size_type, async::handler_t>>
                //,boost::container::allocator<std::pair<std::string, async::handler_t>>
                > m_endpoints_handlers;
};

}
