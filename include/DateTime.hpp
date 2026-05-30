#ifndef TICKET_SYSTEM_DATETIME_HPP
#define TICKET_SYSTEM_DATETIME_HPP

#include <string>

namespace ticket {

struct Date {
    int month;
    int day;

    Date();
    Date(int month_, int day_);
    static Date parse(const std::string& text);
    std::string toString() const;
    bool operator<(const Date& other) const;
    bool operator==(const Date& other) const;
};

struct Time {
    int hour;
    int minute;

    Time();
    Time(int hour_, int minute_);
    static Time parse(const std::string& text);
    std::string toString() const;
    bool operator<(const Time& other) const;
    bool operator==(const Time& other) const;
};

struct DateTime {
    Date date;
    Time time;

    DateTime();
    DateTime(const Date& date_, const Time& time_);
    std::string toString() const;
    bool operator<(const DateTime& other) const;
    bool operator==(const DateTime& other) const;
};

class DateTimeUtils {
public:
    static bool inRange(const Date& begin, const Date& end, const Date& target);
    static int dayOffset(const Date& from, const Date& to);
};

} // namespace ticket

#endif // TICKET_SYSTEM_DATETIME_HPP