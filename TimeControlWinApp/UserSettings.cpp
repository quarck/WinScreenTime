
#include "framework.h"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>

#include <expected>

#include <unordered_map>

#include <thread>
#include <mutex>
#include <atomic>

#include "UserSettings.h"

#include <Windows.h>

#include "Network.h"
#include "DateTimeutils.h"

#include "Log.h"


UserLimits::UserLimits() : userId{ 0 }, allowedMinutesPerWeekDay{} {};

UserLimits::UserLimits(int id, AllowedUsersMap&& map)
    : userId{ id }
    , allowedMinutesPerWeekDay{ std::move(map) }
{
    std::ostringstream s;
    for (auto& [day, minutes] : allowedMinutesPerWeekDay)
    {
        s << std::format("{0}: {1}; ", day, minutes);
    }

    Log::Info("UserLimits created for user {0}; limits: {1}", userId, s.str());
}

int UserLimits::getUserId() const { return userId; }

uint32_t UserLimits::getAllowedMinutes() const
{
    auto weekDay = DateTimeUtils::GetDayOfWeek();
    auto it = allowedMinutesPerWeekDay.find(weekDay);
    return it != allowedMinutesPerWeekDay.end() ? it->second : DEFAULT_ALLOWED_MINUTES;
}

void UserLimits::setExtraAlloweddMinutes(int64_t dayNumber, uint32_t minutes)
{
    allowedExtraMinutesPerDayNumber[dayNumber] = minutes;

    Log::Info("UserLimits extra time for user {0}, day {1}: extra time {2} minutes", userId, dayNumber, minutes);
}

uint32_t UserLimits::getAlowedExtraTime()
{
    auto currentDay = DateTimeUtils::GetDayNumber();

    auto it = allowedExtraMinutesPerDayNumber.find(currentDay);
    return it != allowedExtraMinutesPerDayNumber.end() ? it->second : 0;
}

uint32_t UserLimits::getAllowedTotalMinutes()
{
    return getAllowedMinutes() + getAlowedExtraTime();
}


const std::tuple<UserLimits, bool> SettingsRegistry::getOrDefault(int userId) const
{
    auto it = users.find(userId);
    if (it == users.end())
    {
        return { defaultLimits, true };
    }
    return { it->second, false };
}

SettingsRegistry SettingsRegistry::fromString(const std::string& mainSettings, const std::string& extraTimeSettings)
{
    SettingsRegistry allSettings;
    allSettings.parseUserEntries(mainSettings);
    allSettings.parseExtraTimeEntries(extraTimeSettings);
    return allSettings;
};

std::tuple<int, int64_t, uint32_t> SettingsRegistry::parseExtraTimeEntry(const std::string& entry)
{
    try
    {
        std::istringstream iss(entry);
        int userId;
        int64_t dayNumber;
        uint32_t extraMinutes1;
        uint32_t extraMinutes2;
        if (iss >> userId >> dayNumber >> extraMinutes1 >> extraMinutes2)
        {
            auto extraMinutes = extraMinutes1 ^ ~extraMinutes2;
            if (extraMinutes > UserLimits::MAX_ALLOWED_MINUTES)
            {
                Log::Warning("Configuration tampering detected for user {0} on day {1}: extra minutes {2}:{3}",
                    userId, dayNumber, extraMinutes1, extraMinutes2);
                return { -1, -1, -1 };
            }

            return { userId, dayNumber, extraMinutes1 ^ ~extraMinutes2 };
        }
    }
    catch (...)
    {
        Log::Error("Failed to parse entry {}", entry);
    }
    return { -1, -1, -1 };
}

std::tuple<int, int, uint32_t> SettingsRegistry::parseUserEntry(const std::string& entry)
{
    try
    {
        std::istringstream iss(entry);
        int userId;
        int dayOfWeek;
        uint32_t allowedMinutes1;
        uint32_t allowedMinutes2;

        if (iss >> userId >> dayOfWeek >> allowedMinutes1 >> allowedMinutes2)
        {
            auto allowedMinutes = allowedMinutes1 ^ ~allowedMinutes2;
            if (allowedMinutes > UserLimits::MAX_ALLOWED_MINUTES)
            {
                Log::Warning("Configuration tampering detected for user {0} on {1}: allowed minutes {2}:{3}",
                    userId, static_cast<DateTimeUtils::WeekDay>(dayOfWeek), allowedMinutes1, allowedMinutes2);
                return { -1, -1, -1 };
            }

            return { userId, dayOfWeek, allowedMinutes1 ^ ~allowedMinutes2 };
        }
    }
    catch (...)
    {
        Log::Error("Failed to parse entry {}", entry);
    }
    return { -1, -1, -1 };
}

