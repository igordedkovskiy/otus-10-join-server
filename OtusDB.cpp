#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "OtusDB.hpp"
#include "ParseErr.hpp"

using namespace otus_db;

OtusDB::OtusDB()
{
    auto print = [](const table_t& table)
    {
        std::cout << "Tables:\n";
        for(const auto& line:table)
        {
            for(const auto& v:line)
                std::cout << v << "  ";
            std::cout << std::endl;
        }
    };

    const auto& [table, res]{m_db.execute_query("SELECT name FROM sqlite_master WHERE type='table';")};
//    print(table);
    if(table.empty())
    {
        m_db.execute_query("CREATE TABLE A (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
        m_db.execute_query("CREATE TABLE B (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
    }
    else if(table.size() == 2)
    {
        if(table[1][0] == "A")
            m_db.execute_query("CREATE TABLE B (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
        else if(table[1][0] == "B")
            m_db.execute_query("CREATE TABLE A (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
    }
//    {
//        const auto& [table, res]{m_db.execute_query("SELECT name FROM sqlite_master WHERE type='table';")};
//        print(table);
//    }
}

std::pair<SimpleDB::sql_cmd_t, sql_t> OtusDB::convert_sql(const sql_t &sql)
{
    sql_t csql;
    std::vector<std::string> cmd;
    if(sql.back() == '\n')
        boost::algorithm::split(cmd, sql_t{std::begin(sql), std::end(sql)-1}, boost::algorithm::is_any_of(" "), boost::token_compress_on);
    else
        boost::algorithm::split(cmd, std::move(sql), boost::algorithm::is_any_of(" "), boost::token_compress_on);
    if(cmd[0] == "INTERSECTION")
        csql = intersection();
    else if(cmd[0] == "SYMMETRIC_DIFFERENCE")
        csql = symmetric_difference();
    else if(cmd[0] == "LIST")
        csql = list_of_tables();
    else if(cmd[0] == "PRINT")
        csql = print(cmd);
    else if(cmd[0] == "INSERT")
        csql = insert(cmd);
    else if(cmd[0] == "TRUNCATE")
        csql = truncate(cmd);
    else
        throw ParseErr{"Unknown querry"};
    return std::make_pair(std::move(cmd), std::move(csql));
}

sql_t OtusDB::intersection()
{
    return "SELECT A.id, A.name, B.name FROM A"
           " INNER JOIN B"
           " WHERE A.id = B.id"
           " ORDER BY A.id ASC;";
}

sql_t OtusDB::symmetric_difference()
{
    return "SELECT A.id, A.name FROM A"
           " WHERE A.id NOT IN (SELECT B.id FROM B)"
           " UNION ALL"
           " SELECT B.id, B.name FROM B"
           " WHERE B.id NOT IN (SELECT A.id FROM A)"
           " ORDER BY id ASC;";
}

sql_t OtusDB::insert(const sql_cmd_t& cmd)
{
    if(cmd.size() != 4)
        throw ParseErr{"Incorrect querry format: INSERT [TABLE_NAME]"};
    return "INSERT INTO " + cmd[1] + " VALUES(" + cmd[2] + ", '" + cmd[3] + "');";
}

sql_t OtusDB::truncate(const sql_cmd_t& cmd)
{
    if(cmd.size() != 2)
        throw ParseErr{"Incorrect querry format: TRUNCATE [TABLE_NAME]"};
    return "DELETE FROM " + cmd[1] + ';';
}

sql_t OtusDB::print(const sql_cmd_t &cmd)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if(cmd.size() != 2)
        throw ParseErr{"Incorrect querry format: PRINT [TABLE_NAME]"};
    return "SELECT * FROM " + cmd[1] + " ORDER BY id ASC;";
}

sql_t OtusDB::list_of_tables()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    return "SELECT name"
           " FROM sqlite_master"
           " WHERE type='table';";
}
