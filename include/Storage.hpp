#ifndef TICKET_SYSTEM_STORAGE_HPP
#define TICKET_SYSTEM_STORAGE_HPP

#include "DateTime.hpp"
#include "MemoryRiver.hpp"
#include "bpt.hpp"
#include "vector.hpp"
#include <string>

namespace ticketsystem {

static const int MAX_STATIONS = 100;
static const int MAX_PRICE_SEGMENTS = 100;
static const int MAX_TRAVEL_SEGMENTS = 100;

enum class OrderStatus {
    Success,
    Pending,
    Refunded
};

// 用户记录
struct UserRecord {
    std::string username;
    std::string password;
    std::string name;
    std::string mail;
    int privilege;

    UserRecord();
};

// 列车记录
struct TrainRecord {
    std::string train_id;
    int station_num;
    int seat_num;
    int max_seat_num;
    sjtu::vector<std::string> stations;
    sjtu::vector<int> prices;
    Time start_time;
    sjtu::vector<int> travel_times;
    sjtu::vector<int> stopover_times;
    Date sale_begin;
    Date sale_end;
    char type;
    bool released;

    TrainRecord();
};

// 订单记录
struct OrderRecord {
    int order_id;
    std::string username;
    std::string train_id;
    Date date;
    int from_idx;
    int to_idx;
    int num;
    int price;
    OrderStatus status;
    bool is_waiting;
    int timestamp;

    OrderRecord();
};

// 存储结构
struct BinaryUserRecord {
    char username[21];
    char password[31];
    char name[32];
    char mail[31];
    int privilege;
    bool deleted;
};

struct BinaryTrainRecord {
    char train_id[21];
    int station_num;
    int seat_num;
    int max_seat_num;
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
    int timestamp;
};

struct StoragePaths {
    static const char *USERS;
    static const char *TRAINS;
    static const char *ORDERS;
    static const char *USER_INDEX;
    static const char *TRAIN_INDEX;
    static const char *ORDER_INDEX;
    static const char *ORDER_USER_INDEX;
    static const char *TRAIN_STATION_INDEX;
    static const char *TRAIN_STATION_PAIR_INDEX;
    static const char *ORDER_TRAIN_DATE_INDEX;
};

class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    bool initialize(const std::string &basePath);

    bool addUser(const UserRecord &user);
    bool loadUser(const std::string &username, UserRecord &user) const;
    bool updateUser(const UserRecord &user);

    bool addTrain(const TrainRecord &train);
    bool loadTrain(const std::string &train_id, TrainRecord &train) const;
    bool updateTrain(const TrainRecord &train);
    bool removeTrain(const std::string &train_id);

    bool addOrder(const OrderRecord &order);
    bool loadOrder(int order_id, OrderRecord &order) const;
    bool updateOrder(int order_id, const OrderRecord &order);
    bool loadOrdersByUser(const std::string &username, sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids) const;
    bool loadOrdersByTrainDate(const std::string &train_id, const Date &date, sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids) const;
    bool loadTrainsByStation(const std::string &station, sjtu::vector<TrainRecord> &trains) const;
    bool loadTrainsByStationPair(const std::string &from, const std::string &to, sjtu::vector<TrainRecord> &trains) const;

    bool hasAnyUser() const;
    std::string basePath() const;

private:
    std::string base_path_;
    MemoryRiver<BinaryUserRecord, 1> userRiver_;
    MemoryRiver<BinaryTrainRecord, 1> trainRiver_;
    MemoryRiver<BinaryOrderRecord, 1> orderRiver_;
    BPlusTree<int> *userIndex_;
    BPlusTree<int> *trainIndex_;
    BPlusTree<int> *orderIndex_;
    BPlusTree<int> *orderUserIndex_;
    BPlusTree<int> *trainStationIndex_;
    BPlusTree<int> *trainStationPairIndex_;
    BPlusTree<int> *orderTrainDateIndex_;
    int next_order_id_;
};

} // namespace ticketsystem

#endif // TICKET_SYSTEM_STORAGE_HPP
