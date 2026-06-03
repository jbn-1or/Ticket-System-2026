#ifndef TICKET_SYSTEM_ENTITIES_HPP
#define TICKET_SYSTEM_ENTITIES_HPP

#include <string>
#include "vector.hpp"
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

// 用户记录
struct UserRecord {
    std::string username;  // 用户名
    std::string password;  // 密码
    std::string name;  // 姓名
    std::string mail;  // 邮箱
    int privilege;  // 权限等级

    UserRecord();
};

// 列车记录
struct TrainRecord {
    std::string train_id; // 列车编号
    int station_num;  // 途经站数
    int seat_num;   // 座位数
    int max_seat_num; // 最大座位数
    sjtu::vector<std::string> stations; // 途经站名称列表
    sjtu::vector<int> prices;  // 各段票价
    Time start_time;  // 发车时间
    sjtu::vector<int> travel_times;  // 各段行驶时间（分钟）
    sjtu::vector<int> stopover_times;  // 各站停留时间（分钟）
    Date sale_begin; // 售票开始日期
    Date sale_end; // 售票结束日期
    char type; // 列车类型（G/D/T/K）
    bool released; // 是否已发布

    TrainRecord();
};

// 订单记录
struct OrderRecord {
    int order_id; // 订单编号
    std::string username;  // 用户名
    std::string train_id;  // 列车编号
    Date date;  // 乘车日期
    int from_idx; // 出发站索引
    int to_idx;   // 到达站索引
    int num;  // 购票数量
    int price; // 总票价
    OrderStatus status; // 订单状态
    bool is_waiting;  // 是否为候补订单
    int timestamp;  // 创建时间戳

    OrderRecord();
};

// 候补记录
struct WaitlistRecord {
    std::string username;   // 用户名
    std::string train_id;   // 列车编号
    Date date;   // 乘车日期
    int from_idx;   // 出发站索引
    int to_idx;     // 到达站索引
    int num;    // 候补数量
    int timestamp;  // 创建时间戳

    WaitlistRecord();
};

} // namespace ticket

#endif // TICKET_SYSTEM_ENTITIES_HPP