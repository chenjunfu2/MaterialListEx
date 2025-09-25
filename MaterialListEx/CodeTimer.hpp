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

	template<typename T = std::chrono::nanoseconds>
	T Diff(void)
	{
		return std::chrono::duration_cast<T>(tpStop - tpStart);
	}

	//打印时差
	void PrintElapsed(const char *const cpBegInfo = "", const char *const cpEndInfo = "\n")
	{
		printf("%s%.6lfms%s", cpBegInfo, (long double)Diff<std::chrono::nanoseconds>().count() * 1E-6, cpEndInfo);
	}

	template<typename T = std::chrono::milliseconds>
	static uint64_t GetSteadyTime(void)
	{
		return std::chrono::duration_cast<T>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	template<typename T = std::chrono::milliseconds>
	static uint64_t GetSystemTime(void)
	{
		return std::chrono::duration_cast<T>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
};