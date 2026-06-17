#include "Storage.hpp"
#include "TrainUtils.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace ticket {

// 构造函数
UserRecord::UserRecord() : username(), password(), name(), mail(), privilege(0) {}

TrainRecord::TrainRecord()
    : train_id(), station_num(0), seat_num(0), max_seat_num(0), start_time(), sale_begin(), sale_end(), type(' '), released(false) {
}

OrderRecord::OrderRecord()
    : order_id(0), username(), train_id(), date(), from_idx(0), to_idx(0), num(0),
      price(0), status(OrderStatus::Success), is_waiting(false), timestamp(0) {}

// 路径常量
const char *StoragePaths::USERS = "users.db";
const char *StoragePaths::TRAINS = "trains.db";
const char *StoragePaths::ORDERS = "orders.db";
const char *StoragePaths::WAITLIST = "waitlist.db";
const char *StoragePaths::USER_INDEX = "user_index.idx";
const char *StoragePaths::TRAIN_INDEX = "train_index.idx";
const char *StoragePaths::ORDER_INDEX = "order_index.idx";
const char *StoragePaths::ORDER_USER_INDEX = "order_user_index.idx";
const char *StoragePaths::TRAIN_STATION_INDEX = "train_station_index.idx";
const char *StoragePaths::TRAIN_STATION_PAIR_INDEX = "train_station_pair_index.idx";
const char *StoragePaths::ORDER_TRAIN_DATE_INDEX = "order_train_date_index.idx";

// base: 基础目录路径，file: 文件名
// 拼接基础路径和文件名，返回完整的文件路径
static std::filesystem::path makeDataPath(const std::string &base, const char *file) {
    return std::filesystem::path(base) / file;
}

// source: 源字符串，dest: 目标字符数组，size: 目标数组大小
// 将字符串复制到字符数组
static void copyStringField(const std::string &source, char *dest, int size) {
    int len = static_cast<int>(source.size());
    if (len >= size) {
        len = size - 1;
    }
    // 复制字符串内容
    if (len > 0) {
        memcpy(dest, source.c_str(), len);
    }
    dest[len] = '\0';
    // 其余空间清零
    if (len + 1 < size) {
        memset(dest + len + 1, 0, size - len - 1);
    }
}

// source: 源字符数组
// 从字符数组读取字符串，返回std::string对象
static std::string readStringField(const char *source) {
    return std::string(source);
}

// order_id: 订单ID
// 将订单ID化为固定长度的字符串键，返回格式化后的字符串
static std::string formatOrderKey(int order_id) {
    std::string s = std::to_string(order_id);
    if (s.size() < 20) {
        s = std::string(20 - s.size(), '0') + s;
    }
    return s;
}

// from: 出发站名，to: 到达站名
// 格式化两站为站对索引键，返回"from|to"格式的字符串
static std::string formatStationPairKey(const std::string &from, const std::string &to) {
    std::string result;
    result.reserve(from.size() + to.size() + 2);
    result = from;
    result += '|';
    result += to;
    return result;
}

// train_id: 列车ID，date: 日期
// 格式化列车ID和日期为索引键，返回格式化后的字符串
static std::string formatTrainDateKey(const std::string &train_id, const Date &date) {
    std::string result;
    result.reserve(train_id.size() + 6); // train_id + "|" + MM + DD
    result = train_id;
    result += '|';
    if (date.month < 10)
        result += '0';
    result += std::to_string(date.month);
    if (date.day < 10)
        result += '0';
    result += std::to_string(date.day);
    return result;
}

// user: 用户记录，bin: 输出的二进制用户记录
// 将用户记录转为二进制格式
static void encodeUser(const UserRecord &user, BinaryUserRecord &bin) {
    copyStringField(user.username, bin.username, sizeof(bin.username));
    copyStringField(user.password, bin.password, sizeof(bin.password));
    copyStringField(user.name, bin.name, sizeof(bin.name));
    copyStringField(user.mail, bin.mail, sizeof(bin.mail));
    bin.privilege = user.privilege;
    bin.deleted = false;
    memset(bin.padding, 0, sizeof(bin.padding));
}

// bin: 二进制用户记录，user: 输出的用户记录
// 将二进制用户记录转为用户记录
static void decodeUser(const BinaryUserRecord &bin, UserRecord &user) {
    user.username = readStringField(bin.username);
    user.password = readStringField(bin.password);
    user.name = readStringField(bin.name);
    user.mail = readStringField(bin.mail);
    user.privilege = bin.privilege;
}

