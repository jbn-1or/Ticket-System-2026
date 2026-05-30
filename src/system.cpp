#include "../include/System.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace ticket {

static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void splitString(const std::string& text, char delim, std::string parts[], int& count, int maxCount) {
    count = 0;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == delim) {
            if (count < maxCount) parts[count++] = current;
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    if (count < maxCount) parts[count++] = current;
}

static int findStation(const TrainRecord& train, const std::string& station) {
    for (int i = 0; i < train.station_num; ++i) {
        if (train.stations[i] == station) return i;
    }
    return -1;
}

static int arrivalOffsetMinutes(const TrainRecord& train, int station_idx) {
    if (station_idx == 0) return 0;
    int minutes = 0;
    for (int i = 0; i < station_idx; ++i) {
        minutes += train.travel_times[i];
        if (i + 1 < station_idx) minutes += train.stopover_times[i];
    }
    return minutes;
}

static int departureOffsetMinutes(const TrainRecord& train, int station_idx) {
    int minutes = arrivalOffsetMinutes(train, station_idx);
    if (station_idx > 0 && station_idx < train.station_num - 1) {
        minutes += train.stopover_times[station_idx - 1];
    }
    return minutes;
}

static Date addDays(const Date& date, int offset) {
    int month = date.month;
    int day = date.day;
    if (offset >= 0) {
        while (offset > 0) {
            int month_len = 31;
            if (month == 2) month_len = 28;
            else if (month == 4 || month == 6 || month == 9 || month == 11) month_len = 30;
            int remain = month_len - day;
            if (offset <= remain) {
                day += offset;
                offset = 0;
            } else {
                offset -= (remain + 1);
                day = 1;
                month += 1;
                if (month > 12) month = 1;
            }
        }
    } else {
        offset = -offset;
        while (offset > 0) {
            if (offset < day) {
                day -= offset;
                offset = 0;
            } else {
                offset -= day;
                month -= 1;
                if (month < 1) month = 12;
                if (month == 2) day = 28;
                else if (month == 4 || month == 6 || month == 9 || month == 11) day = 30;
                else day = 31;
            }
        }
    }
    return Date(month, day);
}

static Time departureTimeAtStation(const TrainRecord& train, int station_idx) {
    int total = train.start_time.hour * 60 + train.start_time.minute + departureOffsetMinutes(train, station_idx);
    total %= 1440;
    if (total < 0) total += 1440;
    return Time(total / 60, total % 60);
}

static Date getRunStartDate(const TrainRecord& train, int station_idx, const Date& depart_date) {
    int offset = departureOffsetMinutes(train, station_idx);
    DateTime current(Date(depart_date.month, depart_date.day), departureTimeAtStation(train, station_idx));
    DateTime start = addMinutes(current, -offset);
    return start.date;
}

static DateTime stationArrival(const TrainRecord& train, int station_idx, const Date& start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, arrivalOffsetMinutes(train, station_idx));
}

static DateTime stationDeparture(const TrainRecord& train, int station_idx, const Date& start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, departureOffsetMinutes(train, station_idx));
}

static int routePrice(const TrainRecord& train, int from_idx, int to_idx) {
    int price = 0;
    for (int i = from_idx; i < to_idx; ++i) price += train.prices[i];
    return price;
}

static bool dateInRange(const Date& begin, const Date& end, const Date& target) {
    if (target < begin) return false;
    if (end < target) return false;
    return true;
}

TicketSystem::TicketSystem() {}
TicketSystem::~TicketSystem() {}

bool TicketSystem::initialize(const std::string& dataPath) {
    return storage_.initialize(dataPath);
}

