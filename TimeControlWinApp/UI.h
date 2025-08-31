#pragma once

#include "framework.h"
#include <shellapi.h> // Add this include to define NOTIFYICONDATA

#include <thread>

#include <windows.h>
#include <wininet.h>

#include <string>

namespace UI
{
    static constexpr uint32_t WMU_NOTIFY = WM_USER + 1;

    inline static std::string UI_CLASS_NAME = "ScreenTimeUI";
	inline static std::string UI_WINDOW_NAME = "ScreenTimeUI";

	inline static std::string TOAST_CLASS_NAME = "ScreenTimeToastClass";
	inline static std::string TOAST_WINDOW_NAME = "ScreenTimeToastWindow";

    // Show Windows notification (balloon tip)
    void showToastNotification(const std::string& title, const std::string& message)
    {
        // Register a window class for the message-only window
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = TOAST_CLASS_NAME.c_str();
        RegisterClassA(&wc);

        HWND hwnd = CreateWindowExA(0, TOAST_CLASS_NAME.c_str(), TOAST_WINDOW_NAME.c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
        if (!hwnd) return;

        NOTIFYICONDATAA nid = { 0 };
        nid.cbSize = sizeof(NOTIFYICONDATAA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_TIP;
        nid.hIcon = LoadIconA(NULL, IDI_INFORMATION);
        strncpy_s(nid.szTip, title.c_str(), sizeof(nid.szTip) - 1);

        Shell_NotifyIconA(NIM_ADD, &nid);

        // Now show the balloon
        nid.uFlags = NIF_INFO;
        strncpy_s(nid.szInfo, message.c_str(), sizeof(nid.szInfo) - 1);
        strncpy_s(nid.szInfoTitle, title.c_str(), sizeof(nid.szInfoTitle) - 1);
        nid.dwInfoFlags = NIIF_INFO;
        nid.uTimeout = 15000;

        Shell_NotifyIconA(NIM_MODIFY, &nid);

        std::this_thread::sleep_for(std::chrono::milliseconds(nid.uTimeout + 1000));

        Shell_NotifyIconA(NIM_DELETE, &nid);
        DestroyWindow(hwnd);
    }


    void proxyNotification(int minutesLeft)
    {
        HWND hwnd = FindWindowA(UI_CLASS_NAME.c_str(), UI_WINDOW_NAME.c_str());
        if (hwnd) 
        {
            PostMessageA(hwnd, WMU_NOTIFY, (WPARAM)minutesLeft, 0);
        }
    }
}