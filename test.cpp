#include <vector>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <cassert>
#include "gtest/gtest.h"

#include "read_input.hpp"
#include "async.hpp"

namespace
{

constexpr size_type bulk_size{3};

auto find_file(std::string masks)
{
    const auto regexp_cmp{std::regex(masks)};
    namespace fs = std::filesystem;
    const auto path{fs::absolute(".")};
    using iterator = fs::directory_iterator;
    std::vector<std::string> fnames;
    for(auto it{iterator(path)}; it != iterator(); ++it)
    {
        const auto path{fs::absolute(it->path())};
        if(!fs::is_directory(path))
        {
            const auto fname{path.filename().string()};
            if(regex_match(fname, regexp_cmp))
                fnames.emplace_back(std::move(fname));
        }
    }
    return fnames;
}

bool check(std::stringstream ref, std::string masks)
{
    const auto fname{find_file(std::move(masks))};
    if(fname.empty())
    {
        std::cout << "File not found!\n";
        return false;
    }
    for(const auto& fn:fname)
    {
        std::fstream file{fn, std::fstream::in};
        std::stringstream out;
        out << file.rdbuf();
        if(out.str() == ref.str())
            return true;
    }
    return false;
}

}

TEST(TEST_ASYNC, async_sinlge_thread)
{
    const auto h1{connect(bulk_size)};
    const auto h2{connect(bulk_size)};

    receive(h2, "cmd1\n", 5);
    receive(h1, "cmd1\ncmd2\n", 10);
    receive(h2, "cmd2\n{\ncmd3\ncmd4\n}\n", 19);
    receive(h2, "{\n", 2);
    receive(h2, "cmd5\ncmd6\n{\ncmd7\ncmd8\n}\ncmd9\n}\n", 31);
    receive(h1, "cmd3\ncmd4\n", 10);
    receive(h2, "{\ncmd10\ncmd11\n", 12);
    receive(h1, "cmd5\n", 5);

    disconnect(h1);
    disconnect(h2);

    wait();

    // context 1
    ASSERT_TRUE(check(std::stringstream{"bulk: cmd1, cmd2, cmd3\n"}, "(bulk-" + std::to_string(h1) + "-.*.log)"));
    ASSERT_TRUE(check(std::stringstream{"bulk: cmd4, cmd5\n"}, "(bulk-" + std::to_string(h1) + "-.*.log)"));

    // context 2
    ASSERT_TRUE(check(std::stringstream{"bulk: cmd1, cmd2\n"}, "(bulk-" + std::to_string(h2) + "-.*.log)"));
    ASSERT_TRUE(check(std::stringstream{"bulk: cmd3, cmd4\n"}, "(bulk-" + std::to_string(h2) + "-.*.log)"));
    ASSERT_TRUE(check(std::stringstream{"bulk: cmd5, cmd6, cmd7, cmd8, cmd9\n"}, "(bulk-" + std::to_string(h2) + "-.*.log)"));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
