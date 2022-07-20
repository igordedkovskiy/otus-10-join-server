// homework #6: command processing (bulk).

#include <iostream>
#include <sstream>
#include <fstream>

#include "CmdCollector.hpp"
#include "read_input.hpp"

int main()
{
    CmdCollector commands{read_input_size(std::cin, std::cerr)};

    auto print = [&commands]()
    {
        std::stringstream fname;
        fname << "bulk" << commands.block_start_time(0) << ".log";
        std::fstream file{fname.str(), std::fstream::out | std::fstream::app};
        file << "bulk: ";
        std::cout << "bulk: ";

        std::size_t cntr = 0;
        for(const auto& cmd:commands.get_cmd())
        {
            file << cmd;
            std::cout << cmd;
            if(++cntr < commands.block_size())
            {
                file << ", ";
                std::cout << ", ";
            }

        }
        std::cout << '\n';
        commands.clear_commands();
    };

    auto process = [&commands, &print](std::string&& read)
    {
        commands.process_cmd(std::move(read));
        read.clear();
        if(commands.input_block_finished())
            print();
    };

    read_input<decltype(process), CmdCollector::ParseErr>(std::cin, std::cerr, process);
    commands.finish_block();
    if(commands.input_block_finished())
        print();
    return 0;
}
