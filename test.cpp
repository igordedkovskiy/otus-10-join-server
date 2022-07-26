//#include <vector>
//#include <fstream>
//#include <sstream>
//#include <regex>
//#include <filesystem>
//#include <chrono>
//#include <thread>
//#include "gtest/gtest.h"

//#include "read_input.hpp"
//#include "async.hpp"

//namespace
//{

//constexpr size_type bulk_size{3};
//const char* commands1[] = {"cmd1", "cmd2", "cmd3", "cmd4", "cmd5"};
//constexpr size_type num1 = sizeof(commands1) / sizeof(commands1[0]);
//const char* commands2[] = {"cmd1", "cmd2",
//                           "{", "cmd3", "cmd4", "}",
//                           "{", "cmd5", "cmd6", "{", "cmd7", "cmd8", "}", "cmd9", "}",
//                           "{", "cmd10", "cmd11"
//                          };
//constexpr size_type num2 = sizeof(commands2) / sizeof(commands2[0]);

//auto find_file(std::string masks)
//{
//    const auto regexp_cmp{std::regex(masks)};
//    namespace fs = std::filesystem;
//    const auto path{fs::absolute(".")};
//    using iterator = fs::directory_iterator;
//    std::vector<std::string> fnames;
//    for(auto it{iterator(path)}; it != iterator(); ++it)
//    {
//        const auto path{fs::absolute(it->path())};
//        if(!fs::is_directory(path))
//        {
//            const auto fname{path.filename().string()};
//            if(regex_match(fname, regexp_cmp))
//                fnames.emplace_back(std::move(fname));
//        }
//    }
//    return fnames;
//}

//bool check(std::stringstream ref, std::string masks)
//{
//    const auto fname{find_file(std::move(masks))};
//    if(fname.empty())
//        return false;
//    for(const auto& fn:fname)
//    {
//        std::fstream file{fn, std::fstream::in};
//        std::stringstream out;
//        out << file.rdbuf();
//        if(out.str() != ref.str())
//            return false;
//    }
//    return true;
//}

//void check_all()
//{
//    // context 1
//    ASSERT_TRUE(check(std::stringstream{"bulk: cmd1, cmd2, cmd3\n"}, std::string{"(bulk-1-.*-1.log)"}));
//    ASSERT_TRUE(check(std::stringstream{"bulk: cmd4, cmd5\n"}, std::string{"(bulk-1-.*-2.log)"}));

//    // context 2
//    ASSERT_TRUE(check(std::stringstream{"bulk: cmd1, cmd2\n"}, std::string{"(bulk-2-.*-1.log)"}));
//    ASSERT_TRUE(check(std::stringstream{"bulk: cmd3, cmd4\n"}, std::string{"(bulk-2-.*-2.log)"}));
//    ASSERT_TRUE(check(std::stringstream{"bulk: cmd5, cmd6, cmd7, cmd8, cmd9\n"}, std::string{"(bulk-2-.*-3.log)"}));
//}

//}

//TEST(TEST_ASYNC, async_sinlge_thread)
//{
//    const auto h1{connect(bulk_size)};
//    const auto h2{connect(bulk_size)};

//    receive(h2, commands2, num2);
//    receive(h1, commands1, num1);

//    disconnect(h1);
//    disconnect(h2);

//    check_all();
//}

//TEST(TEST_ASYNC, async_multiple_threads)
//{
//    const auto h1{connect(bulk_size)};
//    const auto h2{connect(bulk_size)};

//    std::thread t1{receive, h1, std::ref(commands1), num1};
//    std::thread t2{receive, h2, std::ref(commands2), num2};
//    t1.join();
//    t2.join();

//    disconnect(h1);
//    disconnect(h2);

//    check_all();
//}

//TEST(TEST_ASYNC, async_sinlge_thread_gradually)
//{
//    const auto h1{connect(bulk_size)};
//    const auto h2{connect(bulk_size)};

//    receive(h2, commands2, 2);
//    receive(h1, commands1, 3);

//    receive(h2, &commands2[0] + 2, 16);
//    receive(h1, &commands1[0] + 3, 2);

//    disconnect(h1);
//    disconnect(h2);

//    check_all();
//}

//int main(int argc, char** argv)
//{
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}
