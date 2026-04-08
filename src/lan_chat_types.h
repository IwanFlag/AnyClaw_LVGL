#ifndef LAN_CHAT_TYPES_H
#define LAN_CHAT_TYPES_H

#include <string>

enum class LanChatEventType {
    System,
    GlobalMessage,
    PrivateMessage,
    GroupMessage,
    UserList,
    GroupList
};

struct LanChatEvent {
    LanChatEventType type = LanChatEventType::System;
    std::string from;
    std::string target;
    std::string text;
};

#endif
