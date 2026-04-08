#ifndef LAN_CHAT_PROTOCOL_H
#define LAN_CHAT_PROTOCOL_H

#include <string>
#include <vector>

struct LanPacket {
    std::string cmd;
    std::string from;
    std::string to;
    std::string group;
    std::string payload;
};

std::string lan_pack(const LanPacket& p);
bool lan_unpack(const std::string& line, LanPacket& out);
std::vector<std::string> lan_split_csv(const std::string& s);
std::string lan_join_csv(const std::vector<std::string>& items);

#endif
