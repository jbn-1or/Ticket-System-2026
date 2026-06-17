#include "OrderUtils.hpp"
#include "System.hpp"
#include "TrainUtils.hpp"

namespace ticketsystem {

// 订单状态
std::string orderStatusString(OrderStatus status) {
    if (status == OrderStatus::Success)
        return "success";
    if (status == OrderStatus::Pending)
        return "pending";
    return "refunded";
}

// 订单排序
// 按时间戳对索引数组排序
static void sortIdx(sjtu::vector<int> &idx, const sjtu::vector<OrderRecord> &orders,
                    const sjtu::vector<int> &ids, bool descending) {
    for (size_t i = 1; i < idx.size(); ++i) {
        int tmp = idx[i];
        size_t j = i;
        while (j > 0) {
            int prev = idx[j - 1];
            bool needSwap;
            if (orders[prev].timestamp != orders[tmp].timestamp) {
                needSwap = descending ? orders[prev].timestamp < orders[tmp].timestamp
                                      : orders[prev].timestamp > orders[tmp].timestamp;
            } else {
                needSwap = ids[prev] > ids[tmp];
            }
            if (!needSwap)
                break;
            idx[j] = idx[j - 1];
            --j;
        }
        idx[j] = tmp;
    }
}

void sortOrdersByTimestamp(sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, bool descending) {
    int count = static_cast<int>(orders.size());
    if (count <= 1)
        return;
    sjtu::vector<int> idx;
    for (int i = 0; i < count; ++i)
        idx.push_back(i);
    sortIdx(idx, orders, ids, descending);
    sjtu::vector<char> done;
    for (int i = 0; i < count; ++i)
        done.push_back(0);
    for (int i = 0; i < count; ++i) {
        if (done[i] || idx[i] == i)
            continue;
        int j = i;
        OrderRecord tmpOrder = orders[j];
        int tmpId = ids[j];
        while (!done[j]) {
            done[j] = true;
            int next = idx[j];
            if (done[next])
                break;
            orders[j] = orders[next];
            ids[j] = ids[next];
            j = next;
        }
        orders[j] = tmpOrder;
        ids[j] = tmpId;
        done[j] = true;
    }
}

// 加载用户订单
bool loadUserOrders(const StorageManager &storage, const std::string &username,
                    sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, bool descending) {
    if (!storage.loadOrdersByUser(username, orders, ids))
        return false;
    sortOrdersByTimestamp(orders, ids, descending);
    return true;
}

// 加载待处理订单
bool loadPendingOrders(const StorageManager &storage,
                       const TrainRecord &train, const Date &start_date,
                       sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids, int &count) {
    if (!storage.loadOrdersByTrainDate(train.train_id, start_date, orders, ids))
        return false;
    sjtu::vector<OrderRecord> filteredOrders;
    sjtu::vector<int> filteredIds;
    for (size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].train_id != train.train_id)
            continue;
        if (orders[i].status != OrderStatus::Pending)
            continue;
        Date orderStart = getRunStartDate(train, orders[i].from_idx, orders[i].date);
        if (!(orderStart == start_date))
            continue;
        filteredOrders.push_back(orders[i]);
        filteredIds.push_back(ids[i]);
    }
    orders = std::move(filteredOrders);
    ids = std::move(filteredIds);
    count = static_cast<int>(orders.size());
    sortOrdersByTimestamp(orders, ids, false);
    return true;
}

// 候补处理
void processWaitlist(TicketSystem &sys, StorageManager &storage,
                     const TrainRecord &train, const Date &start_date) {
    sjtu::vector<OrderRecord> orders;
    sjtu::vector<int> ids;
    int count = 0;
    if (!loadPendingOrders(storage, train, start_date, orders, ids, count)) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        int seats = availableSeats(storage, train, start_date, orders[i].from_idx, orders[i].to_idx);
        if (seats >= orders[i].num) {
            orders[i].status = OrderStatus::Success;
            orders[i].is_waiting = false;
            storage.updateOrder(ids[i], orders[i]);
        }
    }
}

} // namespace ticketsystem
