#ifndef TICKET_SYSTEM_TRAIN_UTILS_HPP
#define TICKET_SYSTEM_TRAIN_UTILS_HPP

#include "Storage.hpp"
#include "DateTime.hpp"
#include "Storage.hpp"
#include "vector.hpp"

namespace ticket {

// --- 站点查找 ---

// 一次扫描同时查找 from 和 to 的索引，减少字符串比较次数
bool findFromTo(const TrainRecord &train, const std::string &from, const std::string &to,
                int &from_idx, int &to_idx);

// 单站查找
int findStation(const TrainRecord &train, const std::string &station);

// --- 时间偏移计算 ---

// 计算从始发站到指定站点的到达时间偏移（分钟）
int arrivalOffsetMinutes(const TrainRecord &train, int station_idx);

// 计算从始发站到指定站点的离开时间偏移（分钟）
int departureOffsetMinutes(const TrainRecord &train, int station_idx);

// 计算列车在指定站点的出发时间
Time departureTimeAtStation(const TrainRecord &train, int station_idx);

// --- 日期/时间计算 ---

// 根据中途某站的出发日期，反推列车起点日期
Date getRunStartDate(const TrainRecord &train, int station_idx, const Date &depart_date);

// 计算列车到达指定站点的时间
DateTime stationArrival(const TrainRecord &train, int station_idx, const Date &start_date);

// 计算列车从指定站点出发的日期时间
DateTime stationDeparture(const TrainRecord &train, int station_idx, const Date &start_date);

// --- 价格 ---

// 计算指定区间的票价
int routePrice(const TrainRecord &train, int from_idx, int to_idx);

// --- 运行日期判断 ---

// 判断列车在指定日期是否可运行
bool canRunOnDate(const TrainRecord &train, int station_idx, const Date &depart_date);

// --- 座位 ---

// 计算指定区间的可用座位数
int availableSeats(const StorageManager &storage,
                   const TrainRecord &train, const Date &start_date,
                   int from_idx, int to_idx);

// --- 中转 ---

// 查找列车在指定站点最早可行的出发时间
DateTime bestSecondDeparture(const TrainRecord &train, int station_idx, const DateTime &earliest);

// --- query_ticket 排序 ---

struct Item {
    std::string line;
    int key1;
    std::string key2;
};

// 插入排序（按key1升序，key1相同时按key2升序）
void sortItem(sjtu::vector<Item> &items);

} // namespace ticket

#endif // TICKET_SYSTEM_TRAIN_UTILS_HPP
