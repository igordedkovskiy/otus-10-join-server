#include <vector>
//#include <fstream>
#include <sstream>
#include "gtest/gtest.h"

#include "CmdCollector.hpp"
#include "read_input.hpp"

TEST(TEST_ASYNC, task_example)
{
    CmdCollector commands{3};

    auto print = [&commands](std::stringstream& out)
    {
        out << "bulk: ";
        std::size_t cntr = 0;
        for(const auto& cmd:commands.get_cmd())
        {
            out << cmd;
            if(++cntr < commands.block_size())
                out << ", ";
        }
        out << '\n';
        commands.clear_commands();
    };

    std::stringstream result;
    auto process = [&commands, &print, &result](std::string&& read)
    {
        commands.process_cmd(std::move(read));
        read.clear();
        if(commands.input_block_finished())
            print(result);
    };

    {
        std::cout << "task1\n";
        std::stringstream input{"cmd1\n"
                                "cmd2\n"
                                "{\n"
                                "cmd3\n"
                                "cmd4\n"
                                "}\n"
                                "{\n"
                                "cmd5\n"
                                "cmd6\n"
                                "{\n"
                                "cmd7\n"
                                "cmd8\n"
                                "}\n"
                                "cmd9\n"
                                "}\n"
                                "{\n"
                                "cmd10\n"
                                "cmd11"};

        read_input<decltype(process), CmdCollector::ParseErr>(input, std::cerr, process);
        commands.finish_block();
        if(commands.input_block_finished())
            print(result);

        std::stringstream ref{"bulk: cmd1, cmd2\n"
                              "bulk: cmd3, cmd4\n"
                              "bulk: cmd5, cmd6, cmd7, cmd8, cmd9\n"};
        ASSERT_TRUE(result.str() == ref.str());
    }

    result.str("");
    commands.reset();
    {
        std::cout << "task2\n";
        std::stringstream input{"cmd1\n"
                                "cmd2\n"
                                "cmd3\n"
                                "cmd4\n"
                                "cmd5"};

        read_input<decltype(process), CmdCollector::ParseErr>(input, std::cerr, process);
        commands.finish_block();
        if(commands.input_block_finished())
            print(result);

        std::stringstream ref{"bulk: cmd1, cmd2, cmd3\n"
                              "bulk: cmd4, cmd5\n"};
        //std::cout << result.str() << '\n';
        ASSERT_TRUE(result.str() == ref.str());
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
