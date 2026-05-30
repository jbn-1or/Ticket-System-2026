#include "../include/Storage.hpp"
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
    for (int i = 0; i < train.station_num; ++i) {
        copyStringField(train.stations[i], bin.stations[i], sizeof(bin.stations[i]));
    }
    for (int i = 0; i < train.station_num - 1; ++i) {
        bin.prices[i] = train.prices[i];
        bin.travel_times[i] = train.travel_times[i];
    }
    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    for (int i = 0; i < stopCount; ++i) {
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
    for (int i = 0; i < train.station_num; ++i) {
        train.stations[i] = readStringField(bin.stations[i]);
    }
    for (int i = 0; i < train.station_num - 1; ++i) {
        train.prices[i] = bin.prices[i];
        train.travel_times[i] = bin.travel_times[i];
    }
    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    for (int i = 0; i < stopCount; ++i) {
        train.stopover_times[i] = bin.stopover_times[i];
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
    : base_path_(), userIndex_(nullptr), trainIndex_(nullptr), orderIndex_(nullptr), next_order_id_(1) {}

StorageManager::~StorageManager() {
    delete userIndex_; 
    delete trainIndex_;
    delete orderIndex_;
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

    userRiver_.initialise(userPath.string());
    trainRiver_.initialise(trainPath.string());
    orderRiver_.initialise(orderPath.string());

    delete userIndex_;
    delete trainIndex_;
    delete orderIndex_;
    userIndex_ = new BPlusTree<int>(userIndexPath.string());
    trainIndex_ = new BPlusTree<int>(trainIndexPath.string());
    orderIndex_ = new BPlusTree<int>(orderIndexPath.string());

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

bool StorageManager::loadAllTrains(TrainRecord trains[], int& count) const {
    count = 0;
    int total = 0;
    trainRiver_.get_info(total, 1);
    if (total <= 0) return true;
    int offsetBase = sizeof(int);
    for (int i = 0; i < total && count < 10000; ++i) {
        int position = offsetBase + i * static_cast<int>(sizeof(BinaryTrainRecord));
        BinaryTrainRecord bin;
        trainRiver_.read(bin, position);
        if (bin.deleted) continue;
        decodeTrain(bin, trains[count++]);
    }
    return true;
}

bool StorageManager::loadAllOrders(OrderRecord orders[], int ids[], int& count) const {
    count = 0;
    int total = 0;
    orderRiver_.get_info(total, 1);
    if (total <= 0) return true;
    int offsetBase = sizeof(int);
    for (int i = 0; i < total && count < 10000; ++i) {
        int position = offsetBase + i * static_cast<int>(sizeof(BinaryOrderRecord));
        BinaryOrderRecord bin;
        orderRiver_.read(bin, position);
        if (bin.deleted) continue;
        decodeOrder(bin, orders[count]);
        ids[count] = bin.order_id;
        count++;
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