std::string TicketSystem::execute(const Command& command) {
    switch (command.type) {
        case CommandType::AddUser: return handleAddUser(command);
        case CommandType::Login: return handleLogin(command);
        case CommandType::Logout: return handleLogout(command);
        case CommandType::QueryProfile: return handleQueryProfile(command);
        case CommandType::ModifyProfile: return handleModifyProfile(command);
        case CommandType::AddTrain: return handleAddTrain(command);
        case CommandType::DeleteTrain: return handleDeleteTrain(command);
        case CommandType::ReleaseTrain: return handleReleaseTrain(command);
        case CommandType::QueryTrain: return handleQueryTrain(command);
        case CommandType::QueryTicket: return handleQueryTicket(command);
        case CommandType::QueryTransfer: return handleQueryTransfer(command);
        case CommandType::BuyTicket: return handleBuyTicket(command);
        case CommandType::QueryOrder: return handleQueryOrder(command);
        case CommandType::RefundTicket: return handleRefundTicket(command);
        case CommandType::Clean: return handleClean(command);
        case CommandType::Exit: return handleExit(command);
        default: return "-1";
    }
}

bool TicketSystem::checkLogin(const std::string& username) const {
    for (int i = 0; i < logged_count; ++i) {
        if (logged_users[i] == username) return true;
    }
    return false;
}

int TicketSystem::privilegeOf(const std::string& username) const {
    UserRecord u;
    if (!storage_.loadUser(username, u)) return -1;
    return u.privilege;
}

std::string TicketSystem::handleAddUser(const Command& command) {
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
    if (!gs.empty()) g = std::stoi(gs);
    if (!storage_.hasAnyUser()) {
        ur.privilege = 10;
        if (!storage_.addUser(ur)) return "-1";
        return "0";
    }
    if (!checkLogin(cur)) return "-1";
    int curp = privilegeOf(cur);
    if (!(curp > g)) return "-1";
    ur.privilege = g;
    if (!storage_.addUser(ur)) return "-1";
    return "0";
}

std::string TicketSystem::handleLogin(const Command& command) {
    std::string u = command.getParam('u');
    std::string p = command.getParam('p');
    UserRecord ur;
    if (!storage_.loadUser(u, ur)) return "-1";
    if (ur.password != p) return "-1";
    if (checkLogin(u)) return "-1";
    if (logged_count >= MAX_LOGGED) return "-1";
    logged_users[logged_count++] = u;
    return "0";
}

std::string TicketSystem::handleLogout(const Command& command) {
    std::string u = command.getParam('u');
    for (int i = 0; i < logged_count; ++i) {
        if (logged_users[i] == u) {
            for (int j = i; j + 1 < logged_count; ++j) logged_users[j] = logged_users[j + 1];
            --logged_count;
            return "0";
        }
    }
    return "-1";
}

std::string TicketSystem::handleQueryProfile(const Command& command) {
    std::string cur = command.getParam('c');
    std::string target = command.getParam('u');
    if (!checkLogin(cur)) return "-1";
    int curp = privilegeOf(cur);
    UserRecord tr;
    if (!storage_.loadUser(target, tr)) return "-1";
    if (!(cur == target || curp > tr.privilege)) return "-1";
    return tr.username + " " + tr.name + " " + tr.mail + " " + std::to_string(tr.privilege);
}

std::string TicketSystem::handleModifyProfile(const Command& command) {
    std::string cur = command.getParam('c');
    std::string target = command.getParam('u');
    if (!checkLogin(cur)) return "-1";
    int curp = privilegeOf(cur);
    UserRecord tr;
    if (!storage_.loadUser(target, tr)) return "-1";
    if (!(cur == target || curp > tr.privilege)) return "-1";
    if (command.hasParam('p')) tr.password = command.getParam('p');
    if (command.hasParam('n')) tr.name = command.getParam('n');
    if (command.hasParam('m')) tr.mail = command.getParam('m');
    if (command.hasParam('g')) {
        int newg = std::stoi(command.getParam('g'));
        if (!(curp > newg)) return "-1";
        tr.privilege = newg;
    }
    if (!storage_.updateUser(tr)) return "-1";
    return tr.username + " " + tr.name + " " + tr.mail + " " + std::to_string(tr.privilege);
}

