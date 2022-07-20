// homework #8

#include <vector>
#include <string>

using handler_t = long long;
using commands_t = std::vector<std::string>;

handler_t connect(std::size_t in_size);
void disconnect(handler_t h);
void receive(handler_t h, commands_t& commands);
