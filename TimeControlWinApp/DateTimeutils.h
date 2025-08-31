#pragma once

#include <fileapi.h>
#include <cstdint>
#include <time.h>
#include <format>

#include <chrono>

namespace DateTimeUtils
{
    enum class WeekDay
    {
        //
        // Starting from 1 for Monday, as it is more convenient for end-user for configuration files update
        //
        MON = 1, 
        TUE = 2, 
        WED = 3,
        THR = 4, 
        FRI = 5, 
        SAT = 6, 
        SUN = 7,
    };


    static constexpr int DAY_START_OFFSET_HOURS = 5; // we count new day at around 5am
	static constexpr int64_t SECONDS_IN_A_DAY = 24 * 3600;

    inline int64_t GetDayNumber()
    {        
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &t);

        int64_t delta = local_tm.tm_hour < DAY_START_OFFSET_HOURS ? -SECONDS_IN_A_DAY : 0;

        local_tm.tm_hour = DAY_START_OFFSET_HOURS;
        local_tm.tm_min = 0;
        local_tm.tm_sec = 0;

        int64_t start_of_day = static_cast<int64_t>(std::mktime(&local_tm));

        return (start_of_day + delta) / SECONDS_IN_A_DAY;
    }

    inline WeekDay GetDayOfWeek(int64_t currentDayNumber = -1)
    {
        if (currentDayNumber == -1)
        {
            currentDayNumber = GetDayNumber();
        }

        // 1970-01-05 was Monday
		return static_cast<WeekDay>((currentDayNumber + 3) % 7 + 1);
    }


	//inline static int64_t GetCurrentDayOfEpoch()
	//{
	//	return (int64_t)time(nullptr) / 86400; // day number
	//}
}


namespace std
{
    template <>
    struct formatter<DateTimeUtils::WeekDay> : formatter<std::string_view> 
    {
        auto format(const DateTimeUtils::WeekDay& wd, std::format_context& ctx) const
        {
            using namespace DateTimeUtils;

            std::string_view name;
            switch (wd) 
            {
            case WeekDay::MON: name = "MON"; break;
            case WeekDay::TUE: name = "TUE"; break;
            case WeekDay::WED: name = "WED"; break;
            case WeekDay::THR: name = "THR"; break;
            case WeekDay::FRI: name = "FRI"; break;
            case WeekDay::SAT: name = "SAT"; break;
            case WeekDay::SUN: name = "SUN"; break;
            default:  name = "UNKNOWN"; break;
            }
            return formatter<std::string_view>::format(name, ctx);
        }
    };
}