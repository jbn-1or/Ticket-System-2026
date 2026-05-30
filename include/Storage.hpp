#ifndef TICKET_SYSTEM_STORAGE_HPP
#define TICKET_SYSTEM_STORAGE_HPP

#include <string>
#include "Entities.hpp"

namespace ticket {

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

    bool addOrder(const OrderRecord& order);
    bool loadOrder(int order_id, OrderRecord& order) const;
    bool updateOrder(const OrderRecord& order);

    bool enqueueWaitlist(const WaitlistRecord& wait);
    bool loadNextWaitlist(WaitlistRecord& wait) const;
};

} // namespace ticket

#endif // TICKET_SYSTEM_STORAGE_HPP