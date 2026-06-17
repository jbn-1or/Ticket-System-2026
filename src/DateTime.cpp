#include "DateTime.hpp"

namespace ticket {

// 获取指定月份的天数，无效月份返回31
static int monthDays(int month) {
    static const int table[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month >= 1 && month <= 12)
        return table[month];
    return 31;
}

// value: 要格式化的整数
// 将整数格式化为两位数字符串，不足补零
static std::string pad2(int value) {
    std::string s = std::to_string(value);
    if (s.size() < 2)
        s = "0" + s;
    return s;
}

// 默认构造函数：初始化月份和日期为0
Date::Date() : month(0), day(0) {}
// 构造函数：用指定月份和日期创建Date对象
Date::Date(int month_, int day_) : month(month_), day(day_) {}

// text: 格式为"mm-dd"的日期字符串
// 解析日期字符串，返回Date对象
Date Date::parse(const std::string &text) {
    Date d;
    if (text.size() >= 5 && text[2] == '-') {
        d.month = std::stoi(text.substr(0, 2));
        d.day = std::stoi(text.substr(3, 2));
    }
    return d;
}

// 将Date对象转换为"mm-dd"格式的字符串
std::string Date::toString() const {
    return pad2(month) + "-" + pad2(day);
}

// 比较两个日期大小，this < other返回true
bool Date::operator<(const Date &other) const {
    if (month != other.month)
        return month < other.month;
    return day < other.day;
}

// 判断两个日期是否相等，相等返回true
bool Date::operator==(const Date &other) const {
    return month == other.month && day == other.day;
}

// 默认构造函数：初始化小时和分钟为0
Time::Time() : hour(0), minute(0) {}
// 构造函数：用指定小时和分钟创建Time对象
Time::Time(int hour_, int minute_) : hour(hour_), minute(minute_) {}

// text: 格式为"hh:mm"的时间字符串
// 解析时间字符串，返回Time对象
Time Time::parse(const std::string &text) {
    Time t;
    if (text.size() >= 5 && text[2] == ':') {
        t.hour = std::stoi(text.substr(0, 2));
        t.minute = std::stoi(text.substr(3, 2));
    }
    return t;
}

// 将Time对象转换为"hh:mm"格式的字符串
std::string Time::toString() const {
    return pad2(hour) + ":" + pad2(minute);
}

// 比较两个时间大小，this < other返回true
bool Time::operator<(const Time &other) const {
    if (hour != other.hour)
        return hour < other.hour;
    return minute < other.minute;
}

// 判断两个时间是否相等，相等返回true
bool Time::operator==(const Time &other) const {
    return hour == other.hour && minute == other.minute;
}

// 默认构造函数：初始化日期和时间为空
DateTime::DateTime() : date(), time() {}
// 构造函数：用指定日期和时间创建DateTime
DateTime::DateTime(const Date &date_, const Time &time_) : date(date_), time(time_) {}

// 将DateTime对象转换为"mm-dd hh:mm"格式的字符串
std::string DateTime::toString() const {
    return date.toString() + " " + time.toString();
}

// 比较两个日期时间大小，this < other返回true
bool DateTime::operator<(const DateTime &other) const {
    if (date == other.date)
        return time < other.time;
    return date < other.date;
}

// 判断两个日期时间是否相等，相等返回true
bool DateTime::operator==(const DateTime &other) const {
    return date == other.date && time == other.time;
}

// d: 日期对象
// 计算日期是一年中的第几天，返回天数
static int dayOfYear(const Date &d) {
    int days = 0;
    for (int m = 1; m < d.month; ++m)
        days += monthDays(m);
    days += d.day;
    return days;
}

// from: 起始日期，to: 结束日期
// 计算两个日期之间的天数差，返回to - from
int DateTimeUtils::dayOffset(const Date &from, const Date &to) {
    return dayOfYear(to) - dayOfYear(from);
}

// d: 原始日期，offset: 天数偏移
// 计算日期加上指定天数后的新日期，返回新Date对象
Date addDays(const Date &d, int offset) {
    int month = d.month;
    int day = d.day;
    if (offset >= 0) {
        while (offset > 0) {
            int remain = monthDays(month) - day;
            if (offset <= remain) {
                day += offset;
                offset = 0;
            } else {
                offset -= (remain + 1);
                day = 1;
                month += 1;
                if (month > 12)
                    month = 1;
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
                if (month < 1)
                    month = 12;
                day = monthDays(month);
            }
        }
    }
    return Date(month, day);
}

// dt: 原始日期时间，minutes: 分钟偏移
// 计算日期时间加上指定分钟后的新日期时间，返回新DateTime对象
DateTime addMinutes(const DateTime &dt, int minutes) {
    int total = dt.time.hour * 60 + dt.time.minute + minutes;
    int dayOffset = 0;
    while (total < 0) {
        total += 1440;
        dayOffset -= 1;
    }
    while (total >= 1440) {
        total -= 1440;
        dayOffset += 1;
    }
    int hour = total / 60;
    int minute = total % 60;
    Date newDate = addDays(dt.date, dayOffset);
    return DateTime(newDate, Time(hour, minute));
}

} // namespace ticket