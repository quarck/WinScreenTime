using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TimeControlTgBot
{
    internal class DateTimeHelpers
    {
        private const int DAY_START_OFFSET_HOURS = 5; // day starts at 5am
        private const long SECONDS_IN_A_DAY = 24 * 3600;

        public static long GetDayNumber(DateTime? dtNow = null)
        {
            DateTime now = dtNow ?? DateTime.Now;

            // if current time is before 5am, shift back one day
            long delta = now.Hour < DAY_START_OFFSET_HOURS ? -SECONDS_IN_A_DAY : 0;

            // normalize to today's start offset (5am)
            DateTime startOfDay = new DateTime(
                now.Year,
                now.Month,
                now.Day,
                DAY_START_OFFSET_HOURS,
                0,
                0,
                DateTimeKind.Local
            );

            // Unix timestamp for 5am local time
            long startOfDayUnix = new DateTimeOffset(startOfDay).ToUnixTimeSeconds();

            return (startOfDayUnix + delta) / SECONDS_IN_A_DAY;
        }

        public static string DayNumberToDateStr(long day)
        {
            long seconds = day * SECONDS_IN_A_DAY;
            DateTimeOffset dto = DateTimeOffset.FromUnixTimeSeconds(seconds).ToLocalTime();
            return dto.ToString("yyyy-MM-dd");
        }
    }
}
