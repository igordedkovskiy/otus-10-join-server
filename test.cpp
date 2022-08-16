#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cassert>
#include "gtest/gtest.h"

#include "SQLiteDB.hpp"
#include "OtusQuery.hpp"
#include "ParseErr.hpp"

void run(otus_db::SQLiteDB& db, otus_db::QueryConverter& conv, otus_db::sql_t sql, const otus_db::SQLiteDB::qresult_t& qresult, const std::string& exc_msg)
{
    try
    {
//        std::cout << sql << std::endl;
        const auto converted{conv.convert_sql(std::move(sql))};
//        std::cout << converted.second << std::endl;
        const auto res{db.execute_query(converted.second)};
//        if(!res.second)
//            std::cout << db.last_error_msg() << std::endl;
        EXPECT_EQ(res.second, qresult.second);
//        for(const auto& row:res.first)
//        {
//              for(const auto& line:row)
//                std::cout << line << std::endl;
//            std::cout << std::endl;
//        }
        EXPECT_EQ(res.first, qresult.first);
    }
    catch(const ParseErr& e)
    {
        EXPECT_EQ(e.get_message(), exc_msg);
    }
}

TEST(TEST_SQLITE_WRAP, sqlite_wrapper)
{
    otus_db::SQLiteDB db{"cpp_sqlite_db.sqlite"};
    {
        const auto [table, res]{db.execute_query("SELECT name FROM sqlite_master WHERE type='table';")};
        if(table.empty())
        {
            db.execute_query("CREATE TABLE A (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
            db.execute_query("CREATE TABLE B (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
        }
        else if(table.size() == 2)
        {
            if(table[1][0] == "A")
                db.execute_query("CREATE TABLE B (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
            else if(table[1][0] == "B")
                db.execute_query("CREATE TABLE A (id int NOT NULL, name text NOT NULL, PRIMARY KEY (id));");
        }
    }

    otus_db::OtusQuery conv;
    auto converted{conv.convert_sql("TRUNCATE A")};
    auto res = db.execute_query(converted.second);
//    if(!res.second)
//        std::cout << db.last_error_msg() << std::endl;
    converted = conv.convert_sql("TRUNCATE B");
    res = db.execute_query(converted.second);
//    if(!res.second)
//        std::cout << db.last_error_msg() << std::endl;
    
    otus_db::SQLiteDB::qresult_t result{otus_db::SQLiteDB::table_t{}, true};
    std::string exc_msg;
    run(db, conv, "INSERT A 4 quality", result, exc_msg);
    run(db, conv, "INSERT A 1 sweater", result, exc_msg);
    run(db, conv, "INSERT A 2 frank", result, exc_msg);
    run(db, conv, "INSERT A 3 violation", result, exc_msg);
    run(db, conv, "INSERT A 0 lean", result, exc_msg);
    run(db, conv, "INSERT A 5 precision", result, exc_msg);

    run(db, conv, "INSERT B 5 lake", result, exc_msg);
    run(db, conv, "INSERT B 3 proposal", result, exc_msg);
    run(db, conv, "INSERT B 4 example", result, exc_msg);
    run(db, conv, "INSERT B 7 wonder", result, exc_msg);
    run(db, conv, "INSERT B 6 flour", result, exc_msg);
    run(db, conv, "INSERT B 8 selection", result, exc_msg);

    result.first = {{"id", "name", "name"},
                    {"3", "violation", "proposal"},
                    {"4", "quality", "example"},
                    {"5", "precision", "lake"}
                   };
    run(db, conv, "INTERSECTION", result, exc_msg);

    result.first = {{"id", "name"},
                    {"0", "lean"},
                    {"1", "sweater"},
                    {"2", "frank"},
                    {"6", "flour"},
                    {"7", "wonder"},
                    {"8", "selection"},
                   };
    run(db, conv, "SYMMETRIC_DIFFERENCE", result, exc_msg);

    result.first = otus_db::SQLiteDB::table_t{};
    result.second = false;
    exc_msg = "ERR duplicate B 7";
    run(db, conv, "INSERT B 7 wonder", result, exc_msg);
    exc_msg = "ERR duplicate A 0";
    run(db, conv, "INSERT A 0 qwerty", result, exc_msg);

    result.first = {{"id", "name"},
                    {"0", "lean"},
                    {"1", "sweater"},
                    {"2", "frank"},
                    {"6", "flour"},
                    {"7", "wonder"},
                    {"8", "selection"},
                   };
    result.second = true;
    run(db, conv, "SYMMETRIC_DIFFERENCE", result, "");
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
