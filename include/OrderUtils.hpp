#ifndef TICKET_SYSTEM_ORDER_UTILS_HPP
#define TICKET_SYSTEM_ORDER_UTILS_HPP

#include "Storage.hpp"
#include "vector.hpp"

namespace ticketsystem {

class TicketSystem;
class StorageManager;

// 将订单状态转换为对应的字符串
std::string orderStatusString(OrderStatus status);

// 按时间戳对订单数组进行原地排序
void sortOrdersByTimestamp(sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, bool descending);

// 加载用户的所有订单并按时间戳排序
bool loadUserOrders(const StorageManager &storage, const std::string &username,
                    sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, bool descending = true);

// 加载指定列车指定日期的所有待处理订单
bool loadPendingOrders(const StorageManager &storage,
                       const TrainRecord &train, const Date &start_date,
                       sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, int &count);

// 处理候补订单队列，将有座位的候补订单转为成功状态
void processWaitlist(TicketSystem &sys, StorageManager &storage,
                     const TrainRecord &train, const Date &start_date);

} // namespace ticketsystem

#endif // TICKET_SYSTEM_ORDER_UTILS_HPP
