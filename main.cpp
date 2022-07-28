// homework #8: async command processing (bulk).

#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include "async.hpp"


void single_thread()
{
    constexpr size_type bulk_size{3};

    const auto h1{connect(bulk_size)};
    receive(h1, "cmd1\ncmd2\ncmd3\ncmd4\ncmd5\n", 25);
    disconnect(h1);

    {
        std::cout << std::endl;
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
        disconnect(h2);
        disconnect(h1);
    }
    {
        std::cout << std::endl;
        const auto h1{connect(bulk_size)};
        receive(h1, "cmd1\n", 5);
        receive(h1, "cmd2\n", 5);
        receive(h1, "cmd3\n", 5);
        receive(h1, "cmd4\n", 5);
        receive(h1, "cmd5\n", 5);
        disconnect(h1);
    }
}

int main()
{
    std::cout << "single thread:\n";
    single_thread();
    return 0;
}
