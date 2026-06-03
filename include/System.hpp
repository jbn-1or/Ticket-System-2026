#ifndef TICKET_SYSTEM_SYSTEM_HPP
#define TICKET_SYSTEM_SYSTEM_HPP

#include <string>
#include <cstring>
#include <map.hpp>
#include "Command.hpp"
#include "Storage.hpp"

namespace ticket {

// P0: 区间占用缓存 key —— (train_id, date)
struct OccupancyKey {
    char train_id[24];
    int month;
    int day;

    bool operator<(const OccupancyKey& other) const {
        int cmp = std::strcmp(train_id, other.train_id);
        if (cmp != 0) return cmp < 0;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
};

// P0: 区间占用缓存 value —— 各段已售座位数
struct OccupancyValue {
    int station_num;
    int occupancy[100];  // occupancy[i] = 第 i 段（station i → i+1）已售座位数
};

class TicketSystem {
public:
    TicketSystem();
    ~TicketSystem();

    bool initialize(const std::string& dataPath);
    std::string execute(const Command& command);

    // P0: 对外暴露缓存访问接口（供静态辅助函数使用）
    const OccupancyValue* getSegmentOccupancy(
        const StorageManager& storage, const TrainRecord& train, const Date& start_date);
    void updateSegmentOccupancy(
        const TrainRecord& train, const Date& start_date,
        int from_idx, int to_idx, int delta);
    void clearSegmentCache();

private:
    bool checkLogin(const std::string& username) const;
    int privilegeOf(const std::string& username) const;

    // logged-in users
    sjtu::map<std::string, bool> logged_users;

    std::string handleAddUser(const Command& command);
    std::string handleLogin(const Command& command);
    std::string handleLogout(const Command& command);
    std::string handleQueryProfile(const Command& command);
    std::string handleModifyProfile(const Command& command);
    std::string handleAddTrain(const Command& command);
    std::string handleDeleteTrain(const Command& command);
    std::string handleReleaseTrain(const Command& command);
    std::string handleQueryTrain(const Command& command);
    std::string handleQueryTicket(const Command& command);
    std::string handleQueryTransfer(const Command& command);
    std::string handleBuyTicket(const Command& command);
    std::string handleQueryOrder(const Command& command);
    std::string handleRefundTicket(const Command& command);
    std::string handleClean(const Command& command);
    std::string handleExit(const Command& command);

private:
    StorageManager storage_;

    // P0: 区间占用缓存
    sjtu::map<OccupancyKey, OccupancyValue> segment_cache_;
};

} // namespace ticket

#endif // TICKET_SYSTEM_SYSTEM_HPP