static bool lineToTrain(const std::string& line, TrainRecord& train) {
    std::string fields[12];
    int count = 0;
    splitString(line, '|', fields, count, 12);
    if (count < 12) return false;
    train.train_id = fields[0];
    train.station_num = std::stoi(fields[1]);
    train.seat_num = std::stoi(fields[2]);
    train.start_time = Time::parse(fields[3]);
    train.sale_begin = Date::parse(fields[4]);
    train.sale_end = Date::parse(fields[5]);
    train.type = fields[6].empty() ? ' ' : fields[6][0];
    train.released = (fields[7] == "1");

    std::string stations[100]; int stationCount = 0;
    splitString(fields[8], ';', stations, stationCount, 100);
    for (int i = 0; i < stationCount; ++i) train.stations[i] = stations[i];

    std::string prices[100]; int priceCount = 0;
    splitString(fields[9], ';', prices, priceCount, 100);
    for (int i = 0; i < priceCount; ++i) train.prices[i] = std::stoi(prices[i]);

    std::string travels[100]; int travelCount = 0;
    splitString(fields[10], ';', travels, travelCount, 100);
    for (int i = 0; i < travelCount; ++i) train.travel_times[i] = std::stoi(travels[i]);

    std::string stopovers[100]; int stopoverCount = 0;
    splitString(fields[11], ';', stopovers, stopoverCount, 100);
    for (int i = 0; i < stopoverCount; ++i) train.stopover_times[i] = std::stoi(stopovers[i]);
    return true;
}

static bool lineToOrder(const std::string& line, OrderRecord& order) {
    std::string fields[11];
    int count = 0;
    splitString(line, '|', fields, count, 11);
    if (count < 11) return false;
    order.username = fields[0];
    order.train_id = fields[1];
    order.date = Date::parse(fields[2]);
    order.from_idx = std::stoi(fields[3]);
    order.to_idx = std::stoi(fields[4]);
    order.num = std::stoi(fields[5]);
    order.price = std::stoi(fields[6]);
    order.status = static_cast<OrderStatus>(std::stoi(fields[7]));
    order.is_waiting = (fields[8] == "1");
    order.timestamp = std::stoll(fields[9]);
    return true;
}

static bool loadAllTrains(const StorageManager& storage, TrainRecord trains[], int& count) {
    return storage.loadAllTrains(trains, count);
}

static int computeSegmentOccupancy(const StorageManager& storage, const TrainRecord& train, const Date& start_date, int occupancy[]) {
    for (int i = 0; i < train.station_num - 1; ++i) occupancy[i] = 0;
    int order_count = 0;
    OrderRecord orders[10000];
    int ids[10000];
    if (!storage.loadAllOrders(orders, ids, order_count)) return 0;
    for (int i = 0; i < order_count; ++i) {
        OrderRecord& order = orders[i];
        if (order.status != OrderStatus::Success) continue;
        if (order.train_id != train.train_id) continue;
        Date orderStart = getRunStartDate(train, order.from_idx, order.date);
        if (train.train_id == "LeavesofGrass" && start_date == Date(8, 5)) {
            std::cerr << "DEBUG order " << order.username << " from " << order.from_idx << " to " << order.to_idx << " date " << order.date.toString() << " start " << orderStart.toString() << " num " << order.num << "\n";
        }
        if (!(orderStart == start_date)) continue;
        for (int s = order.from_idx; s < order.to_idx; ++s) {
            if (s >= 0 && s < train.station_num - 1) occupancy[s] += order.num;
        }
    }
    return 0;
}

static int availableSeats(const StorageManager& storage, const TrainRecord& train, const Date& start_date, int from_idx, int to_idx) {
    int occupancy[100] = {0};
    computeSegmentOccupancy(storage, train, start_date, occupancy);
    int available = train.seat_num;
    for (int i = from_idx; i < to_idx; ++i) {
        int remain = train.seat_num - occupancy[i];
        if (remain < available) available = remain;
    }
    return available;
}

static bool canRunOnDate(const TrainRecord& train, int station_idx, const Date& depart_date) {
    Date start_date = getRunStartDate(train, station_idx, depart_date);
    return dateInRange(train.sale_begin, train.sale_end, start_date);
}

