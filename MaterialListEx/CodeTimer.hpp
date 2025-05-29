#pragma once

#include <chrono>

class CodeTimer
{
public:
	std::chrono::steady_clock::time_point tpStart{};
	std::chrono::steady_clock::time_point tpStop{};
public:
	CodeTimer(void) = default;
	~CodeTimer(void) = default;

	void Start(void)
	{
		tpStart = std::chrono::steady_clock::now();
	}

	void Stop(void)
	{
		tpStop = std::chrono::steady_clock::now();
	}

	//¥Ú”° ±≤Ó
	void PrintElapsed(const char *const cpBegInfo = "", const char *const cpEndInfo = "\n")
	{
		auto tmp = std::chrono::duration_cast<std::chrono::nanoseconds>(tpStop - tpStart);
		printf("%s%lfms%s", cpBegInfo, (long double)tmp.count() * 1E-6, cpEndInfo);
	}
};