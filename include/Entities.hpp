#ifndef TICKET_SYSTEM_ENTITIES_HPP
#define TICKET_SYSTEM_ENTITIES_HPP

#include <string>
#include <vector>
#include "DateTime.hpp"

namespace ticket {

static const int MAX_STATIONS = 100;
static const int MAX_PRICE_SEGMENTS = 100;
static const int MAX_TRAVEL_SEGMENTS = 100;

enum class OrderStatus {
    Success,
    Pending,
    Refunded
};

struct UserRecord {
    std::string username;
    std::string password;
    std::string name;
    std::string mail;
    int privilege;

    UserRecord();
};

struct TrainRecord {
    std::string train_id;
    int station_num;
    int seat_num;
    std::vector<std::string> stations;
    std::vector<int> prices;
    Time start_time;
    std::vector<int> travel_times;
    std::vector<int> stopover_times;
    Date sale_begin;
    Date sale_end;
    char type;
    bool released;

    TrainRecord();
};

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

struct WaitlistRecord {
    std::string username;
    std::string train_id;
    Date date;
    int from_idx;
    int to_idx;
    int num;
    int timestamp;

    WaitlistRecord();
};

} // namespace ticket

#endif // TICKET_SYSTEM_ENTITIES_HPP