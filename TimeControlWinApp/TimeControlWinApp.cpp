// TODO: use telegram bot as a backup. (This app is the bot server, while two users: parents, are allowed to give it commands).


#define _CRT_SECURE_NO_WARNINGS 1

#include "framework.h"
#include "TimeControlWinApp.h"

#include <windows.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")

#include <shellapi.h> // Add this include to define NOTIFYICONDATA

#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>

#include <unordered_map>

#include "UserSettings.h"
#include "UserUtils.h"
#include "Network.h"
#include "UI.h"
#include "Persistence.h"
#include "Log.h"
#include "WarningsState.h"

const int warning2Minutes = 5;
const int warning1Minutes = 15;

// URL of the remote time configuration file
const std::string configUrl =  R"(C:\ProgramData\TPService\conf\configuration.txt)";
const std::string extraTimeConfig = R"(C:\ProgramData\TPService\conf\extra.txt)";

const int thisUserId = 1;

// accesed by the window handler proc callback - has to be global likely
std::atomic_bool isWindowsSessionLocked = false;

void WorkerThread()
{
    auto settings = SettingsMonitor{ configUrl, extraTimeConfig };

    settings.setUserLoggedIn(!isWindowsSessionLocked);

    WarningsState warningState;

	Log::Info("Worker thread started.");

	for (int64_t total_seconds_slept = 0; ;)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        total_seconds_slept += 10;

        if (total_seconds_slept % 60 == 0)
        {
            if (UserUtils::isUserActive())
            {
                Persistence::addMinutes(1);
                if (isWindowsSessionLocked)
                {
                    isWindowsSessionLocked = false;
                    Log::Warning("Session is locked, but user activity detected. Unlocking session.");
                }
            }
        }        

        settings.setUserLoggedIn(!isWindowsSessionLocked);

        if (!settings.hasValidSettings())
        {
            Log::Warning("No valid settings -- not acting to lock");
            continue;
        }

        auto [config, is_default] = settings.get(thisUserId);

        auto timeUsed = Persistence::getTodayActiveMinutes();
        int allowedMinutes = config.getAllowedTotalMinutes();

        if (timeUsed % 10 == 0)
        {
			// Don't spam the log too much
            Log::Info("User {0} used {1} minutes out of allowed {2} minutes (default settings: {3})", thisUserId, timeUsed, allowedMinutes, is_default ? "true" : "false");
        }

        if (timeUsed >= allowedMinutes && !warningState.triedReloadingBeforeLocking(allowedMinutes))
        {
			Log::Info("User {0} exceeded allowed time ({1} minutes). Force reloading settings before applying action.", thisUserId, allowedMinutes);

            settings.reload();
            warningState.setTriedReloadingBeforeLocking(allowedMinutes);

            std::tie(config, is_default) = settings.get(thisUserId);
            int allowedMinutes = config.getAllowedTotalMinutes();
        }
        
        if (timeUsed >= allowedMinutes && warningState.allWarningsGiven(allowedMinutes))
        {
            if (warningState.lockGiven(allowedMinutes))
            {
                Log::Info("User {0} was already locked -- re-locking without timeout (allowedMinutes={1}).", thisUserId, allowedMinutes);
                UI::proxyNotification(0);
            }
            else
            {
                Log::Info("User {0} exceeded allowed time ({1} minutes). Posting notification.", thisUserId, allowedMinutes);
                UI::proxyNotification(0);

                std::this_thread::sleep_for(std::chrono::seconds(15));

                Log::Info("User {0} exceeded allowed time ({1} minutes). Locking user out", thisUserId, allowedMinutes);
                warningState.setLockGiven(allowedMinutes);
            }

            UserUtils::forceLock();
        }
        else if (timeUsed >= allowedMinutes - warning1Minutes && !warningState.isFirstWarningGiven(allowedMinutes))
        {
			Log::Info("User {0} approaching allowed time ({1} minutes). Posting 1st warning", thisUserId, allowedMinutes);
            UI::proxyNotification(allowedMinutes - timeUsed);            
            warningState.setFirstWarningGiven(allowedMinutes);
        }
        else if (timeUsed >= allowedMinutes - warning2Minutes && !warningState.isSecondWarningGiven(allowedMinutes))
        {
            Log::Info("User {0} approaching allowed time ({1} minutes). Posting 2nd warning.", thisUserId, allowedMinutes);
            UI::proxyNotification(allowedMinutes - timeUsed);
            warningState.setSecondWarningGiven(allowedMinutes);
        }        
    }
}

void HandleSessionChange(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case WTS_SESSION_UNLOCK:
    case WTS_SESSION_LOGON:
        Log::Debug("User {0}", wParam == WTS_SESSION_UNLOCK ? "unlocked session" : "logged on");
        isWindowsSessionLocked = false;
        break;

    case WTS_SESSION_LOCK:
    case WTS_SESSION_LOGOFF:
        Log::Debug("User {0}", wParam == WTS_SESSION_LOCK ? "locked session" : "logged off");
        isWindowsSessionLocked = true;
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_WTSSESSION_CHANGE)
    {
        HandleSessionChange(wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    Log::Init();

    std::thread worker{ WorkerThread };
    worker.detach();


    // Register a hidden window to receive session notifications
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "lpservicecls";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowA(wc.lpszClassName, "lpservicehwd", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    // Register for session change notifications
    WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WTSUnRegisterSessionNotification(hwnd);

    return 0;
}
