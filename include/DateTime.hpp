#ifndef TICKET_SYSTEM_DATETIME_HPP
#define TICKET_SYSTEM_DATETIME_HPP

#include <string>

namespace ticketsystem {

// 日期
struct Date {
    int month; // 月份
    int day;   // 日期

    Date();
    Date(int month_, int day_);
    static Date parse(const std::string &text);
    std::string toString() const;
    bool operator<(const Date &other) const;
    bool operator==(const Date &other) const;
};

// 时间
struct Time {
    int hour;
    int minute;

    Time();
    Time(int hour_, int minute_);
    static Time parse(const std::string &text);
    std::string toString() const;
    bool operator<(const Time &other) const;
    bool operator==(const Time &other) const;
};

// 组合日期和时间
struct DateTime {
    Date date; // 日期
    Time time; // 时间

    DateTime();
    DateTime(const Date &date_, const Time &time_);
    std::string toString() const;
    bool operator<(const DateTime &other) const;
    bool operator==(const DateTime &other) const;
};

class DateTimeUtils {
public:
    static int dayOffset(const Date &from, const Date &to);
};

DateTime addMinutes(const DateTime &dt, int minutes);
Date addDays(const Date &d, int offset);

} // namespace ticketsystem

#endif // TICKET_SYSTEM_DATETIME_HPP