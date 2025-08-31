#include "framework.h"

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>


#include <windows.h>
#include <shellapi.h> // Add this include to define NOTIFYICONDATA
#include <wininet.h>

#include <unordered_map>

#include "UserSettings.h"
#include "UserUtils.h"

#pragma comment(lib, "wininet.lib")


namespace Network
{
    // Download remote config file
    std::optional<std::string> DownloadFile(const std::string& url)
    {
        HINTERNET hInternet = InternetOpenA("TimeControlWinApp", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet)
        {
            return std::nullopt;
        }

        HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile)
        {
            InternetCloseHandle(hInternet);
            return std::nullopt;
        }

        std::string result;
        char buffer[4096];
        DWORD bytesRead = 0;
        do
        {
            if (!InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
            {
                break;
            }
            result.append(buffer, bytesRead);

        } while (bytesRead > 0);

        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        if (result.empty())
        {
            return std::nullopt;
        }
        return result;
    }

    std::optional<std::string> ReadLocalFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return std::nullopt;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        return content;
    }

    bool IsLocalFilePath(const std::string& path)
    {
        // Simple check: if the path starts with a drive letter or a backslash, consider it local
        return (path.size() > 1 && ((isalpha(path[0]) && path[1] == ':') || path[0] == '\\' || path[0] == '/'));
    }

    std::optional<std::string> GetFileContentByLocation(const std::string& location)
    {
        if (IsLocalFilePath(location))
        {
            return ReadLocalFile(location);
        }
        else
        {
            return DownloadFile(location);
        }
    }
}