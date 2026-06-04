#ifndef TICKET_SYSTEM_SYSTEM_HPP
#define TICKET_SYSTEM_SYSTEM_HPP

#include "Command.hpp"
#include "Storage.hpp"
#include <cstring>
#include <map.hpp>
#include <string>

namespace ticket {

class TicketSystem {
public:
    TicketSystem();
    ~TicketSystem();

    bool initialize(const std::string &dataPath);
    std::string execute(const Command &command);

private:
    bool checkLogin(const std::string &username) const;
    int privilegeOf(const std::string &username) const;

    // logged-in users
    sjtu::map<std::string, bool> logged_users;

    std::string handleAddUser(const Command &command);
    std::string handleLogin(const Command &command);
    std::string handleLogout(const Command &command);
    std::string handleQueryProfile(const Command &command);
    std::string handleModifyProfile(const Command &command);
    std::string handleAddTrain(const Command &command);
    std::string handleDeleteTrain(const Command &command);
    std::string handleReleaseTrain(const Command &command);
    std::string handleQueryTrain(const Command &command);
    std::string handleQueryTicket(const Command &command);
    std::string handleQueryTransfer(const Command &command);
    std::string handleBuyTicket(const Command &command);
    std::string handleQueryOrder(const Command &command);
    std::string handleRefundTicket(const Command &command);
    std::string handleClean(const Command &command);
    std::string handleExit(const Command &command);

private:
    StorageManager storage_;
};

} // namespace ticket

#endif // TICKET_SYSTEM_SYSTEM_HPP