#pragma once

#include <cstdlib>

#include <boost/asio.hpp>

#include "asio_async_server.hpp"
#include "SQLiteDB.hpp"
#include "OtusQuery.hpp"

namespace async_server
{

class OtusSQLServer
{
public:
    using answer_t = std::string;

    OtusSQLServer();
    virtual ~OtusSQLServer() = default;

    answer_t on_read(rc_data&& data);
    void on_socket_close(size_type address);
//    void on_write(); ???

private:
    using table_t = std::vector<std::vector<std::string>>;

    otus_db::SQLiteDB m_db{"cpp_sqlite_db.sqlite"};
    std::unique_ptr<otus_db::QueryConverter> m_converter;
};


}
