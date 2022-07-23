// homework #8: async command processing (bulk).

#include <chrono>
#include <thread>
#include "async.hpp"

int main()
{
    constexpr size_type bulk_size{3};
    const auto h1{connect(bulk_size)};
    const auto h2{connect(bulk_size)};

    const char* commands1[] = {"cmd1", "cmd2", "cmd3", "cmd4", "cmd5"};
    constexpr size_type num1 = sizeof(commands1) / sizeof(commands1[0]);

    const char* commands2[] = {"cmd1", "cmd2",
                               "{", "cmd3", "cmd4", "}",
                               "{", "cmd5", "cmd6", "{", "cmd7", "cmd8", "}", "cmd9", "}",
                               "{", "cmd10", "cmd11"
                              };
    constexpr size_type num2 = sizeof(commands2) / sizeof(commands2[0]);

    receive(h2, commands2, num2);
    receive(h1, commands1, num1);

    //using namespace std::chrono_literals;
    //std::this_thread::sleep_for(1000ms);

    disconnect(h1);
    disconnect(h2);
    return 0;
}
