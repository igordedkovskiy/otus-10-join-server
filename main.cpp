// homework #8: async command processing (bulk).

#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include "async.hpp"

constexpr size_type bulk_size{3};
const char* commands1[]{"cmd1", "cmd2", "cmd3", "cmd4", "cmd5"};
constexpr size_type num1{sizeof(commands1) / sizeof(commands1[0])};
const char* commands2[]{"cmd1", "cmd2",
                        "{", "cmd3", "cmd4", "}",
                        "{", "cmd5", "cmd6", "{", "cmd7", "cmd8", "}", "cmd9", "}",
                        "{", "cmd10", "cmd11"
                       };
constexpr size_type num2{sizeof(commands2) / sizeof(commands2[0])};

void single_thread()
{
    const auto h1{connect(bulk_size)};
    const auto h2{connect(bulk_size)};

    receive(h2, commands2, num2);
    receive(h1, commands1, num1);

    disconnect(h1);
    disconnect(h2);
}

void multiple_threads()
{
    const auto h1{connect(bulk_size)};
    const auto h2{connect(bulk_size)};

    std::thread t1{receive, h1, std::ref(commands1), num1};
    std::thread t2{receive, h2, std::ref(commands2), num2};
    t1.join();
    t2.join();

    disconnect(h1);
    disconnect(h2);
}

void single_thread_gradually()
{
    const auto h1{connect(bulk_size)};
    const auto h2{connect(bulk_size)};

    receive(h2, commands2, 2);
    receive(h1, commands1, 3);

    receive(h2, &commands2[0] + 2, 16);
    receive(h1, &commands1[0] + 3, 2);

    disconnect(h1);
    disconnect(h2);
}

int main()
{
    std::cout << "single thread:\n";
    single_thread();
    std::cout << "\nmultiple threads:\n";
    multiple_threads();
    std::cout << "\nsingle thread gradually:\n";
    single_thread_gradually();
    return 0;
}
