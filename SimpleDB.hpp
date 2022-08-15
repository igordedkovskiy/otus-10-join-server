#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdarg>
#include <exception>

#include "SQLiteDB.hpp"

namespace otus_db
{

class SimpleDB
{
public:
    using err_t = int;
    using sql_t = std::string;
    using sql_cmd_t = std::vector<std::string>;
    using table_t = std::vector<std::vector<std::string>>;
    using qresult_t = std::pair<table_t, bool>;

    virtual ~SimpleDB() = default;

    bool is_opened() const noexcept;

    void close();

    qresult_t execute_query(const sql_t& sql);

    err_t last_error_code() const noexcept;

    err_t last_extended_error_code() const noexcept;

    const std::string& last_error_msg() const noexcept;

private:
    virtual std::pair<sql_cmd_t, sql_t> convert_sql(const sql_t& sql);

protected:
    SQLiteDB m_db{"cpp_sqlite_db.sqlite"};
};

}
