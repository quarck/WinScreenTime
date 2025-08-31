#include "framework.h"
#include "ScreenTimeUI.h"

#include <windows.h>
#include <string>

#include <windows.h>
#include <string>
#include <format>

#include "../TimeControlWinApp/UI.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    switch (msg) 
    {
    case UI::WMU_NOTIFY: 
    {
        int minutesLeft = (int)wParam; // notification type

        if (minutesLeft > 0)
        {
            UI::showToastNotification("Scren Time", std::format("Time ends soon!\n{0} minute{1} left", minutesLeft, minutesLeft > 1 ? "s" : ""));
        }
        else
        {
            UI::showToastNotification("Screen Time", "Time is over!\nLogging out...");
        }

        break;
    }
    //case WM_DESTROY:
    //    PostQuitMessage(0);
    //    break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) 
{
    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = UI::UI_CLASS_NAME.c_str();
    RegisterClass(&wc);

    // Message-only window (no visible UI)
    HWND hwnd = CreateWindowEx(
        0, UI::UI_CLASS_NAME.c_str(), UI::UI_WINDOW_NAME.c_str(),
        0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );

#if _DEBUG
    UI::showToastNotification("DEBUG DEBUG:", "Screen Time UI started.");
#endif

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
