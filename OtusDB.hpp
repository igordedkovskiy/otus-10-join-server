#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdarg>
#include <exception>

#include "SimpleDB.hpp"

namespace otus_db
{

class OtusDB: public SimpleDB
{
public:
    OtusDB();

private:
    virtual std::pair<sql_cmd_t, sql_t> convert_sql(const sql_t& sql) override;

    sql_t intersection();
    sql_t symmetric_difference();
    sql_t insert(const sql_cmd_t& cmd);
    sql_t truncate(const sql_cmd_t& cmd);
    sql_t print(const sql_cmd_t& cmd);
    sql_t list_of_tables();
};

}
