#pragma once

#include <string>
#include <thread>
#include <cstring>

#include <expected>

#include <unordered_map>

#include <thread>
#include <mutex>
#include <atomic>

#include "DateTimeutils.h"

class UserLimits
{
public:
    static constexpr uint32_t DEFAULT_ALLOWED_MINUTES = 60;
	static constexpr uint32_t MAX_ALLOWED_MINUTES = 1440;

    using AllowedUsersMap = std::unordered_map<DateTimeUtils::WeekDay, uint32_t>;

private:
    int userId = 0;

    AllowedUsersMap allowedMinutesPerWeekDay;

    std::unordered_map<int64_t, uint32_t> allowedExtraMinutesPerDayNumber;
public:

    UserLimits();
    UserLimits(int id, AllowedUsersMap&& map);

    int getUserId() const;

    uint32_t getAllowedMinutes() const;
    void setExtraAlloweddMinutes(int64_t dayNumber, uint32_t minutes);
    uint32_t getAlowedExtraTime();
    uint32_t getAllowedTotalMinutes();
};

class SettingsRegistry
{
private:
    std::unordered_map<int, UserLimits> users;

    UserLimits defaultLimits = UserLimits{};

public:   
    const std::tuple<UserLimits, bool> getOrDefault(int userId) const;

    static SettingsRegistry fromString(const std::string& mainSettings, const std::string& extraTimeSettings);

private:
    static std::tuple<int, int64_t, uint32_t> parseExtraTimeEntry(const std::string& entry);
    static std::tuple<int, int, uint32_t> parseUserEntry(const std::string& entry);
    void parseUserEntries(const std::string& data);
    void parseExtraTimeEntries(const std::string& data);
};

class SettingsMonitor
{
    static constexpr int RELOAD_INTERVAL_SECONDS_FOR_LOCAL = 60;

    std::string mainSettingsFilePath;
	std::string extraTimeSettingsPath;

    std::atomic_int64_t lastReloadtime = 0;

    std::mutex settingsMutex;
    SettingsRegistry settings;

    std::atomic_bool userLoggedin = false;

public:

    SettingsMonitor(const std::string& mainSettingsFile, const std::string& extraTimeSettingsFile);

    bool hasValidSettings() const;

    const std::tuple<UserLimits, bool> get(int userId);

    void setUserLoggedIn(bool loggedIn);

    bool reload();   

private:

    static std::expected<SettingsRegistry, std::string> load(const std::string& mainSettingsFile, const std::string& extraTimeSettingsLocalFile);

    bool monitorDirectory(const std::string& directory);
    void monitorThread();
};