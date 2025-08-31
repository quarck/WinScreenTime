#include "framework.h"

#include <cstdint>

#include "WarningsState.h"

#include "DateTimeutils.h"
#include "Log.h"


bool WarningsState::allWarningsGiven(int forMinutes)
{
	resetIfNeeded(forMinutes);
	return firstWarningGiven && secondWarningGiven;
}

bool WarningsState::isFirstWarningGiven(int forMinutes)
{
	resetIfNeeded(forMinutes);
	return firstWarningGiven;
}

bool WarningsState::isSecondWarningGiven(int forMinutes)
{
	resetIfNeeded(forMinutes);
	return secondWarningGiven;
}

bool WarningsState::lockGiven(int forMinutes)
{
	resetIfNeeded(forMinutes);
	return lockWasGiven;
}

bool WarningsState::triedReloadingBeforeLocking(int forMinutes)
{
	resetIfNeeded(forMinutes);
	return triedReloadingBeforeLock;
}

void WarningsState::setFirstWarningGiven(int forMinutes)
{
	warningGivenForDay = DateTimeUtils::GetDayNumber();
	warningGivenForTotalMinutes = forMinutes;
	firstWarningGiven = true;
}

void WarningsState::setSecondWarningGiven(int forMinutes)
{
	warningGivenForDay = DateTimeUtils::GetDayNumber();
	warningGivenForTotalMinutes = forMinutes;
	secondWarningGiven = true;
}

void WarningsState::setLockGiven(int forMinutes)
{
	warningGivenForDay = DateTimeUtils::GetDayNumber();
	warningGivenForTotalMinutes = forMinutes;
	lockWasGiven = true;
}

void WarningsState::setTriedReloadingBeforeLocking(int forMinutes)
{
	warningGivenForDay = DateTimeUtils::GetDayNumber();
	warningGivenForTotalMinutes = forMinutes;
	triedReloadingBeforeLock = true;
}

void WarningsState::resetIfNeeded(int forMinutes)
{
	int64_t currentDay = DateTimeUtils::GetDayNumber();
	if (currentDay != warningGivenForDay || warningGivenForTotalMinutes != forMinutes)
	{
		Log::Info("Resetting warnings state for new day or changed allowed minutes (currentDay={0}, warningGivenForDay={1}, forMinutes={2}, warningGivenForTotalMinutes={3})",
			currentDay, warningGivenForDay, forMinutes, warningGivenForTotalMinutes);

		firstWarningGiven = false;
		secondWarningGiven = false;
		lockWasGiven = false;
		warningGivenForDay = currentDay;
		warningGivenForTotalMinutes = forMinutes;
	}
}