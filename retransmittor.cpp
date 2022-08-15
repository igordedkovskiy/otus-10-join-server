#include <iostream>
#include "retransmittor.hpp"

using namespace async_server;

Retransmittor::read_t Retransmittor::on_read(rc_data&& data)
{
    const auto& [table, res]{m_db.execute_query(std::move(data.m_data))};
    std::string msg;
    //for(const auto& row:table)
    for(std::size_t cntr{1}; cntr < table.size(); ++cntr)
    {
        auto& row{table[cntr]};
        for(const auto& cell:row)
            msg += std::move(cell) + ' ';
        msg += '\n';
    }
    return std::make_pair(std::move(msg), res);
}

void Retransmittor::on_socket_close([[maybe_unused]] size_type address)
{
    ;
}

