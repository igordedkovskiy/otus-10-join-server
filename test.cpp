#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cassert>
#include "gtest/gtest.h"

#include "OtusDB.hpp"
#include "ParseErr.hpp"

void run(otus_db::OtusDB& db, otus_db::sql_t&& sql, const otus_db::SimpleDB::qresult_t& qresult, const std::string& exc_msg)
{
    try
    {
//        std::cout << sql << std::endl;
        const auto res{db.execute_query(std::move(sql))};
//        if(!qresult.second)
//            std::cout << db.last_error_msg() << std::endl;
        ASSERT_TRUE(res.second == qresult.second);
//        for(const auto& row:res.first)
//        {
//            for(const auto& line:row)
//                std::cout << line << std::endl;
//            std::cout << std::endl;
//        }
        ASSERT_TRUE(res.first == qresult.first);
    }
    catch(const ParseErr& e)
    {
        ASSERT_TRUE(e.get_message() == exc_msg);
    }
}

TEST(TEST_SQLITE_WRAP, sqlite_wrapper)
{
    otus_db::OtusDB db;
    db.execute_query("TRUNCATE A");
    db.execute_query("TRUNCATE B");

    otus_db::SimpleDB::qresult_t result{otus_db::SimpleDB::table_t{}, true};
    std::string exc_msg;
    run(db, "INSERT A 4 quality", result, exc_msg);
    run(db, "INSERT A 1 sweater", result, exc_msg);
    run(db, "INSERT A 2 frank", result, exc_msg);
    run(db, "INSERT A 3 violation", result, exc_msg);
    run(db, "INSERT A 0 lean", result, exc_msg);
    run(db, "INSERT A 5 precision", result, exc_msg);

    run(db, "INSERT B 5 lake", result, exc_msg);
    run(db, "INSERT B 3 proposal", result, exc_msg);
    run(db, "INSERT B 4 example", result, exc_msg);
    run(db, "INSERT B 7 wonder", result, exc_msg);
    run(db, "INSERT B 6 flour", result, exc_msg);
    run(db, "INSERT B 8 selection", result, exc_msg);

    result.first = {{"id", "name", "name"},
                    {"3", "violation", "proposal"},
                    {"4", "quality", "example"},
                    {"5", "precision", "lake"}
                   };
    run(db, "INTERSECTION", result, exc_msg);

    result.first = {{"id", "name"},
                    {"0", "lean"},
                    {"1", "sweater"},
                    {"2", "frank"},
                    {"6", "flour"},
                    {"7", "wonder"},
                    {"8", "selection"},
                   };
    run(db, "SYMMETRIC_DIFFERENCE", result, exc_msg);

    result.second = false;
    exc_msg = "ERR duplicate B 7";
    run(db, "INSERT B 7 wonder", result, exc_msg);
    exc_msg = "ERR duplicate A 0";
    run(db, "INSERT A 0 qwerty", result, exc_msg);

    result.first = {{"id", "name"},
                    {"0", "lean"},
                    {"1", "sweater"},
                    {"2", "frank"},
                    {"6", "flour"},
                    {"7", "wonder"},
                    {"8", "selection"},
                   };
    result.second = true;
    run(db, "SYMMETRIC_DIFFERENCE", result, "");
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
