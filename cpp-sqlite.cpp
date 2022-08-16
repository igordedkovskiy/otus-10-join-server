#include <iostream>
#include <string>
#include <cstdarg>

#include "OtusQuery.hpp"

int main(int argc, char **argv)
{
    otus_db::OtusQuery db;
    if(!db.is_opened())
        return EXIT_FAILURE;

    std::string q;
    while(true)
    {
        std::getline(std::cin, q);
        if(q == "exit")
            return 0;
        try
        {
            const auto res{db.execute_query(q)};
            if(!res.second)
            {
                std::cout << "Error: " << db.last_error_msg() << std::endl;
            }
            else
            {
                for(const auto& line:res.first)
                {
                    for(const auto& v:line)
                        std::cout << v << "  ";
                    std::cout << std::endl;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}
