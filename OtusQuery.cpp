#include <iostream>

#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "OtusQuery.hpp"
#include "ParseErr.hpp"

using namespace otus_db;

namespace
{

sql_t intersection(const QueryConverter::sql_cmd_t& cmd);
sql_t symmetric_difference(const QueryConverter::sql_cmd_t& cmd);
sql_t insert(const QueryConverter::sql_cmd_t& cmd);
sql_t truncate(const QueryConverter::sql_cmd_t& cmd);
sql_t print(const QueryConverter::sql_cmd_t& cmd);
sql_t list_of_tables(const QueryConverter::sql_cmd_t& cmd);

}

OtusQuery::OtusQuery():
    m_map{
            {std::hash<std::string>{}("INSERT"), insert},
            {std::hash<std::string>{}("TRUNCATE"), truncate},
            {std::hash<std::string>{}("INTERSECTION"), intersection},
            {std::hash<std::string>{}("SYMMETRIC_DIFFERENCE"), symmetric_difference},
            {std::hash<std::string>{}("PRINT"), print},
            {std::hash<std::string>{}("LIST"), list_of_tables},
         }
{}

QueryConverter::convert_result_t OtusQuery::convert_sql(sql_t sql)
{
    std::vector<std::string> cmd;
    if(sql.back() == '\n')
        sql.resize(sql.size() - 1);
    boost::algorithm::split(cmd, std::move(sql), boost::algorithm::is_any_of(" "), boost::token_compress_on);
    const auto h{std::hash<std::string>{}(cmd[0])};
    auto el{m_map.find(h)};
    if(el == std::end(m_map))
        throw ParseErr{"Unknown querry"};
    return std::make_pair(std::move(cmd), el->second(cmd));
}

namespace
{

sql_t intersection([[maybe_unused]] const QueryConverter::sql_cmd_t& cmd)
{
    return "SELECT A.id, A.name, B.name FROM A"
           " INNER JOIN B"
           " WHERE A.id = B.id"
           " ORDER BY A.id ASC;";
}

sql_t symmetric_difference([[maybe_unused]] const QueryConverter::sql_cmd_t& cmd)
{
    return "SELECT A.id, A.name FROM A"
           " WHERE A.id NOT IN (SELECT B.id FROM B)"
           " UNION ALL"
           " SELECT B.id, B.name FROM B"
           " WHERE B.id NOT IN (SELECT A.id FROM A)"
           " ORDER BY id ASC;";
}

sql_t insert(const QueryConverter::sql_cmd_t& cmd)
{
    if(cmd.size() != 4)
        throw ParseErr{"Incorrect querry format: INSERT [TABLE_NAME]"};
    return "INSERT INTO " + cmd[1] + " VALUES(" + cmd[2] + ", '" + cmd[3] + "');";
}

sql_t truncate(const QueryConverter::sql_cmd_t& cmd)
{
    if(cmd.size() != 2)
        throw ParseErr{"Incorrect querry format: TRUNCATE [TABLE_NAME]"};
    return "DELETE FROM " + cmd[1] + ';';
}

sql_t print(const QueryConverter::sql_cmd_t &cmd)
{
    if(cmd.size() != 2)
        throw ParseErr{"Incorrect querry format: PRINT [TABLE_NAME]"};
    return "SELECT * FROM " + cmd[1] + " ORDER BY id ASC;";
}

sql_t list_of_tables([[maybe_unused]] const QueryConverter::sql_cmd_t& cmd)
{
    return "SELECT name"
           " FROM sqlite_master"
           " WHERE type='table';";
}

}
