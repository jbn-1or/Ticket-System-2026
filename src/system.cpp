#include "System.hpp"
#include <filesystem>
#include <vector>
#include <cstring>
#include <algorithm>

namespace ticket {

// c: 待检查的字符
// 判断字符是否为空白字符（空格、制表符、回车、换行），是返回true
static bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// text: 待分割的字符串，delim: 分隔符，parts: 输出数组，count: 输出分割数量，maxCount: 最大分割数
// 按分隔符分割字符串，结果存入数组，返回分割数量
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

// text: 待分割的字符串，delim: 分隔符，parts: 输出向量，maxCount: 最大分割数
// 按分隔符分割字符串，结果存入向量
static void splitString(const std::string& text, char delim, std::vector<std::string>& parts, int maxCount) {
    parts.clear();
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == delim) {
            if ((int)parts.size() < maxCount) parts.push_back(current);
            current.clear();
        } else {
            current.push_back(text[i]);
        }
    }
    if ((int)parts.size() < maxCount) parts.push_back(current);
}

// train: 列车记录，station: 站点名称
// 在列车站点列表中查找指定站点，返回索引，未找到返回-1
static int findStation(const TrainRecord& train, const std::string& station) {
    for (int i = 0; i < train.station_num; ++i) {
        if (train.stations[i] == station) return i;
    }
    return -1;
}

// train: 列车记录，station_idx: 站点索引
// 计算从始发站到指定站点的到达时间偏移（分钟）
static int arrivalOffsetMinutes(const TrainRecord& train, int station_idx) {
    if (station_idx == 0) return 0;
    int minutes = 0;
    for (int i = 0; i < station_idx; ++i) {
        minutes += train.travel_times[i];
        if (i + 1 < station_idx) {
            minutes += train.stopover_times[i];
        }
    }
    return minutes;
}

// train: 列车记录，station_idx: 站点索引
// 计算从始发站到指定站点的离开时间偏移（分钟）
static int departureOffsetMinutes(const TrainRecord& train, int station_idx) {
    int minutes = arrivalOffsetMinutes(train, station_idx);
    if (station_idx > 0 && station_idx < train.station_num - 1) {
        minutes += train.stopover_times[station_idx - 1];
    }
    return minutes;
}

// date: 原始日期，offset: 天数偏移（可正可负）
// 计算日期加上指定天数后的新日期，返回新Date对象
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

// train: 列车记录，station_idx: 站点索引
// 计算列车在指定站点的出发时间，返回Time对象
static Time departureTimeAtStation(const TrainRecord& train, int station_idx) {
    int total = train.start_time.hour * 60 + train.start_time.minute + departureOffsetMinutes(train, station_idx);
    total %= 1440;
    if (total < 0) total += 1440;
    return Time(total / 60, total % 60);
}

// train: 列车记录，station_idx: 站点索引，depart_date: 该站点的出发日期
// 根据到达中途某站的时间，反推列车的起点日期，返回Date对象
static Date getRunStartDate(const TrainRecord& train, int station_idx, const Date& depart_date) {
    int offset = departureOffsetMinutes(train, station_idx);
    DateTime current(Date(depart_date.month, depart_date.day), departureTimeAtStation(train, station_idx));
    DateTime start = addMinutes(current, -offset);
    return start.date;
}

// train: 列车记录，station_idx: 站点索引，start_date: 运行起始日期
// 计算列车到达指定站点的时间，返回DateTime对象
static DateTime stationArrival(const TrainRecord& train, int station_idx, const Date& start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, arrivalOffsetMinutes(train, station_idx));
}

// train: 列车记录，station_idx: 站点索引，start_date: 运行起始日期
// 计算列车从指定站点出发的日期时间，返回DateTime对象
static DateTime stationDeparture(const TrainRecord& train, int station_idx, const Date& start_date) {
    DateTime base(start_date, train.start_time);
    return addMinutes(base, departureOffsetMinutes(train, station_idx));
}

