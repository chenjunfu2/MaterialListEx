#pragma once

#include <fstream>
#include <string>
#include <format>

//重载一个日志输出类
//用于在nbt读取报错的时候输出错误log
template<bool bDelayOpenFile>
class PrintLog
{
private:
	const std::string sFileName;
	const std::string sFileHead;
	std::fstream fOpt;
	bool bOpenFail;

private:
	void OpenAndWriteHead(void)
	{
		fOpt.open(sFileName, std::ios_base::out | std::ios_base::trunc);
		fOpt.write((const char *)sFileHead.data(), sizeof(sFileHead.data()[0]) * sFileHead.size());
		if (!fOpt)
		{
			bOpenFail = true;
		}
	}

public:
	PrintLog(const std::string &_sFileName, const std::string &_sFileHead = {}) : sFileName(_sFileName), sFileHead(_sFileHead), fOpt(), bOpenFail(false)
	{
		if constexpr (!bDelayOpenFile)
		{
			OpenAndWriteHead();
		}
	}
	~PrintLog(void) = default;
	PrintLog(PrintLog &&) noexcept = default;
	PrintLog(const PrintLog &) = default;

	template<typename... Args>
	void operator()(const std::FMT_STR<Args...> fmt, Args&&... args) noexcept
	{
		return operator()(NBT_Print_Level::Info, std::move(fmt), std::forward<Args>(args)...);
	}

	template<typename... Args>
	void operator()(NBT_Print_Level lvl, const std::FMT_STR<Args...> fmt, Args&&... args) noexcept
	{
		try
		{
			auto tmp = std::format(std::move(fmt), std::forward<Args>(args)...);

			if constexpr (bDelayOpenFile)
			{
				if (!fOpt.is_open())
				{
					OpenAndWriteHead();
				}
			}

			if (!bOpenFail)
			{
				fOpt.write((const char *)tmp.data(), sizeof(tmp.data()[0]) * tmp.size());
			}
			else
			{
				fwrite(tmp.data(), sizeof(tmp.data()[0]), tmp.size(), stderr);
			}
		}
		catch (const std::exception &e)
		{
			fprintf(stderr, "PrintLog Exception: \"%s\"\n", e.what());
		}
		catch (...)
		{
			fprintf(stderr, "PrintLog Exception: \"Unknown Error\"\n");
		}
	}
};