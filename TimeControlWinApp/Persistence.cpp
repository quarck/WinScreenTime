
#include "framework.h"

#include <string>
#include <iostream>
#include <fstream>

#include <fileapi.h>

#include "DateTimeutils.h"

#include "Persistence.h"


int Persistence::getTodayActiveMinutes()
{
    if (!_initialized)
    {
        loadState();
        _initialized = true;
    }
    return getActiveMinutes(DateTimeUtils::GetDayNumber());
}

void Persistence::setTodayActiveMinutes(int minutes)
{
    _initialized = true;
    setActiveMinutes(DateTimeUtils::GetDayNumber(), minutes);
    saveState();
}

void Persistence::addMinutes(int minues)
{
    int current = getTodayActiveMinutes();
    setTodayActiveMinutes(current + minues);
}

int Persistence::getActiveMinutes(int64_t dayNumber)
{
    if (_currentDayOfEpoch != dayNumber)
    {
        _activeMinutes = 0;
        _currentDayOfEpoch = dayNumber;

        saveState();
    }

    return _activeMinutes;
}

void Persistence::setActiveMinutes(int64_t dayNumber, int minutes)
{
    _currentDayOfEpoch = dayNumber;
    _activeMinutes = minutes;
}

std::string Persistence::getDataFilePath()
{
    auto dayNumber = DateTimeUtils::GetDayNumber();
    auto folderIndex = dayNumber ^ MAGIC1;
    auto dayIndex = dayNumber ^ MAGIC2;
    auto userDataFolder = std::vformat(_dataFileFormat, std::make_format_args(folderIndex, dayIndex));
    return userDataFolder;
}

void Persistence::saveState()
{
    auto dataFile = getDataFilePath();

    ensureFolderExists(dataFile);

    std::ofstream ofs(dataFile);
    ofs << _currentDayOfEpoch << " " << _activeMinutes << std::endl;
}

void Persistence::loadState()
{
    auto dataFile = getDataFilePath();

    std::ifstream ifs(dataFile);
    if (!ifs.good())
    {
        setTodayActiveMinutes(0);
        return;
    }
    ifs >> _currentDayOfEpoch >> _activeMinutes;
}

void Persistence::ensureFolderExists(std::string& path)
{
    size_t pos = 0;
    while ((pos = path.find('\\', pos)) != std::string::npos)
    {
        std::string subdir = path.substr(0, pos);
        ::CreateDirectoryA(subdir.c_str(), NULL);
        pos++;
    }
}
