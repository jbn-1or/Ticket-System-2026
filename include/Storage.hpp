#ifndef TICKET_SYSTEM_STORAGE_HPP
#define TICKET_SYSTEM_STORAGE_HPP

#include <string>
#include "Entities.hpp"
#include "bpt.hpp"
#include "MemoryRiver.hpp"

namespace ticket {

struct BinaryUserRecord {
    char username[21];
    char password[31];
    char name[32];
    char mail[31];
    int privilege;
    bool deleted;
    char padding[3];
};

struct BinaryTrainRecord {
    char train_id[21];
    int station_num;
    int seat_num;
    char stations[MAX_STATIONS][32];
    int prices[MAX_PRICE_SEGMENTS];
    int travel_times[MAX_TRAVEL_SEGMENTS];
    int stopover_times[MAX_TRAVEL_SEGMENTS];
    int start_hour;
    int start_minute;
    int sale_begin_month;
    int sale_begin_day;
    int sale_end_month;
    int sale_end_day;
    char type;
    bool released;
    bool deleted;
    char padding[2];
};

struct BinaryOrderRecord {
    int order_id;
    char username[21];
    char train_id[21];
    int date_month;
    int date_day;
    int from_idx;
    int to_idx;
    int num;
    int price;
    int status;
    bool is_waiting;
    bool deleted;
    char padding[2];
    int timestamp;
};

struct StoragePaths {
    static const char* USERS;
    static const char* TRAINS;
    static const char* ORDERS;
    static const char* WAITLIST;
    static const char* USER_INDEX;
    static const char* TRAIN_INDEX;
    static const char* ORDER_INDEX;
};

class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool initialize(const std::string& basePath);

    bool addUser(const UserRecord& user);
    bool loadUser(const std::string& username, UserRecord& user) const;
    bool updateUser(const UserRecord& user);

    bool addTrain(const TrainRecord& train);
    bool loadTrain(const std::string& train_id, TrainRecord& train) const;
    bool updateTrain(const TrainRecord& train);
    bool removeTrain(const std::string& train_id);

    bool addOrder(const OrderRecord& order);
    bool loadOrder(int order_id, OrderRecord& order) const;
    bool updateOrder(int order_id, const OrderRecord& order);
    bool loadAllTrains(TrainRecord trains[], int& count) const;
    bool loadAllOrders(OrderRecord orders[], int ids[], int& count) const;

    bool enqueueWaitlist(const WaitlistRecord& wait);
    bool loadNextWaitlist(WaitlistRecord& wait) const;

    // helper
    bool hasAnyUser() const;
    std::string basePath() const;

private:
    std::string base_path_;
    MemoryRiver<BinaryUserRecord, 1> userRiver_;
    MemoryRiver<BinaryTrainRecord, 1> trainRiver_;
    MemoryRiver<BinaryOrderRecord, 1> orderRiver_;
    BPlusTree<int>* userIndex_;
    BPlusTree<int>* trainIndex_;
    BPlusTree<int>* orderIndex_;
    int next_order_id_;
};

} // namespace ticket

#endif // TICKET_SYSTEM_STORAGE_HPP