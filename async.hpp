#pragma once

#include <cstdint>

extern "C"
{

using size_type = std::size_t;
using handler_t = size_type;
using commands_t = const char**;

/// \brief   Create a new context
/// \arg \a  bulk_size is a size of a bulk (block of commands)
/// \returns a handler to context
handler_t connect(size_type bulk_size);

int disconnect(handler_t h);

/// \brief Transfer commands to a context
void receive(handler_t h, commands_t commands, size_type num_of_commands);

}
