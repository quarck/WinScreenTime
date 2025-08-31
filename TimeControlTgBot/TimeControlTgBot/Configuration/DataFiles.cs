using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TimeControlTgBot.Configuration
{
    internal class DataFiles
    {
        private const long MAGIC1 = 45251;
        private const long MAGIC2 = 526266722;

        private static readonly string DataFileFormat =
            @"C:\ProgramData\SystemSchedule\data\{0:x4}\{1:x8}.txt";

        public static string GetDataFilePath(long dayNumber)
        {
            long folderIndex = dayNumber ^ MAGIC1;
            long dayIndex = dayNumber ^ MAGIC2;

            // Format using lowercase hex with correct padding (x4 / x8)
            string userDataFolder = string.Format(DataFileFormat, folderIndex, dayIndex);
            return userDataFolder;
        }

        public static int? GetDayUsage(long dayNumber)
        {
            string path = GetDataFilePath(dayNumber);
            if (!System.IO.File.Exists(path))
                return null;
            var text = System.IO.File.ReadAllText(path);

            var parts = text.Split(' ', StringSplitOptions.RemoveEmptyEntries);
            if (parts.Length < 2)
                return null;

            if (int.TryParse(parts[0], out int dayNrCheck) 
                && int.TryParse(parts[1], out int usage) 
                && dayNrCheck == (int)dayNumber)
            {
                return usage;
            }

            return null;
        }
    }
}
