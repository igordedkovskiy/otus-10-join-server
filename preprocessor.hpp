#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "async.h"

namespace preprocess
{

using size_type = async::size_type;
using handler_t = async::handler_t;

class Preprocessor
{
public:
    enum class BlockType { STATIC, DYNAMIC };
    using where_to_cut_t = std::vector<std::pair<size_type, BlockType>>;
    where_to_cut_t run(handler_t h, const char* data, size_type data_size, bool update_state = true);

private:
    struct helper
    {
        size_type m_braces_cntr{0};
        BlockType m_type{BlockType::STATIC};
    };

    std::unordered_map<handler_t, helper> m_map;
};

void preprocess(handler_t h, const char* data, size_type data_size, );

}