// train: 列车记录，from_idx: 起始站点索引，to_idx: 到达站点索引
// 计算指定区间的票价，返回总票价
static int routePrice(const TrainRecord& train, int from_idx, int to_idx) {
    int price = 0;
    for (int i = from_idx; i < to_idx; ++i) price += train.prices[i];
    return price;
}

// begin: 范围开始日期，end: 范围结束日期，target: 目标日期
// 判断目标日期是否在[begin, end]范围内，在范围内返回true
static bool dateInRange(const Date& begin, const Date& end, const Date& target) {
    if (target < begin) return false;
    if (end < target) return false;
    return true;
}

// 默认构造函数：创建空的TicketSystem对象
// 初始化票务系统
TicketSystem::TicketSystem() {}

// 析构函数：清理TicketSystem资源
TicketSystem::~TicketSystem() {}

// dataPath: 数据存储目录路径
// 初始化票务系统，设置数据存储路径，返回是否成功
bool TicketSystem::initialize(const std::string& dataPath) {
    return storage_.initialize(dataPath);
}

// command: 要执行的命令对象
// 根据命令类型分发到对应的处理函数，返回执行结果字符串
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

// username: 用户名
// 检查用户是否已登录，已登录返回true
bool TicketSystem::checkLogin(const std::string& username) const {
    return logged_users.find(username) != logged_users.cend();
}

// username: 用户名
// 获取用户权限等级，用户不存在返回-1
int TicketSystem::privilegeOf(const std::string& username) const {
    UserRecord u;
    if (!storage_.loadUser(username, u)) return -1;
    return u.privilege;
}

// command: 包含用户信息的命令对象
// 处理添加用户请求，成功返回"0"，失败返回"-1"
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

// command: 包含登录信息的命令对象
// 处理用户登录请求，成功返回"0"，失败返回"-1"
std::string TicketSystem::handleLogin(const Command& command) {
    std::string u = command.getParam('u');
    std::string p = command.getParam('p');
    UserRecord ur;
    if (!storage_.loadUser(u, ur)) return "-1";
    if (ur.password != p) return "-1";
    if (checkLogin(u)) return "-1";
    logged_users.insert({u, true});
    return "0";
}

// command: 包含用户名的命令对象
// 处理用户登出请求，成功返回"0"，失败返回"-1"
std::string TicketSystem::handleLogout(const Command& command) {
    std::string u = command.getParam('u');
    auto it = logged_users.find(u);
    if(it == logged_users.cend()) {
        return "-1";
    } else {
        logged_users.erase(it);
        return "0";
    }
}

// command: 包含查询信息的命令对象
// 处理查询用户信息请求，成功返回用户信息，失败返回"-1"
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

// command: 包含修改信息的命令对象
// 处理修改用户信息请求，成功返回修改后的用户信息，失败返回"-1"
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

// storage: 存储管理器，train: 列车记录，start_date: 运行起始日期，occupancy: 输出占用数组
// 计算列车各段的座位占用情况，存入occupancy数组
static int computeSegmentOccupancy(const StorageManager& storage, const TrainRecord& train, const Date& start_date, int occupancy[]) {
    for (int i = 0; i < train.station_num - 1; ++i) occupancy[i] = 0;
    std::vector<OrderRecord> orders;
    std::vector<int> ids;
    if (!storage.loadOrdersByTrainDate(train.train_id, start_date, orders, ids)) return 0;
    for (size_t i = 0; i < orders.size(); ++i) {
        OrderRecord& order = orders[i];
        if (order.status != OrderStatus::Success) continue;
        if (order.train_id != train.train_id) continue;
        Date orderStart = getRunStartDate(train, order.from_idx, order.date);
        if (!(orderStart == start_date)) continue;
        for (int s = order.from_idx; s < order.to_idx; ++s) {
            if (s >= 0 && s < train.station_num - 1) occupancy[s] += order.num;
        }
    }
    return 0;
}

