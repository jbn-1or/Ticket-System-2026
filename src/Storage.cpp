#include "../include/Storage.hpp"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdio>

namespace ticket {

const char* StoragePaths::USERS = "users.db";
const char* StoragePaths::TRAINS = "trains.db";
const char* StoragePaths::ORDERS = "orders.db";
const char* StoragePaths::WAITLIST = "waitlist.db";
const char* StoragePaths::USER_INDEX = "user_index.idx";
const char* StoragePaths::TRAIN_INDEX = "train_index.idx";
const char* StoragePaths::ORDER_INDEX = "order_index.idx";
const char* StoragePaths::ORDER_USER_INDEX = "order_user_index.idx";
const char* StoragePaths::TRAIN_STATION_INDEX = "train_station_index.idx";
const char* StoragePaths::ORDER_TRAIN_DATE_INDEX = "order_train_date_index.idx";

static std::filesystem::path makeDataPath(const std::string& base, const char* file) {
    return std::filesystem::path(base) / file;
}

static void copyStringField(const std::string& source, char* dest, int size) {
    int len = static_cast<int>(source.size());
    if (len >= size) len = size - 1;
    if (len > 0) memcpy(dest, source.data(), len);
    dest[len] = '\0';
    if (len + 1 < size) memset(dest + len + 1, 0, size - len - 1);
}

static std::string readStringField(const char* source) {
    return std::string(source);
}

static std::string formatOrderKey(int order_id) {
    char buffer[24];
    int len = std::snprintf(buffer, sizeof(buffer), "%020d", order_id);
    return std::string(buffer, len);
}

static std::string formatTrainDateKey(const std::string& train_id, const Date& date) {
    char buffer[48];
    int len = std::snprintf(buffer, sizeof(buffer), "%s|%02d%02d", train_id.c_str(), date.month, date.day);
    return std::string(buffer, len);
}

static int departureOffsetMinutes(const TrainRecord& train, int station_idx) {
    if (station_idx == 0) return 0;
    int minutes = 0;
    for (int i = 0; i < station_idx; ++i) {
        minutes += train.travel_times[i];
        if (i + 1 < station_idx) minutes += train.stopover_times[i];
    }
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

static void encodeUser(const UserRecord& user, BinaryUserRecord& bin) {
    copyStringField(user.username, bin.username, sizeof(bin.username));
    copyStringField(user.password, bin.password, sizeof(bin.password));
    copyStringField(user.name, bin.name, sizeof(bin.name));
    copyStringField(user.mail, bin.mail, sizeof(bin.mail));
    bin.privilege = user.privilege;
    bin.deleted = false;
    memset(bin.padding, 0, sizeof(bin.padding));
}

static void decodeUser(const BinaryUserRecord& bin, UserRecord& user) {
    user.username = readStringField(bin.username);
    user.password = readStringField(bin.password);
    user.name = readStringField(bin.name);
    user.mail = readStringField(bin.mail);
    user.privilege = bin.privilege;
}

static void encodeTrain(const TrainRecord& train, BinaryTrainRecord& bin) {
    copyStringField(train.train_id, bin.train_id, sizeof(bin.train_id));
    bin.station_num = train.station_num;
    bin.seat_num = train.seat_num;

    for (int i = 0; i < MAX_STATIONS; ++i) {
        bin.stations[i][0] = '\0';
    }
    for (int i = 0; i < bin.station_num && i < MAX_STATIONS; ++i) {
        copyStringField(train.stations[i], bin.stations[i], sizeof(bin.stations[i]));
    }

    for (int i = 0; i < MAX_PRICE_SEGMENTS; ++i) {
        bin.prices[i] = 0;
    }
    for (int i = 0; i < std::min(bin.station_num - 1, MAX_PRICE_SEGMENTS); ++i) {
        bin.prices[i] = train.prices[i];
    }

    for (int i = 0; i < MAX_TRAVEL_SEGMENTS; ++i) {
        bin.travel_times[i] = 0;
    }
    for (int i = 0; i < std::min(bin.station_num - 1, MAX_TRAVEL_SEGMENTS); ++i) {
        bin.travel_times[i] = train.travel_times[i];
    }

    for (int i = 0; i < MAX_TRAVEL_SEGMENTS; ++i) {
        bin.stopover_times[i] = 0;
    }
    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    for (int i = 0; i < std::min(stopCount, MAX_TRAVEL_SEGMENTS); ++i) {
        bin.stopover_times[i] = train.stopover_times[i];
    }

    bin.start_hour = train.start_time.hour;
    bin.start_minute = train.start_time.minute;
    bin.sale_begin_month = train.sale_begin.month;
    bin.sale_begin_day = train.sale_begin.day;
    bin.sale_end_month = train.sale_end.month;
    bin.sale_end_day = train.sale_end.day;
    bin.type = train.type;
    bin.released = train.released;
    bin.deleted = false;
    memset(bin.padding, 0, sizeof(bin.padding));
}

static void decodeTrain(const BinaryTrainRecord& bin, TrainRecord& train) {
    train.train_id = readStringField(bin.train_id);
    train.station_num = bin.station_num;
    train.seat_num = bin.seat_num;
    train.stations.clear();
    train.stations.reserve(train.station_num);
    for (int i = 0; i < train.station_num && i < MAX_STATIONS; ++i) {
        train.stations.push_back(readStringField(bin.stations[i]));
    }
    int segmentCount = std::max(0, train.station_num - 1);
    train.prices.clear();
    train.prices.reserve(segmentCount);
    train.travel_times.clear();
    train.travel_times.reserve(segmentCount);
    for (int i = 0; i < segmentCount && i < MAX_PRICE_SEGMENTS; ++i) {
        train.prices.push_back(bin.prices[i]);
        train.travel_times.push_back(bin.travel_times[i]);
    }
    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    train.stopover_times.clear();
    train.stopover_times.reserve(stopCount);
    for (int i = 0; i < stopCount && i < MAX_TRAVEL_SEGMENTS; ++i) {
        train.stopover_times.push_back(bin.stopover_times[i]);
    }
    train.start_time = Time(bin.start_hour, bin.start_minute);
    train.sale_begin = Date(bin.sale_begin_month, bin.sale_begin_day);
    train.sale_end = Date(bin.sale_end_month, bin.sale_end_day);
    train.type = bin.type;
    train.released = bin.released;
}

static void encodeOrder(const OrderRecord& order, BinaryOrderRecord& bin) {
    bin.order_id = order.order_id;
    copyStringField(order.username, bin.username, sizeof(bin.username));
    copyStringField(order.train_id, bin.train_id, sizeof(bin.train_id));
    bin.date_month = order.date.month;
    bin.date_day = order.date.day;
    bin.from_idx = order.from_idx;
    bin.to_idx = order.to_idx;
    bin.num = order.num;
    bin.price = order.price;
    bin.status = static_cast<int>(order.status);
    bin.is_waiting = order.is_waiting;
    bin.deleted = false;
    memset(bin.padding, 0, sizeof(bin.padding));
    bin.timestamp = order.timestamp;
}

static void decodeOrder(const BinaryOrderRecord& bin, OrderRecord& order) {
    order.order_id = bin.order_id;
    order.username = readStringField(bin.username);
    order.train_id = readStringField(bin.train_id);
    order.date = Date(bin.date_month, bin.date_day);
    order.from_idx = bin.from_idx;
    order.to_idx = bin.to_idx;
    order.num = bin.num;
    order.price = bin.price;
    order.status = static_cast<OrderStatus>(bin.status);
    order.is_waiting = bin.is_waiting;
    order.timestamp = bin.timestamp;
}

StorageManager::StorageManager()
    : base_path_(), userIndex_(nullptr), trainIndex_(nullptr), orderIndex_(nullptr), orderUserIndex_(nullptr), trainStationIndex_(nullptr), orderTrainDateIndex_(nullptr), next_order_id_(1) {}

StorageManager::~StorageManager() {
    delete userIndex_; 
    delete trainIndex_;
    delete orderIndex_;
    delete orderUserIndex_;
    delete trainStationIndex_;
    delete orderTrainDateIndex_;
}

bool StorageManager::initialize(const std::string& basePath) {
    base_path_ = basePath;
    std::filesystem::create_directories(base_path_);

    auto userPath = makeDataPath(base_path_, StoragePaths::USERS);
    auto trainPath = makeDataPath(base_path_, StoragePaths::TRAINS);
    auto orderPath = makeDataPath(base_path_, StoragePaths::ORDERS);
    auto userIndexPath = makeDataPath(base_path_, StoragePaths::USER_INDEX);
    auto trainIndexPath = makeDataPath(base_path_, StoragePaths::TRAIN_INDEX);
    auto orderIndexPath = makeDataPath(base_path_, StoragePaths::ORDER_INDEX);
    auto orderUserIndexPath = makeDataPath(base_path_, StoragePaths::ORDER_USER_INDEX);
    auto trainStationIndexPath = makeDataPath(base_path_, StoragePaths::TRAIN_STATION_INDEX);
    auto orderTrainDateIndexPath = makeDataPath(base_path_, StoragePaths::ORDER_TRAIN_DATE_INDEX);

    userRiver_.initialise(userPath.string());
    trainRiver_.initialise(trainPath.string());
    orderRiver_.initialise(orderPath.string());

    delete userIndex_;
    delete trainIndex_;
    delete orderIndex_;
    delete orderUserIndex_;
    delete trainStationIndex_;
    delete orderTrainDateIndex_;
    userIndex_ = new BPlusTree<int>(userIndexPath.string());
    trainIndex_ = new BPlusTree<int>(trainIndexPath.string());
    orderIndex_ = new BPlusTree<int>(orderIndexPath.string());
    orderUserIndex_ = new BPlusTree<int>(orderUserIndexPath.string());
    trainStationIndex_ = new BPlusTree<int>(trainStationIndexPath.string());
    orderTrainDateIndex_ = new BPlusTree<int>(orderTrainDateIndexPath.string());

    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    next_order_id_ = totalOrders + 1;
    return true;
}

std::string StorageManager::basePath() const {
    return base_path_;
}

bool StorageManager::addUser(const UserRecord& user) {
    int position;
    if (userIndex_->find(user.username.c_str(), position)) return false;
    BinaryUserRecord bin;
    encodeUser(user, bin);
    int offset = userRiver_.write(bin);
    if (offset < 0) return false;
    if (!userIndex_->insert(user.username.c_str(), offset)) return false;
    int userCount = 0;
    userRiver_.get_info(userCount, 1);
    userRiver_.write_info(userCount + 1, 1);
    return true;
}

bool StorageManager::loadUser(const std::string& username, UserRecord& user) const {
    int position;
    if (!userIndex_->find(username.c_str(), position)) return false;
    BinaryUserRecord bin;
    userRiver_.read(bin, position);
    if (bin.deleted) return false;
    decodeUser(bin, user);
    return true;
}

bool StorageManager::updateUser(const UserRecord& user) {
    int position;
    if (!userIndex_->find(user.username.c_str(), position)) return false;
    BinaryUserRecord bin;
    encodeUser(user, bin);
    userRiver_.update(bin, position);
    return true;
}

bool StorageManager::addTrain(const TrainRecord& train) {
    int position;
    if (trainIndex_->find(train.train_id.c_str(), position)) return false;
    BinaryTrainRecord bin;
    encodeTrain(train, bin);
    int offset = trainRiver_.write(bin);
    if (offset < 0) return false;
    if (!trainIndex_->insert(train.train_id.c_str(), offset)) return false;
    for (int i = 0; i < train.station_num; ++i) {
        if (!trainStationIndex_->insert(train.stations[i].c_str(), offset)) return false;
    }
    int trainCount = 0;
    trainRiver_.get_info(trainCount, 1);
    trainRiver_.write_info(trainCount + 1, 1);
    return true;
}

bool StorageManager::loadTrain(const std::string& train_id, TrainRecord& train) const {
    int position;
    if (!trainIndex_->find(train_id.c_str(), position)) return false;
    BinaryTrainRecord bin;
    trainRiver_.read(bin, position);
    if (bin.deleted) return false;
    decodeTrain(bin, train);
    return true;
}

bool StorageManager::updateTrain(const TrainRecord& train) {
    int position;
    if (!trainIndex_->find(train.train_id.c_str(), position)) return false;
    BinaryTrainRecord bin;
    encodeTrain(train, bin);
    trainRiver_.update(bin, position);
    return true;
}

bool StorageManager::removeTrain(const std::string& train_id) {
    int position;
    if (!trainIndex_->find(train_id.c_str(), position)) return false;
    BinaryTrainRecord bin;
    trainRiver_.read(bin, position);
    if (bin.deleted) return false;
    bin.deleted = true;
    trainRiver_.update(bin, position);
    trainIndex_->remove(train_id.c_str(), position);
    for (int i = 0; i < bin.station_num; ++i) {
        trainStationIndex_->remove(bin.stations[i], position);
    }
    return true;
}

bool StorageManager::addOrder(const OrderRecord& order) {
    OrderRecord copy = order;
    copy.order_id = next_order_id_;
    BinaryOrderRecord bin;
    encodeOrder(copy, bin);
    int offset = orderRiver_.write(bin);
    if (offset < 0) return false;
    std::string key = formatOrderKey(copy.order_id);
    if (!orderIndex_->insert(key.c_str(), offset)) return false;
    if (!orderUserIndex_->insert(copy.username.c_str(), offset)) return false;
    TrainRecord train;
    if (!loadTrain(copy.train_id, train)) return false;
    int depart_offset = train.start_time.hour * 60 + train.start_time.minute + departureOffsetMinutes(train, copy.from_idx);
    int day_offset = depart_offset / 1440;
    Date run_start_date = addDays(copy.date, -day_offset);
    std::string trainDateKey = formatTrainDateKey(copy.train_id, run_start_date);
    if (!orderTrainDateIndex_->insert(trainDateKey.c_str(), offset)) return false;
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    orderRiver_.write_info(totalOrders + 1, 1);
    next_order_id_++;
    return true;
}

bool StorageManager::loadOrder(int order_id, OrderRecord& order) const {
    std::string key = formatOrderKey(order_id);
    int position;
    if (!orderIndex_->find(key.c_str(), position)) return false;
    BinaryOrderRecord bin;
    orderRiver_.read(bin, position);
    if (bin.deleted) return false;
    decodeOrder(bin, order);
    return true;
}

bool StorageManager::loadOrdersByUser(const std::string& username, std::vector<OrderRecord>& orders, std::vector<int>& ids) const {
    orders.clear();
    ids.clear();
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    if (totalOrders <= 0) return true;

    std::vector<int> offsets(totalOrders);
    int count = orderUserIndex_->findAll(username.c_str(), offsets.data(), totalOrders);
    for (int i = 0; i < count; ++i) {
        BinaryOrderRecord bin;
        orderRiver_.read(bin, offsets[i]);
        if (bin.deleted) continue;
        OrderRecord order;
        decodeOrder(bin, order);
        orders.push_back(order);
        ids.push_back(bin.order_id);
    }
    return true;
}

bool StorageManager::loadTrainsByStation(const std::string& station, std::vector<TrainRecord>& trains) const {
    trains.clear();
    int totalTrains = 0;
    trainRiver_.get_info(totalTrains, 1);
    if (totalTrains <= 0) return true;

    std::vector<int> offsets(totalTrains);
    int count = trainStationIndex_->findAll(station.c_str(), offsets.data(), totalTrains);
    for (int i = 0; i < count; ++i) {
        BinaryTrainRecord bin;
        trainRiver_.read(bin, offsets[i]);
        if (bin.deleted) continue;
        TrainRecord train;
        decodeTrain(bin, train);
        trains.push_back(train);
    }
    return true;
}

bool StorageManager::loadOrdersByTrainDate(const std::string& train_id, const Date& date, std::vector<OrderRecord>& orders, std::vector<int>& ids) const {
    orders.clear();
    ids.clear();
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    if (totalOrders <= 0) return true;

    std::string key = formatTrainDateKey(train_id, date);
    std::vector<int> offsets(totalOrders);
    int count = orderTrainDateIndex_->findAll(key.c_str(), offsets.data(), totalOrders);
    for (int i = 0; i < count; ++i) {
        BinaryOrderRecord bin;
        orderRiver_.read(bin, offsets[i]);
        if (bin.deleted) continue;
        OrderRecord order;
        decodeOrder(bin, order);
        orders.push_back(order);
        ids.push_back(bin.order_id);
    }
    return true;
}

bool StorageManager::updateOrder(int order_id, const OrderRecord& order) {
    std::string key = formatOrderKey(order_id);
    int position;
    if (!orderIndex_->find(key.c_str(), position)) return false;
    BinaryOrderRecord bin;
    encodeOrder(order, bin);
    bin.order_id = order_id;
    orderRiver_.update(bin, position);
    return true;
}

bool StorageManager::loadAllTrains(std::vector<TrainRecord>& trains) const {
    trains.clear();
    int total = 0;
    trainRiver_.get_info(total, 1);
    if (total <= 0) return true;
    trains.reserve(total);
    int offsetBase = sizeof(int);
    for (int i = 0; i < total; ++i) {
        int position = offsetBase + i * static_cast<int>(sizeof(BinaryTrainRecord));
        BinaryTrainRecord bin;
        trainRiver_.read(bin, position);
        if (bin.deleted) continue;
        TrainRecord train;
        decodeTrain(bin, train);
        trains.push_back(train);
    }
    return true;
}

bool StorageManager::loadAllOrders(std::vector<OrderRecord>& orders, std::vector<int>& ids) const {
    orders.clear();
    ids.clear();
    int total = 0;
    orderRiver_.get_info(total, 1);
    if (total <= 0) return true;
    orders.reserve(total);
    ids.reserve(total);
    int offsetBase = sizeof(int);
    for (int i = 0; i < total; ++i) {
        int position = offsetBase + i * static_cast<int>(sizeof(BinaryOrderRecord));
        BinaryOrderRecord bin;
        orderRiver_.read(bin, position);
        if (bin.deleted) continue;
        OrderRecord order;
        decodeOrder(bin, order);
        orders.push_back(order);
        ids.push_back(bin.order_id);
    }
    return true;
}

bool StorageManager::enqueueWaitlist(const WaitlistRecord& wait) {
    auto path = makeDataPath(base_path_, StoragePaths::WAITLIST);
    std::ofstream ofs(path, std::ios::binary | std::ios::app);
    if (!ofs.is_open()) return false;
    ofs << wait.username << "|" << wait.train_id << "|" << wait.date.toString() << "|" << wait.from_idx << "|" << wait.to_idx << "|" << wait.num << "|" << wait.timestamp << "\n";
    return true;
}

bool StorageManager::loadNextWaitlist(WaitlistRecord& wait) const {
    auto path = makeDataPath(base_path_, StoragePaths::WAITLIST);
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return false;
    std::string line;
    bool found = false;
    int earliest = 0;
    WaitlistRecord candidate;
    while (std::getline(ifs, line)) {
        std::string fields[7];
        int countField = 0;
        std::string current;
        for (size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || line[i] == '|') {
                if (countField < 7) fields[countField++] = current;
                current.clear();
            } else {
                current.push_back(line[i]);
            }
        }
        if (countField < 7) continue;
        WaitlistRecord item;
        item.username = fields[0];
        item.train_id = fields[1];
        item.date = Date::parse(fields[2]);
        item.from_idx = std::stoi(fields[3]);
        item.to_idx = std::stoi(fields[4]);
        item.num = std::stoi(fields[5]);
        item.timestamp = std::stoi(fields[6]);
        if (!found || item.timestamp < earliest) {
            found = true;
            earliest = item.timestamp;
            candidate = item;
        }
    }
    if (!found) return false;
    wait = candidate;
    return true;
}

bool StorageManager::hasAnyUser() const {
    int count = 0;
    userRiver_.get_info(count, 1);
    return count > 0;
}

} // namespace ticket