std::string TicketSystem::handleAddTrain(const Command& command) {
    TrainRecord train;
    train.train_id = command.getParam('i');
    train.station_num = std::stoi(command.getParam('n'));
    train.seat_num = std::stoi(command.getParam('m'));
    train.start_time = Time::parse(command.getParam('x'));
    std::string saleDate = command.getParam('d');
    std::string saleParts[2];
    int saleCount = 0;
    splitString(saleDate, '|', saleParts, saleCount, 2);
    if (saleCount != 2) return "-1";
    train.sale_begin = Date::parse(saleParts[0]);
    train.sale_end = Date::parse(saleParts[1]);
    train.type = command.getParam('y')[0];
    train.released = false;
    int stationCount = 0;
    splitString(command.getParam('s'), '|', train.stations, stationCount, MAX_STATIONS);
    int priceCount = 0;
    std::string priceParts[100];
    splitString(command.getParam('p'), '|', priceParts, priceCount, MAX_PRICE_SEGMENTS);
    for (int i = 0; i < priceCount; ++i) train.prices[i] = std::stoi(priceParts[i]);
    int travelCount = 0;
    std::string travelParts[100];
    splitString(command.getParam('t'), '|', travelParts, travelCount, MAX_TRAVEL_SEGMENTS);
    for (int i = 0; i < travelCount; ++i) train.travel_times[i] = std::stoi(travelParts[i]);
    int stopCount = 0;
    std::string stopParts[100];
    std::string stopArg = command.getParam('o');
    if (stopArg != "_") {
        splitString(stopArg, '|', stopParts, stopCount, MAX_TRAVEL_SEGMENTS);
        for (int i = 0; i < stopCount; ++i) train.stopover_times[i] = std::stoi(stopParts[i]);
    }
    if (!storage_.addTrain(train)) return "-1";
    return "0";
}

std::string TicketSystem::handleDeleteTrain(const Command& command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train)) return "-1";
    if (train.released) return "-1";
    if (!storage_.removeTrain(id)) return "-1";
    return "0";
}

std::string TicketSystem::handleReleaseTrain(const Command& command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train)) return "-1";
    if (train.released) return "-1";
    train.released = true;
    if (!storage_.updateTrain(train)) return "-1";
    return "0";
}