// storage: 存储管理器，train: 列车记录，start_date: 运行起始日期，from_idx: 起始站点索引，to_idx: 到达站点索引
// 计算指定区间的可用座位数，返回可用座位数
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

// train: 列车记录，station_idx: 站点索引，depart_date: 出发日期
// 判断列车在指定日期是否可运行，可运行返回true
static bool canRunOnDate(const TrainRecord& train, int station_idx, const Date& depart_date) {
    Date start_date = getRunStartDate(train, station_idx, depart_date);
    return dateInRange(train.sale_begin, train.sale_end, start_date);
}

// command: 包含列车信息的命令对象
// 处理添加列车请求，成功返回"0"，失败返回"-1"
std::string TicketSystem::handleAddTrain(const Command& command) {
    TrainRecord train;
    train.train_id = command.getParam('i');
    train.station_num = std::stoi(command.getParam('n'));
    train.seat_num = std::stoi(command.getParam('m'));
    train.max_seat_num = train.seat_num;
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
    std::vector<std::string> stationParts;
    splitString(command.getParam('s'), '|', stationParts, MAX_STATIONS);
    train.stations = std::move(stationParts);
    train.station_num = static_cast<int>(train.stations.size());

    std::vector<std::string> priceParts;
    splitString(command.getParam('p'), '|', priceParts, MAX_PRICE_SEGMENTS);
    train.prices.clear();
    train.prices.reserve(priceParts.size());
    for (const auto& part : priceParts) train.prices.push_back(std::stoi(part));

    std::vector<std::string> travelParts;
    splitString(command.getParam('t'), '|', travelParts, MAX_TRAVEL_SEGMENTS);
    train.travel_times.clear();
    train.travel_times.reserve(travelParts.size());
    for (const auto& part : travelParts) train.travel_times.push_back(std::stoi(part));

    std::string stopArg = command.getParam('o');
    if (stopArg != "_") {
        std::vector<std::string> stopParts;
        splitString(stopArg, '|', stopParts, MAX_TRAVEL_SEGMENTS);
        train.stopover_times.clear();
        train.stopover_times.reserve(stopParts.size());
        for (const auto& part : stopParts) train.stopover_times.push_back(std::stoi(part));
    } else {
        train.stopover_times.clear();
    }
    if (!storage_.addTrain(train)) return "-1";
    return "0";
}

// command: 包含列车ID的命令对象
// 处理删除列车请求（仅未发布的列车可删除），成功返回"0"，失败返回"-1"
std::string TicketSystem::handleDeleteTrain(const Command& command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train)) return "-1";
    if (train.released) return "-1";
    if (!storage_.removeTrain(id)) return "-1";
    return "0";
}

// command: 包含列车ID的命令对象
// 处理发布列车请求，成功返回"0"，失败返回"-1"
std::string TicketSystem::handleReleaseTrain(const Command& command) {
    std::string id = command.getParam('i');
    TrainRecord train;
    if (!storage_.loadTrain(id, train)) return "-1";
    if (train.released) return "-1";
    train.released = true;
    if (!storage_.updateTrain(train)) return "-1";
    return "0";
}

// command: 包含列车ID和日期的命令对象
// 处理查询列车信息请求，成功返回列车详情，失败返回"-1"
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

