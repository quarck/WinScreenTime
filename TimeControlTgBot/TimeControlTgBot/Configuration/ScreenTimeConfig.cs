using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace TimeControlTgBot.Configuration
{
    public class ScreenTimeConfig
    {
        private readonly string _filePath;// = @"C:\ProgramData\TPService\conf\configuration.txt";

        private readonly Dictionary<int, Dictionary<int, uint>> _config = [];

        private readonly Random _rng = new Random();

        public ScreenTimeConfig(string filePath)
        {
            _filePath = filePath;
            Load();
        }


        /// <summary>
        /// Loads configuration from file
        /// </summary>
        private void Load()
        {
            _config.Clear();

            if (!File.Exists(_filePath))
                return;

            foreach (var line in File.ReadAllLines(_filePath))
            {
                if (string.IsNullOrWhiteSpace(line)) 
                    continue;

                var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length != 4) continue;

                if (!int.TryParse(parts[0], out int userId)) continue;
                if (!int.TryParse(parts[1], out int dayOfWeek)) continue;
                if (!uint.TryParse(parts[2], out uint allowedTime1)) continue;
                if (!uint.TryParse(parts[3], out uint allowedTime2)) continue;


                uint allowedTime = allowedTime1 ^ ~allowedTime2;

                SetAllowedTime(userId, dayOfWeek, allowedTime);
            }
        }

        private void SetAllowedTime(int userId, int dayOfWeek, uint allowedTime)
        {
            if (_config.TryGetValue(userId, out var userDict))
            {
                userDict[dayOfWeek] = allowedTime;
            }
            else
            {
                _config[userId] = new Dictionary<int, uint> { { dayOfWeek, allowedTime } };
            }
        }

        /// <summary>
        /// Saves the current config back to the file
        /// </summary>
        private void Save()
        {
            var lines = new List<string>();


            foreach (var (userId, daysConfig) in _config)
            {
                foreach (var (dayOfWeek, allowedTime) in daysConfig)
                {
                    uint allowedTime2 = (uint)_rng.Next(int.MaxValue / 4, int.MaxValue);
                    uint allowedTime1 = allowedTime ^ ~allowedTime2;

                    lines.Add($"{userId} {dayOfWeek} {allowedTime1} {allowedTime2}");
                }
            }

            File.WriteAllLines(_filePath, lines);
        }


        /// <summary>
        /// Get allowed time for a specific user and day
        /// </summary>
        public uint? GetAllowedTime(int userId, int dayOfWeek)
        {
            if (_config.TryGetValue(userId, out var userDict))
            {
                if (userDict.TryGetValue(dayOfWeek, out uint time))
                    return time;
            }
            return null;
        }

        public Dictionary<int, uint> GetAllowedTime(int userId)
        {
            if (_config.TryGetValue(userId, out var userDict))
            {
                return userDict;
            }
            return new Dictionary<int, uint>();
        }

        /// <summary>
        /// Update allowed time (minutes) for a specific user and day
        /// Auto-saves to file after updating
        /// </summary>
        public void UpdateAllowedTime(int userId, int dayOfWeek, uint allowedMinutes)
        {
            if (dayOfWeek < 1 || dayOfWeek > 7)
                throw new ArgumentOutOfRangeException(nameof(dayOfWeek), "Day of week must be between 1 and 7");

            SetAllowedTime(userId, dayOfWeek, allowedMinutes);
            Save();
        }
    }

    public class ExtraTimeConfig
    {
        private readonly string _filePath;// = @"C:\ProgramData\TPService\conf\extra.txt";

        private readonly Dictionary<int, Dictionary<int, uint>> _config = [];

        private readonly Random _rng = new Random();

        public ExtraTimeConfig(string filePath)
        {
            _filePath = filePath;
            Load();
        }

        /// <summary>
        /// Loads extra time configuration from file
        /// </summary>
        private void Load()
        {
            _config.Clear();

            if (!File.Exists(_filePath))
                return;

            foreach (var line in File.ReadAllLines(_filePath))
            {
                if (string.IsNullOrWhiteSpace(line)) continue;

                var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);

                if (parts.Length != 4) continue;

                if (!int.TryParse(parts[0], out int userId)) continue;
                if (!int.TryParse(parts[1], out int dayId)) continue;
                if (!uint.TryParse(parts[2], out uint extraTime1)) continue;
                if (!uint.TryParse(parts[3], out uint extraTime2)) continue;

                uint extraTime = extraTime1 ^ ~extraTime2;

                SetExtraTime(userId, dayId, extraTime);
            }
        }

        void SetExtraTime(int userId, int dayId, uint extraTime)
        {
            if (_config.TryGetValue(userId, out var perDay))
            {
                perDay[dayId] = extraTime;
            }
            else
            {
                _config[userId] = new Dictionary<int, uint> { { dayId, extraTime } };
            }
        }

        /// <summary>
        /// Saves the current extra time config back to the file
        /// </summary>
        private void Save()
        {
            var lines = new List<string>();

            foreach (var (userId, daysConfig) in _config)
            {
                foreach (var (dayId, extraTime) in daysConfig)
                {
                    uint extraTime2 = (uint)_rng.Next(int.MaxValue / 4, int.MaxValue);
                    uint extraTime1 = extraTime ^ ~extraTime2;
                    lines.Add($"{userId} {dayId} {extraTime1} {extraTime2}");
                }
            }
            
            File.WriteAllLines(_filePath, lines);
        }

        /// <summary>
        /// Get extra time for a specific user and day
        /// </summary>
        public int GetExtraTime(int userId)
        {
            int dayId = (int)DateTimeHelpers.GetDayNumber();

            if (_config.TryGetValue(userId, out var userDict))
            {
                if (userDict.TryGetValue(dayId, out uint time))
                    return (int)time;
            }

            return 0;
        }

        /// <summary>
        /// Update extra time (minutes) for a specific user and day
        /// Auto-saves to file after updating
        /// </summary>
        public void UpdateExtraTime(int userId, uint extraMinutes)
        {
            SetExtraTime(userId, (int)DateTimeHelpers.GetDayNumber(), extraMinutes);
            Save();
        }
    }

}
