#include "ParseErr.hpp"

ParseErr::ParseErr(std::string&& m):
    m_msg{std::move(m)}
{}

const char* ParseErr::what() const noexcept
{
    return m_msg.c_str();
}

const std::string &ParseErr::get_message() const noexcept
{
    return m_msg;
}
