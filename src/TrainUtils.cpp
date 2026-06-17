#include "TrainUtils.hpp"

namespace ticketsystem {

// 站点查找
bool findFromTo(const TrainRecord &train, const std::string &from, const std::string &to,
                int &from_idx, int &to_idx) {
    from_idx = -1;
    to_idx = -1;
    for (int i = 0; i < train.station_num; ++i) {
        if (train.stations[i] == from) {
            from_idx = i;
            if (to_idx >= 0)
                break;
        }
        if (train.stations[i] == to) {
            to_idx = i;
            if (from_idx >= 0)
                break;
        }
    }
    if (from_idx < 0)
        return false;
    return to_idx > from_idx;
}

int findStation(const TrainRecord &train, const std::string &station) {
    for (int i = 0; i < train.station_num; ++i) {
        if (train.stations[i] == station)
            return i;
    }
    return -1;
}

// 时间偏移计算
int arrivalOffsetMinutes(const TrainRecord &train, int station_idx) {
    if (station_idx == 0)
        return 0;
    int minutes = 0;
    for (int i = 0; i < station_idx; ++i) {
        minutes += train.travel_times[i];
        if (i + 1 < station_idx) {
            minutes += train.stopover_times[i];
        }
    }
    return minutes;
}

int departureOffsetMinutes(const TrainRecord &train, int station_idx) {
    int minutes = arrivalOffsetMinutes(train, station_idx);
    if (station_idx > 0 && station_idx < train.station_num - 1) {
        minutes += train.stopover_times[station_idx - 1];
    }
    return minutes;
}

Time departureTimeAtStation(const TrainRecord &train, int station_idx) {
    int total = train.start_time.hour * 60 + train.start_time.minute + departureOffsetMinutes(train, station_idx);
    total %= 1440;
    if (total < 0)
        total += 1440;
    return Time(total / 60, total % 60);
}

// 日期/时间计算
Date getRunStartDate(const TrainRecord &train, int station_idx, const Date &depart_date) {
    int offset = departureOffsetMinutes(train, station_idx);
    DateTime current(Date(depart_date.month, depart_date.day), departureTimeAtStation(train, station_idx));
    DateTime start = addMinutes(current, -offset);
    return start.date;
}

DateTime stationArrival(const TrainRecord &train, int station_idx, const Date &start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, arrivalOffsetMinutes(train, station_idx));
}

DateTime stationDeparture(const TrainRecord &train, int station_idx, const Date &start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, departureOffsetMinutes(train, station_idx));
}

// 价格
int routePrice(const TrainRecord &train, int from_idx, int to_idx) {
    int price = 0;
    for (int i = from_idx; i < to_idx; ++i)
        price += train.prices[i];
    return price;
}

// 运行日期判断
static bool dateInRange(const Date &begin, const Date &end, const Date &target) {
    if (target < begin)
        return false;
    if (end < target)
        return false;
    return true;
}

bool canRunOnDate(const TrainRecord &train, int station_idx, const Date &depart_date) {
    Date start_date = getRunStartDate(train, station_idx, depart_date);
    return dateInRange(train.sale_begin, train.sale_end, start_date);
}

// 座位
int availableSeats(const StorageManager &storage,
                   const TrainRecord &train, const Date &start_date,
                   int from_idx, int to_idx) {
    int occupancy[100] = {0};
    sjtu::vector<OrderRecord> orders;
    sjtu::vector<int> ids;
    if (storage.loadOrdersByTrainDate(train.train_id, start_date, orders, ids)) {
        for (size_t i = 0; i < orders.size(); ++i) {
            OrderRecord &order = orders[i];
            if (order.status != OrderStatus::Success)
                continue;
            Date orderStart = getRunStartDate(train, order.from_idx, order.date);
            if (!(orderStart == start_date))
                continue;
            for (int s = order.from_idx; s < order.to_idx; ++s) {
                if (s >= 0 && s < train.station_num - 1)
                    occupancy[s] += order.num;
            }
        }
    }
    int available = train.seat_num;
    for (int i = from_idx; i < to_idx; ++i) {
        int remain = train.seat_num - occupancy[i];
        if (remain < available)
            available = remain;
    }
    return available;
}

// 中转
DateTime bestSecondDeparture(const TrainRecord &train, int station_idx, const DateTime &earliest) {
    Date candidate = getRunStartDate(train, station_idx, earliest.date);
    if (candidate < train.sale_begin)
        candidate = train.sale_begin;
    while (dateInRange(train.sale_begin, train.sale_end, candidate)) {
        DateTime depart = stationDeparture(train, station_idx, candidate);
        if (!(depart < earliest))
            return depart;
        candidate = addDays(candidate, 1);
    }
    return DateTime(Date(0, 0), Time(0, 0));
}

void sortItem(sjtu::vector<Item> &items) {
    for (size_t i = 1; i < items.size(); ++i) {
        Item tmp = items[i];
        size_t j = i;
        while (j > 0 && (items[j - 1].key1 > tmp.key1 ||
                         (items[j - 1].key1 == tmp.key1 && items[j - 1].key2 > tmp.key2))) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = tmp;
    }
}

} // namespace ticketsystem
