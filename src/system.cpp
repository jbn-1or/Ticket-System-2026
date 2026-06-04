#include "System.hpp"
#include "TrainUtils.hpp"
#include "OrderUtils.hpp"
#include "Command.hpp"
#include <cstring>
#include <filesystem>

namespace ticket {

// ==================== TicketSystem 基础方法 ====================

TicketSystem::TicketSystem() {}

TicketSystem::~TicketSystem() {}

bool TicketSystem::initialize(const std::string &dataPath) {
    return storage_.initialize(dataPath);
}

std::string TicketSystem::execute(const Command &command) {
    switch (command.type) {
    case CommandType::AddUser:
        return handleAddUser(command);
    case CommandType::Login:
        return handleLogin(command);
    case CommandType::Logout:
        return handleLogout(command);
    case CommandType::QueryProfile:
        return handleQueryProfile(command);
    case CommandType::ModifyProfile:
        return handleModifyProfile(command);
    case CommandType::AddTrain:
        return handleAddTrain(command);
    case CommandType::DeleteTrain:
        return handleDeleteTrain(command);
    case CommandType::ReleaseTrain:
        return handleReleaseTrain(command);
    case CommandType::QueryTrain:
        return handleQueryTrain(command);
    case CommandType::QueryTicket:
        return handleQueryTicket(command);
    case CommandType::QueryTransfer:
        return handleQueryTransfer(command);
    case CommandType::BuyTicket:
        return handleBuyTicket(command);
    case CommandType::QueryOrder:
        return handleQueryOrder(command);
    case CommandType::RefundTicket:
        return handleRefundTicket(command);
    case CommandType::Clean:
        return handleClean(command);
    case CommandType::Exit:
        return handleExit(command);
    default:
        return "-1";
    }
}

bool TicketSystem::checkLogin(const std::string &username) const {
    return logged_users.find(username) != logged_users.cend();
}

int TicketSystem::privilegeOf(const std::string &username) const {
    UserRecord u;
    if (!storage_.loadUser(username, u))
        return -1;
    return u.privilege;
}

// ==================== handle 函数 ====================

std::string TicketSystem::handleAddUser(const Command &command) {
    std::string newu = command.getParam('u');
    std::string pwd = command.getParam('p');
    std::string name = command.getParam('n');
    std::string mail = command.getParam('m');
    std::string gs = command.getParam('g');
    std::string cur = command.getParam('c');
    UserRecord ur;
    ur.username = newu;
    ur.password = pwd;
    ur.name = name;
    ur.mail = mail;
    int g = 0;
    if (!gs.empty())
        g = std::stoi(gs);
    if (!storage_.hasAnyUser()) {
        ur.privilege = 10;
        if (!storage_.addUser(ur))
            return "-1";
        return "0";
    }
    if (!checkLogin(cur))
        return "-1";
    int curp = privilegeOf(cur);
    if (!(curp > g))
        return "-1";
    ur.privilege = g;
    if (!storage_.addUser(ur))
        return "-1";
    return "0";
}

std::string TicketSystem::handleLogin(const Command &command) {
    std::string u = command.getParam('u');
    std::string p = command.getParam('p');
    UserRecord ur;
    if (!storage_.loadUser(u, ur))
        return "-1";
    if (ur.password != p)
        return "-1";
    if (checkLogin(u))
        return "-1";
    logged_users.insert(sjtu::pair<const std::string, bool>(u, true));
    return "0";
}

std::string TicketSystem::handleLogout(const Command &command) {
    std::string u = command.getParam('u');
    auto it = logged_users.find(u);
    if (it == logged_users.cend()) {
        return "-1";
    } else {
        logged_users.erase(it);
        return "0";
    }
}

std::string TicketSystem::handleQueryProfile(const Command &command) {
    std::string cur = command.getParam('c');
    std::string target = command.getParam('u');
    if (!checkLogin(cur))
        return "-1";
    UserRecord tr;
    if (!storage_.loadUser(target, tr))
        return "-1";
    int curp;
    if (cur == target) {
        curp = tr.privilege;
    } else {
        curp = privilegeOf(cur);
    }
    if (!(cur == target || curp > tr.privilege))
        return "-1";
    return tr.username + " " + tr.name + " " + tr.mail + " " + std::to_string(tr.privilege);
}

std::string TicketSystem::handleModifyProfile(const Command &command) {
    std::string cur = command.getParam('c');
    std::string target = command.getParam('u');
    if (!checkLogin(cur))
        return "-1";
    int curp = privilegeOf(cur);
    UserRecord tr;
    if (!storage_.loadUser(target, tr))
        return "-1";
    if (!(cur == target || curp > tr.privilege))
        return "-1";
    if (command.hasParam('p'))
        tr.password = command.getParam('p');
    if (command.hasParam('n'))
        tr.name = command.getParam('n');
    if (command.hasParam('m'))
        tr.mail = command.getParam('m');
    if (command.hasParam('g')) {
        int newg = std::stoi(command.getParam('g'));
        if (!(curp > newg))
            return "-1";
        tr.privilege = newg;
    }
    if (!storage_.updateUser(tr))
        return "-1";
    return tr.username + " " + tr.name + " " + tr.mail + " " + std::to_string(tr.privilege);
}

std::string TicketSystem::handleAddTrain(const Command &command) {
    TrainRecord train;
    train.train_id = command.getParam('i');
    train.station_num = std::stoi(command.getParam('n'));
    train.seat_num = std::stoi(command.getParam('m'));
    train.max_seat_num = train.seat_num;
    train.start_time = Time::parse(command.getParam('x'));
    sjtu::vector<std::string> saleParts;
    splitString(command.getParam('d'), '|', saleParts, 2);
    if (saleParts.size() != 2)
        return "-1";
    train.sale_begin = Date::parse(saleParts[0]);
    train.sale_end = Date::parse(saleParts[1]);
    train.type = command.getParam('y')[0];
    train.released = false;
    sjtu::vector<std::string> stationParts;
    splitString(command.getParam('s'), '|', stationParts, MAX_STATIONS);
    train.stations = std::move(stationParts);
    train.station_num = static_cast<int>(train.stations.size());

    sjtu::vector<std::string> priceParts;
    splitString(command.getParam('p'), '|', priceParts, MAX_PRICE_SEGMENTS);
    train.prices.clear();
    for (const auto &part : priceParts)
        train.prices.push_back(std::stoi(part));

    sjtu::vector<std::string> travelParts;
    splitString(command.getParam('t'), '|', travelParts, MAX_TRAVEL_SEGMENTS);
    train.travel_times.clear();
    for (const auto &part : travelParts)
        train.travel_times.push_back(std::stoi(part));

    std::string stopArg = command.getParam('o');
    if (stopArg != "_") {
        sjtu::vector<std::string> stopParts;
        splitString(stopArg, '|', stopParts, MAX_TRAVEL_SEGMENTS);
        train.stopover_times.clear();
        for (const auto &part : stopParts)
            train.stopover_times.push_back(std::stoi(part));
    } else {
        train.stopover_times.clear();
    }
    if (!storage_.addTrain(train))
        return "-1";
    return "0";
}

std::string TicketSystem::handleDeleteTrain(const Command &command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train))
        return "-1";
    if (train.released)
        return "-1";
    if (!storage_.removeTrain(id))
        return "-1";
    return "0";
}