void SettingsRegistry::parseUserEntries(const std::string& data)
{
    std::istringstream iss(data);
    std::string line;

    std::unordered_map<int, UserLimits::AllowedUsersMap> tempUsers;

    for (int lineNumber = 1; std::getline(iss, line); lineNumber++)
    {
        auto [userId, dayOfWeek, allowedMinutes] = parseUserEntry(line);
        if (userId == -1)
        {
            Log::Warning("Failed to parse user entry at line {0}: {1}", lineNumber, line);
            continue;
        }

        tempUsers[userId][static_cast<DateTimeUtils::WeekDay>(dayOfWeek)] = allowedMinutes;
    }

    for (auto&& [id, limits] : tempUsers)
    {
        users[id] = UserLimits(id, std::move(limits));
    }
}

void SettingsRegistry::parseExtraTimeEntries(const std::string& data)
{
    std::istringstream iss(data);
    std::string line;
    while (std::getline(iss, line))
    {
        auto [userId, dayNumber, extraMinutes] = parseExtraTimeEntry(line);
        if (userId != -1)
        {
            auto it = users.find(userId);
            if (it != users.end())
            {
                it->second.setExtraAlloweddMinutes(dayNumber, extraMinutes);
            }
        }
    }
}


SettingsMonitor::SettingsMonitor(const std::string& mainSettingsFile, const std::string& extraTimeSettingsFile)
    : mainSettingsFilePath{ mainSettingsFile }
    , extraTimeSettingsPath{ extraTimeSettingsFile }
{
    std::thread thr([this]() { monitorThread(); });
    thr.detach();
}

bool SettingsMonitor::hasValidSettings() const
{
    return lastReloadtime != 0;
}

const std::tuple<UserLimits, bool> SettingsMonitor::get(int userId)
{
    std::lock_guard<std::mutex> lock{ settingsMutex };
    return settings.getOrDefault(userId);
}

void SettingsMonitor::setUserLoggedIn(bool loggedIn)
{
    userLoggedin = loggedIn;
}

bool SettingsMonitor::reload()
{
    auto loadResult = load(mainSettingsFilePath, extraTimeSettingsPath);
    if (!loadResult.has_value())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock{ settingsMutex };
    settings = loadResult.value();
    lastReloadtime = time(nullptr);

    return true;
}


std::expected<SettingsRegistry, std::string> SettingsMonitor::load(const std::string& mainSettingsFile, const std::string& extraTimeSettingsLocalFile)
{
    std::optional<std::string> mainSettingsContent = Network::ReadLocalFile(mainSettingsFile);
    if (!mainSettingsContent.has_value())
    {
        return std::unexpected("Failed to download main config");
    }

    std::string extraTimeSettingsLocalContent = "";
    if (!extraTimeSettingsLocalFile.empty())
    {
        auto opt = Network::ReadLocalFile(extraTimeSettingsLocalFile);
        if (!opt.has_value())
        {
            return std::unexpected("Failed to read extra time config");
        }
        extraTimeSettingsLocalContent = opt.value();
    }

    return SettingsRegistry::fromString(mainSettingsContent.value(), extraTimeSettingsLocalContent);
}

bool SettingsMonitor::monitorDirectory(const std::string& directory)
{
    // Open directory handle
    HANDLE hDir = CreateFile(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        Log::Error("Failed to open directory {0} for monitoring", directory);
        return false;
    }

    char buffer[1024];
    DWORD bytesReturned;

    while (true)
    {
        if (ReadDirectoryChangesW(
            hDir,
            &buffer,
            sizeof(buffer),
            FALSE, // FALSE = monitor only this dir, TRUE = recursive
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned,
            NULL,
            NULL))
        {
            bool needsReload = false;

            FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            do
            {
                int sizeNeeded = WideCharToMultiByte(
                    CP_UTF8,            // target code page
                    0,                  // flags
                    fni->FileName,        // input UTF-16 string
                    fni->FileNameLength / sizeof(WCHAR),   // input length
                    NULL, 0,            // query buffer size
                    NULL, NULL);

                std::vector<char> fileNameV(sizeNeeded, 0);

                WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    fni->FileName,
                    fni->FileNameLength / sizeof(WCHAR),
                    fileNameV.data(),
                    sizeNeeded,
                    NULL, NULL
                );

                std::string filename(fileNameV.data(), sizeNeeded);

                Log::Info("Detected change in file: {0}, change: {1}; reloading config directory", filename, fni->Action);
                needsReload = true;

                // Move to next record (if multiple events are in buffer)
                if (fni->NextEntryOffset != 0)
                {
                    fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset);
                }
                else
                {
                    fni = nullptr;
                }
            } while (fni != nullptr);

            if (needsReload)
            {
                reload();
            }
        }
        else
        {
            Log::Error("ReadDirectoryChangesW failed.");
            break;
        }
    }

    CloseHandle(hDir);
    return false;
}

void SettingsMonitor::monitorThread()
{
    reload();

    size_t pos = mainSettingsFilePath.find_last_of("\\/");
    std::string directory = mainSettingsFilePath.substr(0, pos);

    if (!monitorDirectory(directory))
    {
        Log::Error("Directory monitoring failed, falling back to periodic reload.");

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(RELOAD_INTERVAL_SECONDS_FOR_LOCAL));
            reload();
        }
    }

}
