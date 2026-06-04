#ifndef TICKET_SYSTEM_COMMAND_HPP
#define TICKET_SYSTEM_COMMAND_HPP

#include "vector.hpp"
#include <string>

namespace ticket {

enum class CommandType {
    AddUser,
    Login,
    Logout,
    QueryProfile,
    ModifyProfile,
    AddTrain,
    DeleteTrain,
    ReleaseTrain,
    QueryTrain,
    QueryTicket,
    QueryTransfer,
    BuyTicket,
    QueryOrder,
    RefundTicket,
    Clean,
    Exit,
    Unknown
};

// 命令参数结构体，存储单个键值对参数
struct CommandParam {
    char key;          // 参数键
    std::string value; // 参数值
};

// 命令结构体，存储解析后的命令信息
struct Command {
    int timestamp;           // 时间戳
    CommandType type;        // 命令类型
    CommandParam params[16]; // 参数数组（最多16个）
    int param_count;         // 实际参数数量

    Command();
    void clear();
    void addParam(char key, const std::string &value);
    const std::string &getParam(char key) const;
    bool hasParam(char key) const;
};

class CommandParser {
public:
    Command parse(const std::string &line) const;
    static CommandType toType(const std::string &name);
};

// 按分隔符分割字符串
void splitString(const std::string &text, char delim, sjtu::vector<std::string> &parts, int maxCount);

} // namespace ticket

#endif // TICKET_SYSTEM_COMMAND_HPP