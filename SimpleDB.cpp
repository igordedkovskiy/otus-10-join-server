#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "SimpleDB.hpp"
#include "ParseErr.hpp"

using namespace otus_db;

bool SimpleDB::is_opened() const noexcept
{
    return m_db.is_opened();
}

void SimpleDB::close()
{
    m_db.close();
}

SimpleDB::qresult_t SimpleDB::execute_query(const sql_t& sql)
{
    const auto& [cmd, csql]{convert_sql(sql)};
    const auto res{m_db.execute_query(csql)};
    if(!res.second)
    {
        if(m_db.last_error_code() == SQLITE_CONSTRAINT)
        {
            if(cmd[0] == "INSERT")
                throw ParseErr{"ERR duplicate " + cmd[1] + ' ' + cmd[2]};
        }
    }
    return res;
}

SimpleDB::err_t SimpleDB::last_error_code() const noexcept
{
    return m_db.last_error_code();
}

SimpleDB::err_t SimpleDB::last_extended_error_code() const noexcept
{
    return m_db.last_extended_error_code();
}

const std::string &SimpleDB::last_error_msg() const noexcept
{
    return m_db.last_error_msg();
}

std::pair<SimpleDB::sql_cmd_t, sql_t> SimpleDB::convert_sql(const sql_t &sql)
{
    return std::make_pair(sql_cmd_t{}, sql);
}