// train: 列车记录，bin: 输出的二进制列车记录
// 将列车记录转为二进制格式
static void encodeTrain(const TrainRecord &train, BinaryTrainRecord &bin) {
    copyStringField(train.train_id, bin.train_id, sizeof(bin.train_id));
    bin.station_num = train.station_num;
    bin.seat_num = train.seat_num;
    bin.max_seat_num = train.max_seat_num;

    // 只写入有数据的站名，其余位置清零
    int sn = bin.station_num > MAX_STATIONS ? MAX_STATIONS : bin.station_num;
    for (int i = 0; i < sn; ++i) {
        copyStringField(train.stations[i], bin.stations[i], sizeof(bin.stations[i]));
    }
    for (int i = sn; i < MAX_STATIONS; ++i) {
        bin.stations[i][0] = '\0';
    }

    // 只写入有数据的区间，其余位置清零
    int segCount = sn - 1;
    if (segCount < 0)
        segCount = 0;
    if (segCount > MAX_PRICE_SEGMENTS)
        segCount = MAX_PRICE_SEGMENTS;
    for (int i = 0; i < segCount; ++i) {
        bin.prices[i] = train.prices[i];
        bin.travel_times[i] = train.travel_times[i];
    }
    for (int i = segCount; i < MAX_PRICE_SEGMENTS; ++i) {
        bin.prices[i] = 0;
    }
    for (int i = segCount; i < MAX_TRAVEL_SEGMENTS; ++i) {
        bin.travel_times[i] = 0;
    }

    // 只写入有数据的停留时间
    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    if (stopCount > MAX_TRAVEL_SEGMENTS)
        stopCount = MAX_TRAVEL_SEGMENTS;
    for (int i = 0; i < stopCount; ++i) {
        bin.stopover_times[i] = train.stopover_times[i];
    }
    for (int i = stopCount; i < MAX_TRAVEL_SEGMENTS; ++i) {
        bin.stopover_times[i] = 0;
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

// bin: 二进制列车记录，train: 输出的列车记录
// 将二进制列车记录转为列车记录
static void decodeTrain(const BinaryTrainRecord &bin, TrainRecord &train) {
    train.train_id = readStringField(bin.train_id);
    train.station_num = bin.station_num;
    train.seat_num = bin.seat_num;
    train.max_seat_num = bin.max_seat_num;

    int sn = bin.station_num;
    if (sn > MAX_STATIONS)
        sn = MAX_STATIONS;
    train.stations.clear();
    for (int i = 0; i < sn; ++i) {
        train.stations.push_back(readStringField(bin.stations[i]));
    }

    int segCount = train.station_num - 1;
    if (segCount < 0)
        segCount = 0;
    if (segCount > MAX_PRICE_SEGMENTS)
        segCount = MAX_PRICE_SEGMENTS;
    train.prices.clear();
    train.travel_times.clear();
    for (int i = 0; i < segCount; ++i) {
        train.prices.push_back(bin.prices[i]);
        train.travel_times.push_back(bin.travel_times[i]);
    }

    int stopCount = train.station_num > 1 ? train.station_num - 2 : 0;
    if (stopCount > MAX_TRAVEL_SEGMENTS)
        stopCount = MAX_TRAVEL_SEGMENTS;
    train.stopover_times.clear();
    for (int i = 0; i < stopCount; ++i) {
        train.stopover_times.push_back(bin.stopover_times[i]);
    }
    train.start_time = Time(bin.start_hour, bin.start_minute);
    train.sale_begin = Date(bin.sale_begin_month, bin.sale_begin_day);
    train.sale_end = Date(bin.sale_end_month, bin.sale_end_day);
    train.type = bin.type;
    train.released = bin.released;
}

// order: 订单记录，bin: 输出的二进制订单记录
// 将订单记录转为二进制格式
static void encodeOrder(const OrderRecord &order, BinaryOrderRecord &bin) {
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

// bin: 二进制订单记录，order: 输出的订单记录
// 将二进制订单记录转为订单记录
static void decodeOrder(const BinaryOrderRecord &bin, OrderRecord &order) {
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

// 默认构造函数：初始化StorageManager对象
// 初始化所有索引指针为nullptr，订单ID从1开始
StorageManager::StorageManager()
    : base_path_(), userIndex_(nullptr), trainIndex_(nullptr), orderIndex_(nullptr), orderUserIndex_(nullptr), trainStationIndex_(nullptr), trainStationPairIndex_(nullptr), orderTrainDateIndex_(nullptr), next_order_id_(1) {}

// 析构函数：清理StorageManager资源
// 释放所有索引对象的内存
StorageManager::~StorageManager() {
    delete userIndex_;
    delete trainIndex_;
    delete orderIndex_;
    delete orderUserIndex_;
    delete trainStationIndex_;
    delete orderTrainDateIndex_;
}

// basePath: 数据存储目录路径
// 初始化存储管理器，创建必要的文件和索引，成功返回true
bool StorageManager::initialize(const std::string &basePath) {
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
    auto trainStationPairIndexPath = makeDataPath(base_path_, StoragePaths::TRAIN_STATION_PAIR_INDEX);
    auto orderTrainDateIndexPath = makeDataPath(base_path_, StoragePaths::ORDER_TRAIN_DATE_INDEX);

    userRiver_.initialise(userPath.string());
    trainRiver_.initialise(trainPath.string());
    orderRiver_.initialise(orderPath.string());

    delete userIndex_;
    delete trainIndex_;
    delete orderIndex_;
    delete orderUserIndex_;
    delete trainStationIndex_;
    delete trainStationPairIndex_;
    delete orderTrainDateIndex_;
    userIndex_ = new BPlusTree<int>(userIndexPath.string());
    trainIndex_ = new BPlusTree<int>(trainIndexPath.string());
    orderIndex_ = new BPlusTree<int>(orderIndexPath.string());
    orderUserIndex_ = new BPlusTree<int>(orderUserIndexPath.string());
    trainStationIndex_ = new BPlusTree<int>(trainStationIndexPath.string());
    trainStationPairIndex_ = new BPlusTree<int>(trainStationPairIndexPath.string());
    orderTrainDateIndex_ = new BPlusTree<int>(orderTrainDateIndexPath.string());

    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    next_order_id_ = totalOrders + 1;

    // 无条件重建站对索引：遍历所有已有列车，确保都有站对索引条目
    {
        int totalTrains = 0;
        trainRiver_.get_info(totalTrains, 1);
        if (totalTrains > 0) {
            int offsetBase = sizeof(int);
            for (int i = 0; i < totalTrains; ++i) {
                int position = offsetBase + i * static_cast<int>(sizeof(BinaryTrainRecord));
                BinaryTrainRecord bin;
                trainRiver_.read(bin, position);
                if (bin.deleted)
                    continue;
                for (int a = 0; a < bin.station_num; ++a) {
                    for (int b = a + 1; b < bin.station_num; ++b) {
                        std::string pairKey = formatStationPairKey(
                            readStringField(bin.stations[a]),
                            readStringField(bin.stations[b]));
                        trainStationPairIndex_->insert(pairKey.c_str(), position);
                    }
                }
            }
        }
    }

    return true;
}

// 无参数
// 获取数据存储基础路径，返回路径字符串
std::string StorageManager::basePath() const {
    return base_path_;
}

// user: 要添加的用户记录
// 添加用户到存储，用户名已存在返回false，成功返回true
bool StorageManager::addUser(const UserRecord &user) {
    int position;
    if (userIndex_->find(user.username.c_str(), position))
        return false;
    BinaryUserRecord bin;
    encodeUser(user, bin);
    int offset = userRiver_.write(bin);
    if (offset < 0)
        return false;
    if (!userIndex_->insert(user.username.c_str(), offset))
        return false;
    int userCount = 0;
    userRiver_.get_info(userCount, 1);
    userRiver_.write_info(userCount + 1, 1);
    return true;
}

// username: 用户名，user: 输出的用户记录
// 根据用户名加载用户记录，用户不存在返回false，成功返回true
bool StorageManager::loadUser(const std::string &username, UserRecord &user) const {
    int position;
    if (!userIndex_->find(username.c_str(), position))
        return false;
    BinaryUserRecord bin;
    userRiver_.read(bin, position);
    if (bin.deleted)
        return false;
    decodeUser(bin, user);
    return true;
}

// user: 更新后的用户记录
// 更新用户信息，用户不存在返回false，成功返回true
bool StorageManager::updateUser(const UserRecord &user) {
    int position;
    if (!userIndex_->find(user.username.c_str(), position))
        return false;
    BinaryUserRecord bin;
    encodeUser(user, bin);
    userRiver_.update(bin, position);
    return true;
}

// train: 要添加的列车记录
// 添加列车到存储，列车ID已存在返回false，成功返回true
bool StorageManager::addTrain(const TrainRecord &train) {
    int position;
    if (trainIndex_->find(train.train_id.c_str(), position))
        return false;
    BinaryTrainRecord bin;
    encodeTrain(train, bin);
    int offset = trainRiver_.write(bin);
    if (offset < 0)
        return false;
    if (!trainIndex_->insert(train.train_id.c_str(), offset))
        return false;
    // 插入单站索引和站对索引
    for (int i = 0; i < train.station_num; ++i) {
        if (!trainStationIndex_->insert(train.stations[i].c_str(), offset))
            return false;
        // 对于每个后面的站，插入 (站i, 站j) 的站对索引
        for (int j = i + 1; j < train.station_num; ++j) {
            std::string pairKey = formatStationPairKey(train.stations[i], train.stations[j]);
            if (!trainStationPairIndex_->insert(pairKey.c_str(), offset))
                return false;
        }
    }
    int trainCount = 0;
    trainRiver_.get_info(trainCount, 1);
    trainRiver_.write_info(trainCount + 1, 1);
    return true;
}

// train_id: 列车ID，train: 输出的列车记录
// 根据列车ID加载列车记录，列车不存在返回false，成功返回true
bool StorageManager::loadTrain(const std::string &train_id, TrainRecord &train) const {
    int position;
    if (!trainIndex_->find(train_id.c_str(), position))
        return false;
    BinaryTrainRecord bin;
    trainRiver_.read(bin, position);
    if (bin.deleted)
        return false;
    decodeTrain(bin, train);
    return true;
}

// train: 更新后的列车记录
// 更新列车信息，列车不存在返回false，成功返回true
bool StorageManager::updateTrain(const TrainRecord &train) {
    int position;
    if (!trainIndex_->find(train.train_id.c_str(), position))
        return false;
    BinaryTrainRecord bin;
    encodeTrain(train, bin);
    trainRiver_.update(bin, position);
    return true;
}

// train_id: 要删除的列车ID
// 删除列车（软删除），列车不存在或已删除返回false，成功返回true
bool StorageManager::removeTrain(const std::string &train_id) {
    int position;
    if (!trainIndex_->find(train_id.c_str(), position))
        return false;
    BinaryTrainRecord bin;
    trainRiver_.read(bin, position);
    if (bin.deleted)
        return false;
    bin.deleted = true;
    trainRiver_.update(bin, position);
    trainIndex_->remove(train_id.c_str(), position);
    // 删除单站索引和站对索引
    for (int i = 0; i < bin.station_num; ++i) {
        trainStationIndex_->remove(bin.stations[i], position);
        for (int j = i + 1; j < bin.station_num; ++j) {
            std::string pairKey = formatStationPairKey(
                readStringField(bin.stations[i]),
                readStringField(bin.stations[j]));
            trainStationPairIndex_->remove(pairKey.c_str(), position);
        }
    }
    return true;
}

// order: 要添加的订单记录
// 添加订单到存储，自动分配订单ID，成功返回true
bool StorageManager::addOrder(const OrderRecord &order) {
    OrderRecord copy = order;
    copy.order_id = next_order_id_;
    BinaryOrderRecord bin;
    encodeOrder(copy, bin);
    int offset = orderRiver_.write(bin);
    if (offset < 0)
        return false;
    std::string key = formatOrderKey(copy.order_id);
    if (!orderIndex_->insert(key.c_str(), offset))
        return false;
    if (!orderUserIndex_->insert(copy.username.c_str(), offset))
        return false;
    TrainRecord train;
    if (!loadTrain(copy.train_id, train))
        return false;
    int depart_offset = train.start_time.hour * 60 + train.start_time.minute + departureOffsetMinutes(train, copy.from_idx);
    int day_offset = depart_offset / 1440;
    Date run_start_date = addDays(copy.date, -day_offset);
    std::string trainDateKey = formatTrainDateKey(copy.train_id, run_start_date);
    if (!orderTrainDateIndex_->insert(trainDateKey.c_str(), offset))
        return false;
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    orderRiver_.write_info(totalOrders + 1, 1);
    next_order_id_++;
    return true;
}

// order_id: 订单ID，order: 输出的订单记录
// 根据订单ID加载订单记录，订单不存在返回false，成功返回true
bool StorageManager::loadOrder(int order_id, OrderRecord &order) const {
    std::string key = formatOrderKey(order_id);
    int position;
    if (!orderIndex_->find(key.c_str(), position))
        return false;
    BinaryOrderRecord bin;
    orderRiver_.read(bin, position);
    if (bin.deleted)
        return false;
    decodeOrder(bin, order);
    return true;
}

// username: 用户名，orders: 输出订单向量，ids: 输出订单ID向量
// 加载指定用户的所有订单，成功返回true
bool StorageManager::loadOrdersByUser(const std::string &username, sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids) const {
    orders.clear();
    ids.clear();
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    if (totalOrders <= 0)
        return true;

    int *offsets = new int[totalOrders];
    int count = orderUserIndex_->findAll(username.c_str(), offsets, totalOrders);
    for (int i = 0; i < count; ++i) {
        BinaryOrderRecord bin;
        orderRiver_.read(bin, offsets[i]);
        if (bin.deleted)
            continue;
        OrderRecord order;
        decodeOrder(bin, order);
        orders.push_back(order);
        ids.push_back(bin.order_id);
    }
    delete[] offsets;
    return true;
}

// station: 站点名称，trains: 输出列车向量
// 加载经过指定站点的所有列车，成功返回true
bool StorageManager::loadTrainsByStation(const std::string &station, sjtu::vector<TrainRecord> &trains) const {
    trains.clear();
    int totalTrains = 0;
    trainRiver_.get_info(totalTrains, 1);
    if (totalTrains <= 0)
        return true;

    int *offsets = new int[totalTrains];
    int count = trainStationIndex_->findAll(station.c_str(), offsets, totalTrains);
    for (int i = 0; i < count; ++i) {
        BinaryTrainRecord bin;
        trainRiver_.read(bin, offsets[i]);
        if (bin.deleted)
            continue;
        TrainRecord train;
        decodeTrain(bin, train);
        trains.push_back(train);
    }
    delete[] offsets;
    return true;
}

// from: 出发站名，to: 到达站名，trains: 输出列车向量
// 通过站对索引快速加载同时经过from和to且from在to之前的列车，成功返回true
bool StorageManager::loadTrainsByStationPair(const std::string &from, const std::string &to, sjtu::vector<TrainRecord> &trains) const {
    trains.clear();
    int totalTrains = 0;
    trainRiver_.get_info(totalTrains, 1);
    if (totalTrains <= 0)
        return true;

    std::string pairKey = formatStationPairKey(from, to);
    int *offsets = new int[totalTrains];
    int count = trainStationPairIndex_->findAll(pairKey.c_str(), offsets, totalTrains);
    for (int i = 0; i < count; ++i) {
        BinaryTrainRecord bin;
        trainRiver_.read(bin, offsets[i]);
        if (bin.deleted)
            continue;
        TrainRecord train;
        decodeTrain(bin, train);
        trains.push_back(train);
    }
    delete[] offsets;
    return true;
}

// train_id: 列车ID，date: 日期，orders: 输出订单向量，ids: 输出订单ID向量
// 加载指定列车指定日期的所有订单，成功返回true
bool StorageManager::loadOrdersByTrainDate(const std::string &train_id, const Date &date, sjtu::vector<OrderRecord> &orders, sjtu::vector<int> &ids) const {
    orders.clear();
    ids.clear();
    int totalOrders = 0;
    orderRiver_.get_info(totalOrders, 1);
    if (totalOrders <= 0)
        return true;

    std::string key = formatTrainDateKey(train_id, date);
    int *offsets = new int[totalOrders];
    int count = orderTrainDateIndex_->findAll(key.c_str(), offsets, totalOrders);
    for (int i = 0; i < count; ++i) {
        BinaryOrderRecord bin;
        orderRiver_.read(bin, offsets[i]);
        if (bin.deleted)
            continue;
        OrderRecord order;
        decodeOrder(bin, order);
        orders.push_back(order);
        ids.push_back(bin.order_id);
    }
    delete[] offsets;
    return true;
}

// order_id: 要更新的订单ID，order: 更新后的订单记录
// 更新订单信息，订单不存在返回false，成功返回true
bool StorageManager::updateOrder(int order_id, const OrderRecord &order) {
    std::string key = formatOrderKey(order_id);
    int position;
    if (!orderIndex_->find(key.c_str(), position))
        return false;
    BinaryOrderRecord bin;
    encodeOrder(order, bin);
    bin.order_id = order_id;
    orderRiver_.update(bin, position);
    return true;
}

// 无参数
// 检查是否存在任何用户，存在返回true
bool StorageManager::hasAnyUser() const {
    int count = 0;
    userRiver_.get_info(count, 1);
    return count > 0;
}

} // namespace ticket