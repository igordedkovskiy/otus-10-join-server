#include <iostream>
#include "retransmittor.hpp"
//#include "CmdCollector.hpp"

using namespace async_server;

Retransmittor::Retransmittor(std::string&& db_name):
    m_db{std::forward<decltype(db_name)>(db_name)}
{}

void Retransmittor::on_read(rc_data&& data)
{
//    const auto addr{std::move(data.m_endpoint)};

    using protocol_fmt_t = std::tuple<std::string, // cmd
                                      std::string, // table name if any
                                      std::string, // key if any
                                      std::string, // value if any
                                     >;

    auto preprocess = [](std::string&& data)->protocol_fmt_t
    {
        auto msg{std::forward<decltype(data)>(data)};
        protocol_fmt_t pfmt;
        auto& [cmd, table, key, value] = pfmt;
        const auto delim1{msg.find_first_of(' ')};
        if(delim1 == std::string::npos)
        {
            if(msg == "INTERSECTION")
                cmd = "INTERSECTION";
            else if(msg == "SYMMETRIC_DIFFERENCE")
                cmd = "SYMMETRIC_DIFFERENCE";
            else
                throw ParseError{"incorrect querry format"};
        }
        else
        {
            cmd = msg.substr(0, delim1);
            if(cmd == "INSERT")
            {
                const auto delim2 = addr.find_first_of(' ', delim1 + 1);
                if(delim2 == std::string::npos || delim2 == msg.size() - 1)
                    throw ParseError{"incorrect querry format"};
                table = msg.substr(delim1 + 1, delim2 - delim1 - 1);

                const auto delim3 = addr.find_first_of(' ', delim2 + 1);
                if(delim3 == std::string::npos || delim3 == msg.size() - 1)
                    throw ParseError{"incorrect querry format"};
                key = msg.substr(delim2 + 1, delim3 - delim2 - 1);

                try
                {
                    [[maybe_unused]] const auto k{static_cast<std::size_t>(stoi(key))};
                }
                catch(std::logic_error& e)
                {
                    throw ParseError{"incorrect querry format"};
                }

                const auto delim4 = addr.find_first_of(' ', delim3 + 1);
                if(delim4 != std::string::npos)
                    throw ParseError{"incorrect querry format"};
                value = msg.substr(delim3 + 1, msg.size() - delim3 - 1);
            }
            else if(cmd == "TRUNCATE")
            {
                const auto delim2 = addr.find_first_of(' ', delim1 + 1);
                if(delim2 != std::string::npos)
                    throw ParseError{"incorrect querry format"};
                table = msg.substr(delim1 + 1, msg.size() - delim1 - 1);
            }
            else
                throw ParseError{"incorrect querry format"};
        }
        return pfmt;
    };

    const auto pfmt = preprocess(std::forward<decltype(data)>(data));
    const auto key = std::get<2>(pfmt);
    auto translate = [&pfmt]()
    {
        auto& [cmd, table, key, value] = pfmt;
        std::string order{"ORDER BY id"};
        if(cmd == "INSERT")
            return std::string{"INSERT INTO " + std::move(table) +
                               " VALUES(" + std::move(key) + ", '" + std::move(value) + "')" + std::move(order) + ";"};
        else if(cmd == "TRUNCATE")
            return std::string{"TRUNCATE TABLE " + std::move(table)};
        else if(cmd == "INTERSECTION")
        {
            return std::string{"INSERT INTO " + std::move(table) +
                               " VALUES(" + std::move(key) + ", '" + std::move(value) + "')" + std::move(order) + ";"};
        }
        else if(cmd == "SYMMETRIC_DIFFERENCE")
        {
            return std::string{"INSERT INTO " + std::move(table) +
                               " VALUES(" + std::move(key) + ", '" + std::move(value) + "')" + std::move(order) + ";"};
        }
    };

    if(!m_db.execute_query(sql.c_str()))
    {
        if(m_db.last_extended_error_code() == SQLITE_CONSTRAINT_PRIMARYKEY)
            throw ParseError{"ERR duplicate " + std::move(key)};
    }
}

void Retransmittor::on_socket_close([[maybe_unused]] size_type address){}


Retransmittor::DB::DB(std::string&& name):
    m_name{std::forward<decltype(name)>(name)}
{
    if(sqlite3_open(m_name.c_str(), &m_handle))
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(m_handle) << std::endl;
        sqlite3_close(m_handle);
        m_handle = nullptr;
    }
    else
        std::cout << m_name << " database opened successfully!" << std::endl;
}

Retransmittor::DB::~DB()
{
    close();
}

void Retransmittor::DB::close()
{
    if(m_handle)
        sqlite3_close(m_handle);
}

bool Retransmittor::DB::execute_query(const std::string &sql)
{
    auto print_results = [](void *, int columns, char **data, char **names) -> int
    {
        for(int i = 0; i < columns; ++i)
            std::cout << names[i] << " = " << (data[i] ? data[i] : "NULL") << std::endl;
        std::cout << std::endl;
        return 0;
    };

    char* errmsg{nullptr};
    m_last_err_code = sqlite3_exec(m_handle, sql.c_str(), print_results, 0, &errmsg);
    m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
    if(m_last_err_code != SQLITE_OK)
    {
        std::cerr << "Can't execute query: " << errmsg << std::endl;
        std::cerr << "Error code: " << m_last_err_code << std::endl;
        m_last_ext_err_code = sqlite3_extended_errcode(m_handle);
        std::cerr << "Extended error code: " << m_last_ext_err_code << std::endl;
        if(m_last_err_code == SQLITE_CONSTRAINT)
        {
            if(m_last_ext_err_code == SQLITE_CONSTRAINT_PRIMARYKEY)
            {
                std::cerr << "ERR duplicate: ?" << std::endl;
            }
            std::cerr << sql << std::endl;
        }
        m_last_err_msg = errmsg;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

const std::string &Retransmittor::DB::last_error_msg() const
{
    return m_last_err_msg;
}

Retransmittor::DB::err_t Retransmittor::DB::last_extended_error_code() const
{
    return m_last_ext_err_code;
}

Retransmittor::DB::err_t Retransmittor::DB::last_error_code() const
{
    return m_last_err_code;
}

bool Retransmittor::DB::is_valid() const
{
    return m_handle;
}

