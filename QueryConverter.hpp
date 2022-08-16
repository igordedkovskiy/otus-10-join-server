#pragma once

#include <string>
#include <vector>

namespace otus_db
{

using sql_t = std::string;

class QueryConverter
{
public:
    using err_t = int;
    using sql_cmd_t = std::vector<std::string>;
    using table_t = std::vector<std::vector<std::string>>;
    using convert_result_t = std::pair<sql_cmd_t, sql_t>;

    virtual ~QueryConverter() = default;

//    virtual convert_result_t convert_sql(const sql_t& sql);
//    virtual convert_result_t convert_sql(sql_t&& sql);
    virtual convert_result_t convert_sql(sql_t sql);
};

}
