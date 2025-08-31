#pragma once

#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>

#include "Persistence.h"

class Log
{
	inline static std::string logFileName = "C:\\ProgramData\\TPService\\log.txt";
	inline static std::mutex logMutex;

public:

	static void Init()
	{
		Persistence::ensureFolderExists(logFileName);

		std::lock_guard lock{ logMutex};

		std::ofstream logFile;
		logFile.open(logFileName, std::ios_base::app);
		if (logFile.is_open())
		{
			logFile << "\n\n----------------------------------------\n";
			logFile << GetTimestamp() << ": Log initialized.\n";
		}
	}

	template <typename... Args>
	static void Info(const char* format, Args... args)
	{
		return LogMessage("INFO", format, args...);
	}

	template <typename... Args>
	static void Error(const char* format, Args... args)
	{
		return LogMessage("ERROR", format, args...);
	}

	template <typename... Args>
	static void Debug(const char* format, Args... args)
	{
		return LogMessage("DEBUG", format, args...);
	}

	template <typename... Args>
	static void Warning(const char* format, Args... args)
	{
		return LogMessage("WARNING", format, args...);
	}

private: 

	template <typename... Args>
	static void LogMessage(const char* level, const char* format, Args... args)
	{
		std::lock_guard lock{ logMutex };

		std::ofstream logFile;
		logFile.open(logFileName, std::ios_base::app);
		if (logFile.is_open())
		{
			std::string message = std::vformat(format, std::make_format_args(args...));
			logFile << GetTimestamp() << "[" << DateTimeUtils::GetDayNumber() << "]" << " [" << level << "] " << message << "\n";
		}
	}	

	static std::string GetTimestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
		localtime_s(&tm, &in_time_t);

		std::string s = std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);

		return s;
	}
};