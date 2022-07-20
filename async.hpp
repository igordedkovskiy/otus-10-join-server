#pragma once

#include <cstdint>

extern "C"
{

using size_type = std::size_t;
using handler_t = size_type;
using commands_t = char**;

handler_t connect(size_type bulk_size);
int disconnect(handler_t h);
void receive(handler_t h, commands_t commands, size_type& num_of_commands);

}
