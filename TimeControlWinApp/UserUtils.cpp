
#include "framework.h"

#include <shellapi.h> // Add this include to define NOTIFYICONDATA

#include <windows.h>
#include <wininet.h>

#include "UserUtils.h"

// Check for keyboard/mouse activity
bool UserUtils::isUserActive(int timeWindowSeconds)
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii))
    {
        DWORD idleTime = GetTickCount() - lii.dwTime;
        return idleTime < timeWindowSeconds * 1000; // Active if input in last 60 seconds
    }
    return false;
}

// Force logout
void UserUtils::forceLock()
{
    LockWorkStation();
}

void UserUtils::forceLogout()
{
    ExitWindowsEx(EWX_LOGOFF | EWX_FORCE, 0);
}
