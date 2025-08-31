#pragma once

#include <string>
#include <optional>

namespace Network
{
    // Download remote config file
    std::optional<std::string> DownloadFile(const std::string& url);

    std::optional<std::string> ReadLocalFile(const std::string& filePath);

    bool IsLocalFilePath(const std::string& path);

    std::optional<std::string> GetFileContentByLocation(const std::string& location);
}