std::string TicketSystem::handleReleaseTrain(const Command &command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train))
        return "-1";
    if (train.released)
        return "-1";
    train.released = true;
    if (!storage_.updateTrain(train))
        return "-1";
    return "0";
}

std::string TicketSystem::handleQueryTrain(const Command &command) {
    std::string id = command.getParam('i');
    Date date = Date::parse(command.getParam('d'));
    TrainRecord train;
    if (!storage_.loadTrain(id, train))
        return "-1";
    if (date < train.sale_begin)
        return "-1";
    if (train.sale_end < date)
        return "-1";
    Date start_date = date;
    std::string out = train.train_id + " ";
    out.push_back(train.type);
    for (int i = 0; i < train.station_num; ++i) {
        out += "\n" + train.stations[i] + " ";
        if (i == 0) {
            out += "xx-xx xx:xx -> " + stationDeparture(train, i, start_date).toString();
        } else if (i == train.station_num - 1) {
            out += stationArrival(train, i, start_date).toString() + " -> xx-xx xx:xx";
        } else {
            out += stationArrival(train, i, start_date).toString() + " -> " + stationDeparture(train, i, start_date).toString();
        }
        int price = routePrice(train, 0, i);
        out += " " + std::to_string(price);
        out += " ";
        if (i == train.station_num - 1) {
            out += "x";
        } else {
            int seats = train.seat_num;
            if (train.released) {
                seats = availableSeats(storage_, train, start_date, i, i + 1);
            }
            out += std::to_string(seats);
        }
    }
    return out;
}

