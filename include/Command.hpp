#ifndef TICKET_SYSTEM_COMMAND_HPP
#define TICKET_SYSTEM_COMMAND_HPP

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

struct CommandParam {
    char key;
    std::string value;
};

struct Command {
    int timestamp;
    CommandType type;
    CommandParam params[16];
    int param_count;

    Command();
    void clear();
    void addParam(char key, const std::string& value);
    const std::string& getParam(char key) const;
    bool hasParam(char key) const;
};

class CommandParser {
public:
    Command parse(const std::string& line) const;
    static CommandType toType(const std::string& name);
};

} // namespace ticket

#endif // TICKET_SYSTEM_COMMAND_HPP