#pragma once

#include <string>
#include <array>
#include <vector>
#include <cstdarg>

struct sqlite3;

namespace otus_db
{

using sql_t = std::string;

class SQLiteDB
{
public:
    using err_t = int;
    using table_t = std::vector<std::vector<std::string>>;
    using qresult_t = std::pair<table_t, bool>;

    SQLiteDB(const std::string& name);

    ~SQLiteDB();

    bool is_opened() const noexcept;

    void close();

    qresult_t execute_query(const sql_t& sql);

    err_t last_error_code() const noexcept;

    err_t last_extended_error_code() const noexcept;

    const std::string& last_error_msg() const noexcept;

private:
    sqlite3* m_handle{nullptr};
    const std::string m_name;
    std::string m_last_err_msg;
    err_t m_last_err_code{0};
    err_t m_last_ext_err_code{0};
};

}
