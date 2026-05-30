#include "../include/DateTime.hpp"

namespace ticket {

static int monthDays(int month) {
    static const int table[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month >= 1 && month <= 12) return table[month];
    return 31;
}

static std::string pad2(int value) {
    std::string s = std::to_string(value);
    if (s.size() < 2) s = "0" + s;
    return s;
}

Date::Date() : month(0), day(0) {}
Date::Date(int month_, int day_) : month(month_), day(day_) {}

Date Date::parse(const std::string& text) {
    Date d;
    if (text.size() >= 5 && text[2] == '-') {
        d.month = std::stoi(text.substr(0,2));
        d.day = std::stoi(text.substr(3,2));
    }
    return d;
}

std::string Date::toString() const {
    return pad2(month) + "-" + pad2(day);
}

bool Date::operator<(const Date& other) const {
    if (month != other.month) return month < other.month;
    return day < other.day;
}

bool Date::operator==(const Date& other) const {
    return month == other.month && day == other.day;
}

Time::Time() : hour(0), minute(0) {}
Time::Time(int hour_, int minute_) : hour(hour_), minute(minute_) {}

Time Time::parse(const std::string& text) {
    Time t;
    if (text.size() >= 5 && text[2] == ':') {
        t.hour = std::stoi(text.substr(0,2));
        t.minute = std::stoi(text.substr(3,2));
    }
    return t;
}

std::string Time::toString() const {
    return pad2(hour) + ":" + pad2(minute);
}

bool Time::operator<(const Time& other) const {
    if (hour != other.hour) return hour < other.hour;
    return minute < other.minute;
}

bool Time::operator==(const Time& other) const {
    return hour == other.hour && minute == other.minute;
}

DateTime::DateTime() : date(), time() {}
DateTime::DateTime(const Date& date_, const Time& time_) : date(date_), time(time_) {}

std::string DateTime::toString() const {
    return date.toString() + " " + time.toString();
}

bool DateTime::operator<(const DateTime& other) const {
    if (date == other.date) return time < other.time;
    return date < other.date;
}

bool DateTime::operator==(const DateTime& other) const {
    return date == other.date && time == other.time;
}

bool DateTimeUtils::inRange(const Date& begin, const Date& end, const Date& target) {
    return !(target < begin) && !(end < target);
}

static int dayOfYear(const Date& d) {
    int days = 0;
    for (int m = 1; m < d.month; ++m) days += monthDays(m);
    days += d.day;
    return days;
}

int DateTimeUtils::dayOffset(const Date& from, const Date& to) {
    return dayOfYear(to) - dayOfYear(from);
}

static Date addDays(const Date& d, int offset) {
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
                day = monthDays(month);
            }
        }
    }
    return Date(month, day);
}

DateTime addMinutes(const DateTime& dt, int minutes) {
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

