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

    /// \arg \b sql custom query in a form mentioned in homework #10
    /// \returns SQL
    virtual convert_result_t convert_sql(sql_t sql) override;
private:
    using ftype = sql_t (*)(const std::vector<std::string>&);

    /// \brief first is hash of custom query, second - pointer to function that returns SQL
    std::unordered_map<std::size_t, ftype> m_map;
};

}
