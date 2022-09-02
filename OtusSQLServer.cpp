#include <iostream>

#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string.hpp>

extern "C"
{
#include "sqlite3.h"
}

#include "OtusSQLServer.hpp"
#include "ParseErr.hpp"

using namespace async_server;
using otus_db::OtusQuery;

OtusSQLServer::OtusSQLServer():
    m_converter{std::make_unique<OtusQuery>()}
{
    [[maybe_unused]] auto print = [](const table_t& table)
    {
        std::cout << "Tables:\n";
        for(const auto& line:table)
        {
            for(const auto& v:line)
                std::cout << v << "  ";
            std::cout << std::endl;
        }
    };

    const auto [table, res]{m_db.execute_query("SELECT name FROM sqlite_master WHERE type='table';")};
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

async_server::OtusSQLServer::answer_t async_server::OtusSQLServer::on_read(rc_data&& data)
{
    try
    {
        const auto [cmd, csql]{m_converter->convert_sql(std::move(data.m_data))};
        const auto [table, res]{m_db.execute_query(csql)};
        std::string msg;
        if(!res)
        {
            if(m_db.last_error_code() == SQLITE_CONSTRAINT)
            {
                if(cmd[0] == "INSERT")
                    return "ERR duplicate " + cmd[1] + ' ' + cmd[2] + '\n';
            }
            return "ERR\n";
        }
        for(std::size_t cntr{1}; cntr < table.size(); ++cntr)
        {
            auto& row{table[cntr]};
            for(auto& cell:row)
            {
                msg += std::move(cell);
                if(msg.back() == '\n')
                    msg.resize(msg.size() - 1);
                msg += ' ';
            }
            msg += '\n';
        }
        msg += "OK\n";
        return msg;
    }
    catch(const ParseErr& e)
    {
        return std::move(e.get_message()) + "\nERR\n";
    }
}

void OtusSQLServer::on_socket_close([[maybe_unused]] size_type address) {}
