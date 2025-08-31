#pragma once

class UserUtils
{
public:
    // Check for keyboard/mouse activity
    static bool isUserActive(int timeWindowSeconds = 60);

    static void forceLock();

    static void forceLogout();
};