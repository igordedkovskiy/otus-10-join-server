#pragma once

#include <string>
#include <cstdarg>
#include <exception>

class ParseErr: public std::exception
{
public:
    ParseErr(std::string&& m);
    const char* what() const noexcept override;
    const std::string& get_message() const noexcept;
private:
    std::string m_msg;
};
