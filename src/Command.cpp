#include "../include/Command.hpp"

namespace ticket {

Command::Command() : timestamp(0), type(CommandType::Unknown), param_count(0) {}

void Command::clear() {
    timestamp = 0;
    type = CommandType::Unknown;
    param_count = 0;
}

void Command::addParam(char key, const std::string& value) {
    if (param_count < 16) {
        params[param_count].key = key;
        params[param_count].value = value;
        ++param_count;
    }
}

const std::string& Command::getParam(char key) const {
    static std::string empty;
    for (int i = 0; i < param_count; ++i) {
        if (params[i].key == key) return params[i].value;
    }
    return empty;
}

bool Command::hasParam(char key) const {
    for (int i = 0; i < param_count; ++i) {
        if (params[i].key == key) return true;
    }
    return false;
}

CommandType CommandParser::toType(const std::string& name) {
    if (name == "add_user") return CommandType::AddUser;
    if (name == "login") return CommandType::Login;
    if (name == "logout") return CommandType::Logout;
    if (name == "query_profile") return CommandType::QueryProfile;
    if (name == "modify_profile") return CommandType::ModifyProfile;
    if (name == "add_train") return CommandType::AddTrain;
    if (name == "delete_train") return CommandType::DeleteTrain;
    if (name == "release_train") return CommandType::ReleaseTrain;
    if (name == "query_train") return CommandType::QueryTrain;
    if (name == "query_ticket") return CommandType::QueryTicket;
    if (name == "query_transfer") return CommandType::QueryTransfer;
    if (name == "buy_ticket") return CommandType::BuyTicket;
    if (name == "query_order") return CommandType::QueryOrder;
    if (name == "refund_ticket") return CommandType::RefundTicket;
    if (name == "clean") return CommandType::Clean;
    if (name == "exit") return CommandType::Exit;
    return CommandType::Unknown;
}

static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

Command CommandParser::parse(const std::string& line) const {
    Command cmd;
    size_t pos = 0;
    size_t len = line.size();

    if (pos < len && line[pos] == '[') {
        size_t end_pos = line.find(']', pos);
        if (end_pos != std::string::npos) {
            std::string ts = line.substr(pos + 1, end_pos - pos - 1);
            cmd.timestamp = std::stoi(ts);
            pos = end_pos + 1;
        }
    }

    while (pos < len && isSpace(line[pos])) pos++;
    size_t start = pos;
    while (pos < len && !isSpace(line[pos])) pos++;
    if (start < pos) {
        std::string name = line.substr(start, pos - start);
        cmd.type = toType(name);
    }

    while (pos < len) {
        while (pos < len && isSpace(line[pos])) pos++;
        if (pos + 1 >= len || line[pos] != '-') break;
        char key = line[pos + 1];
        pos += 2;
        while (pos < len && isSpace(line[pos])) pos++;
        start = pos;
        while (pos < len && !isSpace(line[pos])) pos++;
        if (start < pos) {
            cmd.addParam(key, line.substr(start, pos - start));
        }
    }

    return cmd;
}

} // namespace ticket

