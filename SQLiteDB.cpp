#include <iostream>
#include <cstdarg>

#include "SQLiteDB.hpp"

extern "C"
{
#include "sqlite3.h"
}

using namespace otus_db;

SQLiteDB::SQLiteDB(const std::string &name):
    m_name{name}
{
    if(sqlite3_open(m_name.c_str(), &m_handle))
    {
//        std::cerr << "Can't open database: " << sqlite3_errmsg(m_handle) << std::endl;
        sqlite3_close(m_handle);
        m_handle = nullptr;
    }
    else
        std::cout << m_name << " database opened successfully!" << std::endl;
}

SQLiteDB::~SQLiteDB()
{
    close();
}

bool SQLiteDB::is_opened() const noexcept
{
    return m_handle;
}

void SQLiteDB::close()
{
    if(m_handle)
        sqlite3_close(m_handle);
    m_handle = nullptr;
}

const std::string& SQLiteDB::last_error_msg() const noexcept
{
    return m_last_err_msg;
}

SQLiteDB::err_t SQLiteDB::last_extended_error_code() const noexcept
{
    return m_last_ext_err_code;
}

SQLiteDB::err_t SQLiteDB::last_error_code() const noexcept
{
    return m_last_err_code;
}

SQLiteDB::qresult_t SQLiteDB::execute_query(const std::string &sql)
{
    auto get_result = [](void* resPtr, int columns, char **data, char **names) -> int
    {
        auto& result{*static_cast<table_t*>(resPtr)};
        if(result.empty())
        {
            result.emplace_back(std::vector<std::string>{});
            auto& line{result.back()};
            for(decltype(columns) i{0}; i < columns; ++i)
                line.emplace_back(names[i]);
        }
        result.emplace_back(std::vector<std::string>{});
        auto& line{result.back()};
        for(decltype(columns) i{0}; i < columns; ++i)
            line.emplace_back(data[i] ? data[i] : "");
        return 0;
    };

    char* errmsg{nullptr};
    table_t result;
    m_last_err_code = sqlite3_exec(m_handle, sql.c_str(), get_result, &result, &errmsg);
    m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
    if(m_last_err_code != SQLITE_OK)
    {
//        std::cerr << "Querry: " << sql << std::endl;
//        std::cerr << "Can't execute query: " << errmsg << std::endl;
//        std::cerr << "Error code: " << m_last_err_code << std::endl;
        m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
//        std::cerr << "Extended error code: " << m_last_ext_err_code << std::endl;
//        if(m_last_err_code == SQLITE_CONSTRAINT)
//        {
//            if(m_last_ext_err_code == SQLITE_CONSTRAINT_PRIMARYKEY)
//                std::cerr << "ERR duplicate: ?" << std::endl;
//            std::cerr << sql << std::endl;
//        }
        m_last_err_msg = errmsg;
        sqlite3_free(errmsg); // <--- обратите внимание не C-style работу с памятью
        return std::make_pair(table_t{}, false);
    }
    return std::make_pair(std::move(result), true);
}
