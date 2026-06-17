#include "Command.hpp"

namespace ticketsystem {

// 默认构造函数
Command::Command() : timestamp(0), type(CommandType::Unknown), param_count(0) {}

// key: 参数的键名，value: 参数的值
// 向Command对象添加一个键值对参数，最多支持16个参数
void Command::addParam(char key, const std::string &value) {
    if (param_count < 16) {
        params[param_count].key = key;
        params[param_count].value = value;
        ++param_count;
    }
}

// key: 要查找的参数键名
// 根据键名获取参数值，不存在则返回空字符串
const std::string &Command::getParam(char key) const {
    static std::string empty;
    for (int i = 0; i < param_count; ++i) {
        if (params[i].key == key)
            return params[i].value;
    }
    return empty;
}

// key: 要检查的参数键名
// 检查Command对象是否包含指定键名的参数，存在返回true，否则返回false
bool Command::hasParam(char key) const {
    for (int i = 0; i < param_count; ++i) {
        if (params[i].key == key)
            return true;
    }
    return false;
}

// name: 命令名称字符串
// 将命令名字符串转换为对应的CommandType枚举值，未知命令返回Unknown
CommandType CommandParser::toType(const std::string &name) {
    if (name == "add_user")
        return CommandType::AddUser;
    if (name == "login")
        return CommandType::Login;
    if (name == "logout")
        return CommandType::Logout;
    if (name == "query_profile")
        return CommandType::QueryProfile;
    if (name == "modify_profile")
        return CommandType::ModifyProfile;
    if (name == "add_train")
        return CommandType::AddTrain;
    if (name == "delete_train")
        return CommandType::DeleteTrain;
    if (name == "release_train")
        return CommandType::ReleaseTrain;
    if (name == "query_train")
        return CommandType::QueryTrain;
    if (name == "query_ticket")
        return CommandType::QueryTicket;
    if (name == "query_transfer")
        return CommandType::QueryTransfer;
    if (name == "buy_ticket")
        return CommandType::BuyTicket;
    if (name == "query_order")
        return CommandType::QueryOrder;
    if (name == "refund_ticket")
        return CommandType::RefundTicket;
    if (name == "clean")
        return CommandType::Clean;
    if (name == "exit")
        return CommandType::Exit;
    return CommandType::Unknown;
}

// c: 待检查的字符
// 判断字符是否为空白字符,是返回true
static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// line: 待解析的命令行字符串
// 解析命令行字符串，提取时间戳、命令类型和参数，返回Command对象
Command CommandParser::parse(const std::string &line) const {
    Command cmd;
    size_t pos = 0;
    size_t len = line.size();

    // 提取时间戳
    if (pos < len && line[pos] == '[') {
        size_t end_pos = line.find(']', pos);
        if (end_pos != std::string::npos) {
            std::string ts = line.substr(pos + 1, end_pos - pos - 1);
            cmd.timestamp = std::stoi(ts);
            pos = end_pos + 1;
        }
    }

    // 提取命令类型
    while (pos < len && isSpace(line[pos]))
        pos++;
    size_t start = pos;
    while (pos < len && !isSpace(line[pos]))
        pos++;
    if (start < pos) {
        std::string name = line.substr(start, pos - start);
        cmd.type = toType(name);
    }

    // 提取参数
    while (pos < len) {
        while (pos < len && isSpace(line[pos]))
            pos++;
        if (pos + 1 >= len || line[pos] != '-')
            break;
        char key = line[pos + 1];
        pos += 2;
        while (pos < len && isSpace(line[pos]))
            pos++;
        start = pos;
        while (pos < len && !isSpace(line[pos]))
            pos++;
        if (start < pos) {
            cmd.addParam(key, line.substr(start, pos - start));
        }
    }

    return cmd;
}

// text: 待分割的字符串，delim: 分隔符，parts: 输出向量，maxCount: 最大分割数
// 按分隔符分割字符串，结果存入向量
void splitString(const std::string &text, char delim, sjtu::vector<std::string> &parts, int maxCount) {
    parts.clear();
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == delim) {
            if ((int)parts.size() < maxCount)
                parts.push_back(current);
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    if ((int)parts.size() < maxCount)
        parts.push_back(current);
}

} // namespace ticketsystem