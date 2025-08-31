#pragma once

#include <cstdint>

class WarningsState
{
	bool firstWarningGiven = false;
	bool secondWarningGiven = false;
	bool triedReloadingBeforeLock = false;
	bool lockWasGiven = false;

	int64_t warningGivenForDay = -1;
	int warningGivenForTotalMinutes = 0;

public:
	bool allWarningsGiven(int forMinutes);

	bool isFirstWarningGiven(int forMinutes);

	bool isSecondWarningGiven(int forMinutes);

	bool lockGiven(int forMinutes);

	bool triedReloadingBeforeLocking(int forMinutes);

	void setFirstWarningGiven(int forMinutes);

	void setSecondWarningGiven(int forMinutes);

	void setLockGiven(int forMinutes);

	void setTriedReloadingBeforeLocking(int forMinutes);

private:
	void resetIfNeeded(int forMinutes);
};