std::string TicketSystem::handleQueryTrain(const Command& command) {
    std::string id = command.getParam('i');
    Date date = Date::parse(command.getParam('d'));
    TrainRecord train;
    if (!storage_.loadTrain(id, train)) return "-1";
    if (!dateInRange(train.sale_begin, train.sale_end, date)) return "-1";
    Date start_date = date;
    std::string out = train.train_id + " ";
    out.push_back(train.type);
    for (int i = 0; i < train.station_num; ++i) {
        out += "\n" + train.stations[i] + " ";
        if (i == 0) {
            out += "xx-xx xx:xx -> " + stationDeparture(train, i, start_date).time.toString();
        } else if (i == train.station_num - 1) {
            out += stationArrival(train, i, start_date).toString() + " -> xx-xx xx:xx";
        } else {
            out += stationArrival(train, i, start_date).toString() + " -> " + stationDeparture(train, i, start_date).time.toString();
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

std::string TicketSystem::handleQueryTicket(const Command& command) {
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    TrainRecord* trains = new TrainRecord[10000];
    int train_count = 0;
    if (!loadAllTrains(storage_, trains, train_count)) {
        delete[] trains;
        return "0";
    }
    struct Item { std::string line; int key1; std::string key2; };
    Item* items = new Item[10000];
    int item_count = 0;
    for (int i = 0; i < train_count; ++i) {
        TrainRecord& train = trains[i];
        if (!train.released) continue;
        int from_idx = findStation(train, from);
        int to_idx = findStation(train, to);
        if (from_idx < 0 || to_idx < 0 || from_idx >= to_idx) continue;
        if (!canRunOnDate(train, from_idx, date)) continue;
        Date start_date = getRunStartDate(train, from_idx, date);
        DateTime depart = stationDeparture(train, from_idx, start_date);
        DateTime arrive = stationArrival(train, to_idx, start_date);
        int price = routePrice(train, from_idx, to_idx);
        int seats = availableSeats(storage_, train, start_date, from_idx, to_idx);
        int duration = DateTimeUtils::dayOffset(depart.date, arrive.date) * 1440 + (arrive.time.hour * 60 + arrive.time.minute) - (depart.time.hour * 60 + depart.time.minute);
        std::string line = train.train_id + " " + from + " " + depart.toString() + " -> " + to + " " + arrive.toString() + " " + std::to_string(price) + " " + std::to_string(seats);
        if (item_count < 10000) {
            if (order == "time") {
                items[item_count++] = { line, duration, std::to_string(price) };
            } else {
                items[item_count++] = { line, price, train.train_id };
            }
        }
    }
    if (item_count == 0) {
        delete[] trains;
        delete[] items;
        return "0";
    }
    for (int i = 0; i < item_count - 1; ++i) {
        int best = i;
        for (int j = i + 1; j < item_count; ++j) {
            if (items[j].key1 < items[best].key1 || (items[j].key1 == items[best].key1 && items[j].key2 < items[best].key2)) {
                best = j;
            }
        }
        if (best != i) {
            Item tmp = items[i]; items[i] = items[best]; items[best] = tmp;
        }
    }
    std::string out = std::to_string(item_count);
    for (int i = 0; i < item_count; ++i) {
        out += "\n" + items[i].line;
    }
    delete[] trains;
    delete[] items;
    return out;
}

static DateTime bestSecondDeparture(const TrainRecord& train, int station_idx, const DateTime& earliest) {
    for (int add = 0; add < 3; ++add) {
        Date candidate = addDays(earliest.date, add);
        Date start_date = getRunStartDate(train, station_idx, candidate);
        if (!dateInRange(train.sale_begin, train.sale_end, start_date)) continue;
        DateTime depart = stationDeparture(train, station_idx, start_date);
        if (!(depart < earliest)) return depart;
    }
    return DateTime(Date(0, 0), Time(0, 0));
}

std::string TicketSystem::handleQueryTransfer(const Command& command) {
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    TrainRecord* trains = new TrainRecord[10000];
    int train_count = 0;
    if (!loadAllTrains(storage_, trains, train_count)) {
        delete[] trains;
        return "0";
    }
    bool found = false;
    int best_main = 0;
    int best_secondary = 0;
    std::string best_line1;
    std::string best_line2;
    std::string best_first_id;
    std::string best_second_id;
    for (int i = 0; i < train_count; ++i) {
        TrainRecord& first = trains[i];
        if (!first.released) continue;
        int start_idx = findStation(first, from);
        if (start_idx < 0) continue;
        if (!canRunOnDate(first, start_idx, date)) continue;
        Date first_start = getRunStartDate(first, start_idx, date);
        DateTime first_depart = stationDeparture(first, start_idx, first_start);
        for (int mid = start_idx + 1; mid < first.station_num - 1; ++mid) {
            DateTime first_arrive = stationArrival(first, mid, first_start);
            for (int j = 0; j < train_count; ++j) {
                TrainRecord& second = trains[j];
                if (!second.released) continue;
                if (first.train_id == second.train_id) continue;
                int mid_idx = findStation(second, first.stations[mid]);
                int end_idx = findStation(second, to);
                if (mid_idx < 0 || end_idx < 0 || mid_idx >= end_idx) continue;
                DateTime second_depart = bestSecondDeparture(second, mid_idx, first_arrive);
                if (second_depart.date.month == 0) continue;
                Date second_start = getRunStartDate(second, mid_idx, second_depart.date);
                DateTime second_arrive = stationArrival(second, end_idx, second_start);
                int first_price = routePrice(first, start_idx, mid);
                int second_price = routePrice(second, mid_idx, end_idx);
                int total_price = first_price + second_price;
                int total_time = DateTimeUtils::dayOffset(first_depart.date, second_arrive.date) * 1440 + (second_arrive.time.hour * 60 + second_arrive.time.minute) - (first_depart.time.hour * 60 + first_depart.time.minute);
                std::string line1 = first.train_id + " " + from + " " + first_depart.toString() + " -> " + first.stations[mid] + " " + first_arrive.toString() + " " + std::to_string(first_price) + " " + std::to_string(availableSeats(storage_, first, first_start, start_idx, mid));
                std::string line2 = second.train_id + " " + first.stations[mid] + " " + second_depart.toString() + " -> " + to + " " + second_arrive.toString() + " " + std::to_string(second_price) + " " + std::to_string(availableSeats(storage_, second, second_start, mid_idx, end_idx));
                if (!found) {
                    found = true;
                    best_main = (order == "time" ? total_time : total_price);
                    best_secondary = (order == "time" ? total_price : total_time);
                    best_first_id = first.train_id;
                    best_second_id = second.train_id;
                    best_line1 = line1;
                    best_line2 = line2;
                } else {
                    int main_key = (order == "time" ? total_time : total_price);
                    int sec_key = (order == "time" ? total_price : total_time);
                    bool better = false;
                    if (main_key < best_main) better = true;
                    else if (main_key == best_main) {
                        if (sec_key < best_secondary) better = true;
                        else if (sec_key == best_secondary) {
                            if (first.train_id < best_first_id) better = true;
                            else if (first.train_id == best_first_id && second.train_id < best_second_id) better = true;
                        }
                    }
                    if (better) {
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
        delete[] trains;
        return "0";
    }
    std::string result = best_line1 + "\n" + best_line2;
    delete[] trains;
    return result;
}

static bool loadAllOrders(const StorageManager& storage, OrderRecord orders[], int ids[], int& count) {
    return storage.loadAllOrders(orders, ids, count);
}

std::string TicketSystem::handleBuyTicket(const Command& command) {
    std::string user = command.getParam('u');
    std::string train_id = command.getParam('i');
    Date date = Date::parse(command.getParam('d'));
    int num = std::stoi(command.getParam('n'));
    std::string from = command.getParam('f');
    std::string to = command.getParam('t');
    std::string q = command.hasParam('q') ? command.getParam('q') : "false";
    if (!checkLogin(user)) return "-1";
    TrainRecord train;
    if (!storage_.loadTrain(train_id, train)) return "-1";
    if (!train.released) return "-1";
    int from_idx = findStation(train, from);
    int to_idx = findStation(train, to);
    if (from_idx < 0 || to_idx < 0 || from_idx >= to_idx) return "-1";
    if (!canRunOnDate(train, from_idx, date)) return "-1";
    Date start_date = getRunStartDate(train, from_idx, date);
    int seats = availableSeats(storage_, train, start_date, from_idx, to_idx);
    int unit_price = routePrice(train, from_idx, to_idx);
    if (seats >= num) {
        OrderRecord order;
        order.username = user;
        order.train_id = train_id;
        order.date = date;
        order.from_idx = from_idx;
        order.to_idx = to_idx;
        order.num = num;
        order.price = unit_price * num;
        order.status = OrderStatus::Success;
        order.is_waiting = false;
        order.timestamp = command.timestamp;
        if (!storage_.addOrder(order)) return "-1";
        return std::to_string(order.price);
    }
    if (q == "true") {
        OrderRecord order;
        order.username = user;
        order.train_id = train_id;
        order.date = date;
        order.from_idx = from_idx;
        order.to_idx = to_idx;
        order.num = num;
        order.price = unit_price * num;
        order.status = OrderStatus::Pending;
        order.is_waiting = true;
        order.timestamp = command.timestamp;
        if (!storage_.addOrder(order)) return "-1";
        return "queue";
    }
    return "-1";
}

std::string TicketSystem::handleQueryOrder(const Command& command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user)) return "-1";
    OrderRecord* orders = new OrderRecord[10000];
    int* ids = new int[10000];
    int count = 0;
    if (!storage_.loadAllOrders(orders, ids, count)) return "-1";
    int outCount = 0;
    for (int i = 0; i < count; ++i) {
        if (orders[i].username == user) {
            orders[outCount] = orders[i];
            ids[outCount] = ids[i];
            outCount++;
        }
    }
    count = outCount;
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (orders[j].timestamp > orders[i].timestamp) {
                std::swap(orders[i], orders[j]);
                std::swap(ids[i], ids[j]);
            }
        }
    }
    std::string out = std::to_string(count);
    for (int i = 0; i < count; ++i) {
        OrderRecord& order = orders[i];
        std::string status;
        if (order.status == OrderStatus::Success) status = "success";
        else if (order.status == OrderStatus::Pending) status = "pending";
        else status = "refunded";
        TrainRecord train;
        if (!storage_.loadTrain(order.train_id, train)) {
            delete[] orders;
            delete[] ids;
            return "-1";
        }
        Date start_date = getRunStartDate(train, order.from_idx, order.date);
        DateTime depart = stationDeparture(train, order.from_idx, start_date);
        DateTime arrive = stationArrival(train, order.to_idx, start_date);
        out += "\n[" + status + "] " + order.train_id + " " + train.stations[order.from_idx] + " " + depart.toString() + " -> " + train.stations[order.to_idx] + " " + arrive.toString() + " " + std::to_string(order.price) + " " + std::to_string(order.num);
    }
    delete[] orders;
    delete[] ids;
    return out;
}

static void processWaitlist(StorageManager& storage, const TrainRecord& train, const Date& start_date) {
    OrderRecord* orders = new OrderRecord[10000];
    int* ids = new int[10000];
    int count = 0;
    if (!loadAllOrders(storage, orders, ids, count)) {
        delete[] orders;
        delete[] ids;
        return;
    }
    for (int i = 0; i < count; ++i) {
        if (orders[i].train_id != train.train_id) continue;
        if (orders[i].status != OrderStatus::Pending) continue;
        Date orderStart = getRunStartDate(train, orders[i].from_idx, orders[i].date);
        if (!(orderStart == start_date)) continue;
        int seats = availableSeats(storage, train, start_date, orders[i].from_idx, orders[i].to_idx);
        if (seats >= orders[i].num) {
            orders[i].status = OrderStatus::Success;
            orders[i].is_waiting = false;
            storage.updateOrder(ids[i], orders[i]);
        }
    }
    delete[] orders;
    delete[] ids;
}

std::string TicketSystem::handleRefundTicket(const Command& command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user)) return "-1";
    int index = 1;
    if (command.hasParam('n')) index = std::stoi(command.getParam('n'));
    OrderRecord* orders = new OrderRecord[10000];
    int* ids = new int[10000];
    int count = 0;
    if (!storage_.loadAllOrders(orders, ids, count)) return "-1";
    int outCount = 0;
    for (int i = 0; i < count; ++i) {
        if (orders[i].username == user) {
            orders[outCount] = orders[i];
            ids[outCount] = ids[i];
            outCount++;
        }
    }
    count = outCount;
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (orders[j].timestamp > orders[i].timestamp) {
                std::swap(orders[i], orders[j]);
                std::swap(ids[i], ids[j]);
            }
        }
    }
    if (index < 1 || index > count) {
        delete[] orders;
        delete[] ids;
        return "-1";
    }
    OrderRecord order = orders[index - 1];
    int orderIdTarget = ids[index - 1];
    if (order.status == OrderStatus::Refunded) {
        delete[] orders;
        delete[] ids;
        return "-1";
    }
    bool was_success = (order.status == OrderStatus::Success);
    order.status = OrderStatus::Refunded;
    order.is_waiting = false;
    if (!storage_.updateOrder(orderIdTarget, order)) {
        delete[] orders;
        delete[] ids;
        return "-1";
    }
    if (was_success) {
        TrainRecord train;
        if (storage_.loadTrain(order.train_id, train)) {
            Date run_start = getRunStartDate(train, order.from_idx, order.date);
            processWaitlist(storage_, train, run_start);
        }
    }
    delete[] orders;
    delete[] ids;
    return "0";
}

std::string TicketSystem::handleClean(const Command& command) {
    std::filesystem::remove_all(storage_.basePath());
    storage_.initialize(storage_.basePath());
    logged_count = 0;
    return "0";
}

std::string TicketSystem::handleExit(const Command& command) {
    logged_count = 0;
    return "bye";
}

} // namespace ticket