std::string TicketSystem::handleQueryTicket(const Command &command) {
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    sjtu::vector<TrainRecord> trains;
    if (!storage_.loadTrainsByStationPair(from, to, trains))
        return "0";
    sjtu::vector<Item> items;
    for (size_t i = 0; i < trains.size(); ++i) {
        const TrainRecord &train = trains[i];
        if (!train.released)
            continue;
        int from_idx = findStation(train, from);
        int to_idx = findStation(train, to);
        if (from_idx < 0 || to_idx < 0 || to_idx <= from_idx)
            continue;
        if (!canRunOnDate(train, from_idx, date))
            continue;
        Date start_date = getRunStartDate(train, from_idx, date);
        DateTime depart = stationDeparture(train, from_idx, start_date);
        DateTime arrive = stationArrival(train, to_idx, start_date);
        int price = routePrice(train, from_idx, to_idx);
        int seats = availableSeats(storage_, train, start_date, from_idx, to_idx);
        int duration = DateTimeUtils::dayOffset(depart.date, arrive.date) * 1440 + (arrive.time.hour * 60 + arrive.time.minute) - (depart.time.hour * 60 + depart.time.minute);
        std::string line = train.train_id + " " + from + " " + depart.toString() + " -> " + to + " " + arrive.toString() + " " + std::to_string(price) + " " + std::to_string(seats);
        if (order == "time") {
            items.push_back({line, duration, train.train_id});
        } else {
            items.push_back({line, price, train.train_id});
        }
    }
    if (items.empty()) {
        return "0";
    }
    sortItem(items);
    std::string out = std::to_string(static_cast<int>(items.size()));
    for (size_t i = 0; i < items.size(); ++i) {
        out += "\n" + items[i].line;
    }
    return out;
}

std::string TicketSystem::handleQueryTransfer(const Command &command) {
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    sjtu::vector<TrainRecord> firstTrains;
    if (!storage_.loadTrainsByStation(from, firstTrains))
        return "0";
    bool found = false;
    int best_main = 0;
    int best_secondary = 0;
    std::string best_line1;
    std::string best_line2;
    std::string best_first_id;
    std::string best_second_id;

    for (size_t i = 0; i < firstTrains.size(); ++i) {
        TrainRecord &first = firstTrains[i];
        if (!first.released)
            continue;
        int start_idx = findStation(first, from);
        if (start_idx < 0)
            continue;
        Date first_start = getRunStartDate(first, start_idx, date);
        DateTime first_depart = stationDeparture(first, start_idx, first_start);
        if (!(first_depart.date == date))
            continue;
        if (first_start < first.sale_begin || first.sale_end < first_start)
            continue;
        for (int mid = start_idx + 1; mid < first.station_num; ++mid) {
            const std::string &mid_station = first.stations[mid];
            DateTime first_arrive = stationArrival(first, mid, first_start);
            int first_price = routePrice(first, start_idx, mid);
            int seats1 = availableSeats(storage_, first, first_start, start_idx, mid);
            if (seats1 <= 0)
                continue;

            sjtu::vector<TrainRecord> midSecondTrains;
            if (!storage_.loadTrainsByStationPair(mid_station, to, midSecondTrains))
                continue;

            for (size_t j = 0; j < midSecondTrains.size(); ++j) {
                TrainRecord &second = midSecondTrains[j];
                if (!second.released)
                    continue;
                if (first.train_id == second.train_id)
                    continue;
                int mid_idx = findStation(second, mid_station);
                int end_idx = findStation(second, to);
                if (mid_idx < 0 || end_idx < 0 || mid_idx >= end_idx)
                    continue;
                DateTime second_depart = bestSecondDeparture(second, mid_idx, first_arrive);
                if (second_depart.date.month == 0)
                    continue;

                Date second_start = getRunStartDate(second, mid_idx, second_depart.date);
                DateTime second_arrive = stationArrival(second, end_idx, second_start);

                int second_price = routePrice(second, mid_idx, end_idx);
                int seats2 = availableSeats(storage_, second, second_start, mid_idx, end_idx);
                if (seats2 <= 0)
                    continue;
                int total_price = first_price + second_price;
                int total_time = DateTimeUtils::dayOffset(first_depart.date, second_arrive.date) * 1440 + (second_arrive.time.hour * 60 + second_arrive.time.minute) - (first_depart.time.hour * 60 + first_depart.time.minute);

                std::string line1 = first.train_id + " " + from + " " + first_depart.toString() + " -> " + first.stations[mid] + " " + first_arrive.toString() + " " + std::to_string(first_price) + " " + std::to_string(seats1);
                std::string line2 = second.train_id + " " + first.stations[mid] + " " + second_depart.toString() + " -> " + to + " " + second_arrive.toString() + " " + std::to_string(second_price) + " " + std::to_string(seats2);

                int main_key = (order == "time" ? total_time : total_price);
                int sec_key = (order == "time" ? total_price : total_time);
                if (!found) {
                    found = true;
                    best_main = main_key;
                    best_secondary = sec_key;
                    best_first_id = first.train_id;
                    best_second_id = second.train_id;
                    best_line1 = line1;
                    best_line2 = line2;
                } else {
                    bool isBetter = main_key < best_main ||
                                    (main_key == best_main && sec_key < best_secondary) ||
                                    (main_key == best_main && sec_key == best_secondary && first.train_id < best_first_id) ||
                                    (main_key == best_main && sec_key == best_secondary && first.train_id == best_first_id && second.train_id < best_second_id);
                    if (isBetter) {
                        best_main = main_key;
                        best_secondary = sec_key;
                        best_first_id = first.train_id;
                        best_second_id = second.train_id;
                        best_line1 = line1;
                        best_line2 = line2;
                    }
                }
            }
        }
    }
    if (!found) {
        return "0";
    }
    std::string result = best_line1 + "\n" + best_line2;
    return result;
}