// command: 包含出发站、到达站、日期的命令对象
// 处理查询车票请求，成功返回符合条件的列车列表，无结果返回"0"
std::string TicketSystem::handleQueryTicket(const Command& command) {
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    std::vector<TrainRecord> trains;
    if (!storage_.loadTrainsByStation(from, trains)) return "0";
    struct Item { 
        std::string line;
        int key1;
        std::string key2;
    };
    std::vector<Item> items;
    items.reserve(trains.size());
    for (size_t i = 0; i < trains.size(); ++i) {
        TrainRecord& train = trains[i];
        if (!train.released) 
            continue;
        int from_idx = findStation(train, from);
        int to_idx = findStation(train, to);
        if (from_idx < 0 || to_idx < 0 || from_idx >= to_idx)
            continue;
        if (!canRunOnDate(train, from_idx, date))
            continue;
        Date start_date = getRunStartDate(train, from_idx, date);
        DateTime depart = stationDeparture(train, from_idx, start_date);
        DateTime arrive = stationArrival(train, to_idx, start_date);
        int price = routePrice(train, from_idx, to_idx);
        int seats = availableSeats(storage_, train, start_date, from_idx, to_idx);
        int duration = DateTimeUtils::dayOffset(depart.date, arrive.date) * 1440 + (arrive.time.hour * 60 + arrive.time.minute) - (depart.time.hour * 60 + depart.time.minute);
        std::string line = train.train_id + " " + from + " " + depart.toString() 
            + " -> " + to + " " + arrive.toString() + " " + std::to_string(price) 
            + " " + std::to_string(seats);
        if (order == "time") {
            // 按时间排序：主要按耗时，其次按 Train ID
            items.push_back({ line, duration, train.train_id });
        } else {
            // 按价格排序：主要按票价，其次按 Train ID
            items.push_back({ line, price, train.train_id });
        }
    }
    if (items.empty()) {
        return "0";
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.key1 != b.key1) return a.key1 < b.key1;
        return a.key2 < b.key2;
    });

    std::string out = std::to_string(static_cast<int>(items.size()));
    
    for (size_t i = 0; i < items.size(); ++i) {
        out += "\n" + items[i].line;
    }
    return out;
}

