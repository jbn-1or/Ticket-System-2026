#ifndef TICKET_SYSTEM_SYSTEM_HPP
#define TICKET_SYSTEM_SYSTEM_HPP

#include <string>
#include "Command.hpp"
#include "Entities.hpp"
#include "Storage.hpp"

namespace ticket {

class TicketSystem {
public:
    TicketSystem();
    ~TicketSystem();

    bool initialize(const std::string& dataPath);
    std::string execute(const Command& command);

private:
    bool checkLogin(const std::string& username) const;
    int privilegeOf(const std::string& username) const;

    void handleAddUser(const Command& command);
    void handleLogin(const Command& command);
    void handleLogout(const Command& command);
    void handleQueryProfile(const Command& command);
    void handleModifyProfile(const Command& command);
    void handleAddTrain(const Command& command);
    void handleDeleteTrain(const Command& command);
    void handleReleaseTrain(const Command& command);
    void handleQueryTrain(const Command& command);
    void handleQueryTicket(const Command& command);
    void handleQueryTransfer(const Command& command);
    void handleBuyTicket(const Command& command);
    void handleQueryOrder(const Command& command);
    void handleRefundTicket(const Command& command);
    void handleClean(const Command& command);
    void handleExit(const Command& command);

private:
    StorageManager storage_;
};

} // namespace ticket

#endif // TICKET_SYSTEM_SYSTEM_HPP