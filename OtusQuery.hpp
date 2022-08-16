#pragma once

#include <string>
#include <unordered_map>

#include "QueryConverter.hpp"

namespace otus_db
{

class OtusQuery: public QueryConverter
{
public:
    OtusQuery();

    virtual convert_result_t convert_sql(sql_t sql) override;
private:
    using ftype = sql_t (*)(const std::vector<std::string>&);
    std::unordered_map<std::size_t, ftype> m_map;
};

}
