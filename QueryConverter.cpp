#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "QueryConverter.hpp"
#include "ParseErr.hpp"

using namespace otus_db;

QueryConverter::convert_result_t QueryConverter::convert_sql(sql_t sql)
{
    return std::make_pair(sql_cmd_t{}, std::move(sql));
}

