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
//    const auto h2{connect(bulk_size)};

    //receive(h2, commands2, num2);
//    receive(h1, commands1, num1);

    receive(h1, "cmd1\ncmd2\ncmd3\ncmd4\ncmd5\n", 25);

    disconnect(h1);
//    disconnect(h2);

    using namespace std::chrono;
    const auto start1 = std::chrono::high_resolution_clock::now();
//    for(size_type cntr{0}; cntr < 10; ++cntr)
    {
        {
            std::cout << "\n\n" << std::endl;
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
            std::cout << "\n\n" << std::endl;
            const auto h1{connect(bulk_size)};
            receive(h1, "cmd1\n", 5);
            receive(h1, "cmd2\n", 5);
            receive(h1, "cmd3\n", 5);
            receive(h1, "cmd4\n", 5);
            receive(h1, "cmd5\n", 5);
            disconnect(h1);
        }
    }
    const auto passed1 = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start1).count();

//    const auto start2 = std::chrono::high_resolution_clock::now();
//    for(size_type cntr{0}; cntr < 10; ++cntr)
//    {
//        {
//            std::cout << "\n\n" << std::endl;
//            const auto h1{connect_ths(bulk_size)};
//            const auto h2{connect_ths(bulk_size)};
//            receive_ths(h2, "cmd1\n", 5);
//            receive_ths(h1, "cmd1\ncmd2\n", 10);
//            receive_ths(h2, "cmd2\n{\ncmd3\ncmd4\n}\n", 19);
//            receive_ths(h2, "{\n", 2);
//            receive_ths(h2, "cmd5\ncmd6\n{\ncmd7\ncmd8\n}\ncmd9\n}\n", 31);
//            receive_ths(h1, "cmd3\ncmd4\n", 10);
//            receive_ths(h2, "{\ncmd10\ncmd11\n", 12);
//            receive_ths(h1, "cmd5\n", 5);
//            disconnect_ths(h2);
//            disconnect_ths(h1);
//        }
//        {
//            std::cout << "\n\n" << std::endl;
//            const auto h1{connect_ths(bulk_size)};
//            receive_ths(h1, "cmd1\n", 5);
//            receive_ths(h1, "cmd2\n", 5);
//            receive_ths(h1, "cmd3\n", 5);
//            receive_ths(h1, "cmd4\n", 5);
//            receive_ths(h1, "cmd5\n", 5);
//            disconnect_ths(h1);
//        }
//    }
//    const auto passed2 = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start2).count();
    std::cout << passed1 << std::endl;
//    std::cout << passed2 << std::endl;
}

//void multiple_threads()
//{
//    const auto h1{connect(bulk_size)};
//    const auto h2{connect(bulk_size)};

//    std::thread t1{receive, h1, std::ref(commands1), num1};
//    std::thread t2{receive, h2, std::ref(commands2), num2};
//    t1.join();
//    t2.join();

//    disconnect(h1);
//    disconnect(h2);
//}

//void single_thread_gradually()
//{
//    const auto h1{connect(bulk_size)};
//    const auto h2{connect(bulk_size)};

//    receive(h2, commands2, 2);
//    receive(h1, commands1, 3);

//    receive(h2, &commands2[0] + 2, 16);
//    receive(h1, &commands1[0] + 3, 2);

//    disconnect(h1);
//    disconnect(h2);
//}

int main()
{
    std::cout << "single thread:\n";
    single_thread();
//    std::cout << "\nmultiple threads:\n";
//    multiple_threads();
//    std::cout << "\nsingle thread gradually:\n";
//    single_thread_gradually();
    return 0;
}
