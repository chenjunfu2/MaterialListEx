#pragma once

#include <chrono>

class CodeTimer
{
public:
	std::chrono::steady_clock::time_point tpStart{};
	std::chrono::steady_clock::time_point tpStop{};

	enum UnitConversion : int64_t
	{
		nanos_per_micro = 1000,		//1000ns=1us
		micros_per_milli = 1000,	//1000us=1ms
		millis_per_second = 1000,	//1000ms=1s
		seconds_per_minute = 60,	//60s=1min
		minutes_per_hour = 60,		//60min=1h
	};

	static inline constexpr UnitConversion arrConv[] =//用于遍历比较并进行单位换算
	{
		nanos_per_micro,
		micros_per_milli,
		millis_per_second,
		seconds_per_minute,
		minutes_per_hour,
	};

	enum UnitType : uint8_t
	{
		nano = 0,	//ns
		micro,		//us
		milli,		//ms
		second,		//s
		minute,		//min
		hour,		//h
		unknown,	//unknown

		UnitType_END,//end flag
	};

	static inline constexpr const char *const strUnitType[] = 
	{
		"ns",
		"us",
		"ms",
		"s",
		"min",
		"h",
		"unknown"
	};

	static_assert(sizeof(strUnitType) / sizeof(strUnitType[0]) == UnitType_END, "Array and enumeration lose synchronization");

	struct OutputData
	{
		UnitType enUnitType = nano;
		long double ldTime = 0.0;
	};

	OutputData GetOutputData(int64_t i64Nano)
	{
		OutputData ret{ .enUnitType = nano,.ldTime = (long double)i64Nano };

		for (const auto &it : arrConv)
		{
			if (ret.ldTime < (long double)it)
			{
				break;//如果当前在下一个单位范围内，则跳出
			}

			//否则切换并计算下一个单位
			ret.enUnitType = (UnitType)((uint8_t)ret.enUnitType + 1);
			ret.ldTime /= (long double)it;
		}

		//超出范围，设置为未知
		if (ret.enUnitType >= UnitType_END)
		{
			ret.enUnitType = unknown;
		}

		return ret;
	}
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
		//通过时差长度选择合适的单位
		auto [enUnitType, ldTime] = GetOutputData(Diff<std::chrono::nanoseconds>().count());
		printf("%s%.6lf%s%s", cpBegInfo, ldTime, strUnitType[enUnitType], cpEndInfo);
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