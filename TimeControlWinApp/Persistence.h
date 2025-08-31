#pragma once

#include <string>
#include <iostream>
#include <fstream>

#include <fileapi.h>

#include "DateTimeutils.h"

class Persistence
{
	static constexpr int SAVE_INTERVAL_SECONDS = 300; // Save state every 5 minutes

    static constexpr int64_t MAGIC1 = 45251;
    static constexpr int64_t MAGIC2 = 526266722;

    inline static int _activeMinutes = 0;
    inline static int64_t _currentDayOfEpoch = -1;

    inline static bool _initialized = false;

    inline static std::string _dataFileFormat = "C:\\ProgramData\\SystemSchedule\\data\\{0:04x}\\{1:08x}.txt";

public: 
    static int getTodayActiveMinutes();

    static void setTodayActiveMinutes(int minutes);

    static void addMinutes(int minues);

private:
    static int getActiveMinutes(int64_t dayNumber);

    static void setActiveMinutes(int64_t dayNumber, int minutes);

    static std::string getDataFilePath();

    static void saveState();

    static void loadState();

    static void ensureFolderExists(std::string& path);

    friend class Log;
};
