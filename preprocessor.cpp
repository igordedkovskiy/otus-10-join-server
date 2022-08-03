#include <iostream>
#include <sstream>
#include <fstream>

#include "preprocessor.hpp"

namespace preprocess
{

Preprocessor::where_to_cut_t Preprocessor::run(handler_t h, const char *data, size_type data_size, bool update_state)
{
    auto el{m_map.find(h)};
    if(el == std::end(m_map))
    {
        auto e{m_map.insert(std::make_pair(h, helper{}))};
        if(!e.second)
            throw;
        el = e.first;
    }

    auto properties{el->second};
    auto& [braces_cntr, curent_type]{properties};

    where_to_cut_t result;

    for(size_type cntr{0}; cntr < data_size; ++cntr)
    {
        if(data[cntr] == '{')
        {
            if(braces_cntr == 0 && cntr > 1)
                result.emplace_back(std::make_pair(cntr, BlockType::STATIC));

            ++braces_cntr;
            curent_type = BlockType::DYNAMIC;
        }
        else if(data[cntr] == '}')
        {
            if(braces_cntr == 0)
                throw;
            if(--braces_cntr == 0)
            {
                curent_type = BlockType::STATIC;
                result.emplace_back(std::make_pair(cntr + 1, BlockType::DYNAMIC));
            }
        }
    }

    if(update_state)
        el->second = properties;
    if(result.empty())
        result.emplace_back(std::make_pair(data_size, curent_type));
    return result;
}

}
