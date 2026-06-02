#ifndef TICKET_SYSTEM_STORAGE_HPP
#define TICKET_SYSTEM_STORAGE_HPP

#include <string>
#include <vector>
#include "Entities.hpp"
#include "bpt.hpp"
#include "MemoryRiver.hpp"

namespace ticket {

// 二进制用户记录结构体
struct BinaryUserRecord {
    char username[21];  // 用户名（最大20字符）
    char password[31];  // 密码（最大30字符）
    char name[32];      // 姓名（最大31字符）
    char mail[31];      // 邮箱（最大30字符）
    int privilege;      // 权限等级
    bool deleted;       // 是否已删除
    char padding[3];    // 对齐填充
};

// 二进制列车记录结构体
struct BinaryTrainRecord {
    char train_id[21];                 // 列车编号（最大20字符）
    int station_num;                   // 途经站数
    int seat_num;                      // 座位数
    int max_seat_num;                  // 最大座位数
    char stations[MAX_STATIONS][32];   // 途经站名称数组
    int prices[MAX_PRICE_SEGMENTS];    // 各段票价数组
    int travel_times[MAX_TRAVEL_SEGMENTS];   // 各段行驶时间数组（分钟）
    int stopover_times[MAX_TRAVEL_SEGMENTS]; // 各站停留时间数组（分钟）
    int start_hour;                    // 发车小时
    int start_minute;                  // 发车分钟
    int sale_begin_month;              // 售票开始月份
    int sale_begin_day;                // 售票开始日期
    int sale_end_month;                // 售票结束月份
    int sale_end_day;                  // 售票结束日期
    char type;                         // 列车类型
    bool released;                     // 是否已发布
    bool deleted;                      // 是否已删除
    char padding[2];                   // 对齐填充
};

// 二进制订单记录结构体
struct BinaryOrderRecord {
    int order_id;        // 订单编号
    char username[21];   // 用户名（最大20字符）
    char train_id[21];   // 列车编号（最大20字符）
    int date_month;      // 乘车月份
    int date_day;        // 乘车日期
    int from_idx;        // 出发站索引
    int to_idx;          // 到达站索引
    int num;             // 购票数量
    int price;           // 总票价
    int status;          // 订单状态（0-Success, 1-Pending, 2-Refunded）
    bool is_waiting;     // 是否为候补订单
    bool deleted;        // 是否已删除
    char padding[2];     // 对齐填充
    int timestamp;       // 创建时间戳
};

// 存储路径结构体
struct StoragePaths {
    static const char* USERS;              // 用户数据文件路径
    static const char* TRAINS;             // 列车数据文件路径
    static const char* ORDERS;             // 订单数据文件路径
    static const char* WAITLIST;           // 候补队列文件路径
    static const char* USER_INDEX;         // 用户索引文件路径
    static const char* TRAIN_INDEX;        // 列车索引文件路径
    static const char* ORDER_INDEX;        // 订单索引文件路径
    static const char* ORDER_USER_INDEX;   // 订单-用户索引文件路径
    static const char* TRAIN_STATION_INDEX; // 列车-站点索引文件路径
    static const char* ORDER_TRAIN_DATE_INDEX; // 订单-列车-日期索引文件路径
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
    bool loadOrdersByUser(const std::string& username, std::vector<OrderRecord>& orders, std::vector<int>& ids) const;
    bool loadOrdersByTrainDate(const std::string& train_id, const Date& date, std::vector<OrderRecord>& orders, std::vector<int>& ids) const;
    bool loadTrainsByStation(const std::string& station, std::vector<TrainRecord>& trains) const;
    bool loadAllTrains(std::vector<TrainRecord>& trains) const;
    bool loadAllOrders(std::vector<OrderRecord>& orders, std::vector<int>& ids) const;

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
    BPlusTree<int>* orderUserIndex_;
    BPlusTree<int>* trainStationIndex_;
    BPlusTree<int>* orderTrainDateIndex_;
    int next_order_id_;
};

} // namespace ticket

#endif // TICKET_SYSTEM_STORAGE_HPP