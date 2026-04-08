#include "lan_chat_protocol.h"

static std::string sanitize(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c == '|' || c == '\n' || c == '\r') c = ' ';
    }
    return out;
}

std::string lan_pack(const LanPacket& p) {
    return sanitize(p.cmd) + "|" + sanitize(p.from) + "|" + sanitize(p.to) + "|" +
           sanitize(p.group) + "|" + sanitize(p.payload) + "\n";
}

bool lan_unpack(const std::string& line, LanPacket& out) {
    size_t p1 = line.find('|');
    if (p1 == std::string::npos) return false;
    size_t p2 = line.find('|', p1 + 1);
    if (p2 == std::string::npos) return false;
    size_t p3 = line.find('|', p2 + 1);
    if (p3 == std::string::npos) return false;
    size_t p4 = line.find('|', p3 + 1);
    if (p4 == std::string::npos) return false;
    out.cmd = line.substr(0, p1);
    out.from = line.substr(p1 + 1, p2 - p1 - 1);
    out.to = line.substr(p2 + 1, p3 - p2 - 1);
    out.group = line.substr(p3 + 1, p4 - p3 - 1);
    out.payload = line.substr(p4 + 1);
    while (!out.payload.empty() && (out.payload.back() == '\n' || out.payload.back() == '\r')) out.payload.pop_back();
    return true;
}

std::vector<std::string> lan_split_csv(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= s.size()) {
        size_t pos = s.find(',', start);
        if (pos == std::string::npos) {
            if (start < s.size()) out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

std::string lan_join_csv(const std::vector<std::string>& items) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out += ",";
        out += items[i];
    }
    return out;
}