std::string TicketSystem::handleBuyTicket(const Command &command) {
    std::string user = command.getParam('u');
    std::string train_id = command.getParam('i');
    Date date = Date::parse(command.getParam('d'));
    int num = std::stoi(command.getParam('n'));
    std::string from = command.getParam('f');
    std::string to = command.getParam('t');
    std::string q = command.hasParam('q') ? command.getParam('q') : "false";
    if (!checkLogin(user))
        return "-1";
    TrainRecord train;
    if (!storage_.loadTrain(train_id, train))
        return "-1";
    if (!train.released)
        return "-1";
    int from_idx, to_idx;
    if (!findFromTo(train, from, to, from_idx, to_idx))
        return "-1";
    if (!canRunOnDate(train, from_idx, date))
        return "-1";
    Date start_date = getRunStartDate(train, from_idx, date);
    int seats = availableSeats(storage_, train, start_date, from_idx, to_idx);
    int unit_price = routePrice(train, from_idx, to_idx);
    OrderRecord order;
    order.username = user;
    order.train_id = train_id;
    order.date = date;
    order.from_idx = from_idx;
    order.to_idx = to_idx;
    order.num = num;
    order.price = unit_price;
    order.timestamp = command.timestamp;
    if (num > train.max_seat_num)
        return "-1";
    if (seats >= num) {
        order.status = OrderStatus::Success;
        order.is_waiting = false;
        if (!storage_.addOrder(order))
            return "-1";
        return std::to_string(static_cast<long long>(unit_price) * num);
    }
    if (q == "true") {
        order.status = OrderStatus::Pending;
        order.is_waiting = true;
        if (!storage_.addOrder(order))
            return "-1";
        return "queue";
    }
    return "-1";
}

std::string TicketSystem::handleQueryOrder(const Command &command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user))
        return "-1";
    sjtu::vector<OrderRecord> orders;
    sjtu::vector<int> ids;
    if (!loadUserOrders(storage_, user, orders, ids, true)) {
        return "-1";
    }
    int count = static_cast<int>(orders.size());
    std::string out = std::to_string(count);
    for (int i = 0; i < count; ++i) {
        OrderRecord &order = orders[i];
        std::string status = orderStatusString(order.status);
        TrainRecord train;
        if (!storage_.loadTrain(order.train_id, train)) {
            return "-1";
        }
        Date start_date = getRunStartDate(train, order.from_idx, order.date);
        DateTime depart = stationDeparture(train, order.from_idx, start_date);
        DateTime arrive = stationArrival(train, order.to_idx, start_date);
        out += "\n[" + status + "] " + order.train_id + " " + train.stations[order.from_idx] + " " + depart.toString() + " -> " + train.stations[order.to_idx] + " " + arrive.toString() + " " + std::to_string(order.price) + " " + std::to_string(order.num);
    }
    return out;
}

std::string TicketSystem::handleRefundTicket(const Command &command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user))
        return "-1";
    int index = 1;
    if (command.hasParam('n'))
        index = std::stoi(command.getParam('n'));
    sjtu::vector<OrderRecord> orders;
    sjtu::vector<int> ids;
    if (!loadUserOrders(storage_, user, orders, ids, true)) {
        return "-1";
    }
    int count = static_cast<int>(orders.size());
    if (index < 1 || index > count) {
        return "-1";
    }
    OrderRecord order = orders[index - 1];
    int orderIdTarget = ids[index - 1];
    if (order.status == OrderStatus::Refunded) {
        return "-1";
    }
    bool was_success = (order.status == OrderStatus::Success);
    order.status = OrderStatus::Refunded;
    order.is_waiting = false;
    if (!storage_.updateOrder(orderIdTarget, order)) {
        return "-1";
    }
    if (was_success) {
        TrainRecord train;
        if (storage_.loadTrain(order.train_id, train)) {
            Date run_start = getRunStartDate(train, order.from_idx, order.date);
            processWaitlist(*this, storage_, train, run_start);
        }
    }
    return "0";
}

std::string TicketSystem::handleClean(const Command &command) {
    std::filesystem::remove_all(storage_.basePath());
    storage_.initialize(storage_.basePath());
    logged_users.clear();
    return "0";
}

std::string TicketSystem::handleExit(const Command &command) {
    logged_users.clear();
    return "bye";
}

} // namespace ticket