// train: 列车记录，station_idx: 站点索引，earliest: 最早允许的出发时间
// 查找列车在指定站点最早可行的出发时间，返回DateTime对象，无可用时间返回无效日期
static DateTime bestSecondDeparture(const TrainRecord& train, int station_idx, const DateTime& earliest) {
    // 最早可行出发时间
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

// command: 包含出发站、到达站、日期的命令对象
// 处理查询中转车票请求，成功返回最优中转方案，无结果返回"0"
std::string TicketSystem::handleQueryTransfer(const Command& command) {
    // 读取命令参数：出发站、到达站、日期、排序方式
    std::string from = command.getParam('s');
    std::string to = command.getParam('t');
    Date date = Date::parse(command.getParam('d'));
    std::string order = command.hasParam('p') ? command.getParam('p') : "time";
    // 加载出发站和到达站的列车记录
    std::vector<TrainRecord> firstTrains;
    if (!storage_.loadTrainsByStation(from, firstTrains)) 
        return "0";
    std::vector<TrainRecord> secondTrains;
    if (!storage_.loadTrainsByStation(to, secondTrains)) 
        return "0";
    // 初始化最优中转方案的变量
    bool found = false;
    int best_main = 0;
    int best_secondary = 0;
    std::string best_line1;
    std::string best_line2;
    std::string best_first_id;
    std::string best_second_id;
    // 遍历出发站的列车，查找符合要求的中转方案
    for (size_t i = 0; i < firstTrains.size(); ++i) {
        TrainRecord& first = firstTrains[i];
        if (!first.released)
            continue;
        int start_idx = findStation(first, from);
        if (start_idx < 0)
            continue;
        // 得到列车的起点站出发时间
        Date first_start = getRunStartDate(first, start_idx, date);
        // 计算列车到达from站的时间
        DateTime first_depart = stationDeparture(first, start_idx, first_start);
        // 判断列车是否在指定日期运行，并且到达时间不晚于查询日期
        if (!(first_depart.date == date) || !dateInRange(first.sale_begin, first.sale_end, first_start))
            continue;
        // 遍历first列车的中间站,查找符合要求的中转方案
        for (int mid = start_idx + 1; mid < first.station_num; ++mid) {
            // 计算第一辆列车到达中转站的时间
            DateTime first_arrive = stationArrival(first, mid, first_start);
            for (size_t j = 0; j < secondTrains.size(); ++j) {
                TrainRecord& second = secondTrains[j];
                if (!second.released) 
                    continue;
                if (first.train_id == second.train_id) 
                    continue;
                int mid_idx = findStation(second, first.stations[mid]);
                int end_idx = findStation(second, to);
                if (mid_idx < 0 || end_idx < 0 || mid_idx >= end_idx) 
                    continue;
                // 查找第二辆列车在中转站最早可行的出发时间
                DateTime second_depart = bestSecondDeparture(second, mid_idx, first_arrive);
                if (second_depart.date.month == 0) 
                    continue;

                Date second_start = getRunStartDate(second, mid_idx, second_depart.date);
                DateTime second_arrive = stationArrival(second, end_idx, second_start);

                int first_price = routePrice(first, start_idx, mid);
                int second_price = routePrice(second, mid_idx, end_idx);
                int seats1 = availableSeats(storage_, first, first_start, start_idx, mid);
                int seats2 = availableSeats(storage_, second, second_start, mid_idx, end_idx);
                // 检查是否有座位可用
                if (seats1 <= 0 || seats2 <= 0)
                    continue;
                int total_price = first_price + second_price;
                int total_time = DateTimeUtils::dayOffset(first_depart.date, second_arrive.date) * 1440 
                    + (second_arrive.time.hour * 60 + second_arrive.time.minute) 
                    - (first_depart.time.hour * 60 + first_depart.time.minute);

                std::string line1 = first.train_id + " " + from + " " + first_depart.toString()
                    + " -> " + first.stations[mid] + " " + first_arrive.toString() + " "
                    + std::to_string(first_price) + " " + std::to_string(seats1);
                std::string line2 = second.train_id + " " + first.stations[mid] + " "
                    + second_depart.toString() + " -> " + to + " " + second_arrive.toString() + " "
                    + std::to_string(second_price) + " " + std::to_string(seats2);

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

// left/right: 待比较的订单记录，descending: 是否降序
// 比较两个订单的时间戳，返回比较结果（true表示left应排在right前面）
static bool compareOrderTimestamp(const OrderRecord& left, const OrderRecord& right, bool descending) {
    if (descending) return left.timestamp > right.timestamp;
    return left.timestamp < right.timestamp;
}

// orders: 订单数组，ids: 订单ID数组，count: 订单数量，descending: 是否降序
// 按时间戳对订单数组进行排序
static void sortOrdersByTimestamp(OrderRecord orders[], int ids[], int count, bool descending) {
    std::vector<int> idx(count);
    for (int i = 0; i < count; ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        if (orders[a].timestamp != orders[b].timestamp) {
            return descending ? orders[a].timestamp > orders[b].timestamp
                               : orders[a].timestamp < orders[b].timestamp;
        }
        return ids[a] < ids[b];
    });
    std::vector<OrderRecord> sortedOrders;
    std::vector<int> sortedIds;
    sortedOrders.reserve(count);
    sortedIds.reserve(count);
    for (int i = 0; i < count; ++i) {
        sortedOrders.push_back(std::move(orders[idx[i]]));
        sortedIds.push_back(ids[idx[i]]);
    }
    for (int i = 0; i < count; ++i) {
        orders[i] = std::move(sortedOrders[i]);
        ids[i] = sortedIds[i];
    }
}

// storage: 存储管理器, username: 用户名，orders: 输出订单向量，ids: 输出订单ID向量，descending: 是否降序
// 加载用户的所有订单并按时间戳排序，成功返回true
static bool loadUserOrders(const StorageManager& storage, const std::string& username,
        std::vector<OrderRecord>& orders, std::vector<int>& ids, bool descending = true) {
    if (!storage.loadOrdersByUser(username, orders, ids)) return false;
    int count = static_cast<int>(orders.size());
    sortOrdersByTimestamp(orders.data(), ids.data(), count, descending);
    return true;
}

// status: 订单状态枚举值
// 将订单状态转换为对应的字符串，返回"success"、"pending"或"refunded"
static std::string orderStatusString(OrderStatus status) {
    if (status == OrderStatus::Success) return "success";
    if (status == OrderStatus::Pending) return "pending";
    return "refunded";
}

// storage: 存储管理器，train: 列车记录，start_date: 运行起始日期，orders: 输出订单向量，ids: 输出订单ID向量，count: 输出订单数量
// 加载指定列车指定日期的所有待处理订单，成功返回true
static bool loadPendingOrders(const StorageManager& storage,
        const TrainRecord& train, const Date& start_date,
        std::vector<OrderRecord>& orders, std::vector<int>& ids, int& count) {
    if (!storage.loadOrdersByTrainDate(train.train_id, start_date, orders, ids)) return false;
    std::vector<OrderRecord> filteredOrders;
    std::vector<int> filteredIds;
    for (size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].train_id != train.train_id) continue;
        if (orders[i].status != OrderStatus::Pending) continue;
        Date orderStart = getRunStartDate(train, orders[i].from_idx, orders[i].date);
        if (!(orderStart == start_date)) continue;
        filteredOrders.push_back(orders[i]);
        filteredIds.push_back(ids[i]);
    }
    orders.swap(filteredOrders);
    ids.swap(filteredIds);
    count = static_cast<int>(orders.size());
    sortOrdersByTimestamp(orders.data(), ids.data(), count, false);
    return true;
}

// command: 包含购票信息的命令对象
// 处理购票请求，成功返回票价或"queue"，失败返回"-1"
std::string TicketSystem::handleBuyTicket(const Command& command) {
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
    int from_idx = findStation(train, from);
    int to_idx = findStation(train, to);
    if (from_idx < 0 || to_idx < 0 || from_idx >= to_idx) 
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

// command: 包含用户名的命令对象
// 处理查询用户订单请求，成功返回订单列表，失败返回"-1"
std::string TicketSystem::handleQueryOrder(const Command& command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user)) return "-1";
    std::vector<OrderRecord> orders;
    std::vector<int> ids;
    if (!loadUserOrders(storage_, user, orders, ids, true)) {
        return "-1";
    }
    int count = static_cast<int>(orders.size());
    std::string out = std::to_string(count);
    for (int i = 0; i < count; ++i) {
        OrderRecord& order = orders[i];
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

// storage: 存储管理器，train: 列车记录，start_date: 运行起始日期
// 处理候补订单队列，将有座位的候补订单转为成功状态
static void processWaitlist(StorageManager& storage, const TrainRecord& train, const Date& start_date) {
    std::vector<OrderRecord> orders;
    std::vector<int> ids;
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

// command: 包含用户名和订单序号的命令对象
// 处理退票请求，成功返回"0"，失败返回"-1"
std::string TicketSystem::handleRefundTicket(const Command& command) {
    std::string user = command.getParam('u');
    if (!checkLogin(user)) return "-1";
    int index = 1;
    if (command.hasParam('n')) index = std::stoi(command.getParam('n'));
    std::vector<OrderRecord> orders;
    std::vector<int> ids;
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
            processWaitlist(storage_, train, run_start);
        }
    }
    return "0";
}

// command: 清理命令对象
// 清理所有数据（删除存储目录），重新初始化系统，返回"0"
std::string TicketSystem::handleClean(const Command& command) {
    std::filesystem::remove_all(storage_.basePath());
    storage_.initialize(storage_.basePath());
    logged_users.clear();
    return "0";
}

// command: 退出命令对象
// 处理退出请求，清空登录用户列表，返回"bye"
std::string TicketSystem::handleExit(const Command& command) {
    logged_users.clear();
    return "bye";
}

} // namespace ticket