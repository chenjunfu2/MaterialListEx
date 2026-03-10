#pragma once

#include <string>
#include <NBT_IO.hpp>
#include "CodeTimer.hpp"

std::string GetFilenameWithoutExtension(const std::string &sFilePath)
{
	size_t szPos = sFilePath.find_last_of('.');//找到最后一个.获得后缀名的位置
	return sFilePath.substr(0, szPos);//然后返回子字符串
}


//找到一个唯一文件名
std::string GenerateUniqueFilename(const std::string &sBeg, const std::string &sEnd, uint32_t u32TryCount = 10)//默认最多重试10次
{
	while (u32TryCount != 0)
	{
		//时间用[]包围
		auto tmpPath = std::format("{}[{}]{}", sBeg, CodeTimer::GetSystemTime(), sEnd);//获取当前系统时间戳作为中间的部分
		if (!NBT_IO::IsFileExist(tmpPath))
		{
			return tmpPath;
		}

		//等几ms在继续
		CodeTimer::Sleep(std::chrono::milliseconds(10));
		--u32TryCount;
	}

	//次数到上限直接返回空
	return std::string{};
}
