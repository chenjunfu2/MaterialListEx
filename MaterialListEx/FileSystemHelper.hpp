#pragma once

#include <string>
#include <fstream>

#include <NBT_IO.hpp>
#include <nlohmann/json.hpp>
using Json = nlohmann::json;

#include "CodeTimer.hpp"

inline std::string GetFilenameWithoutExtension(const std::string &sFilePath)
{
	size_t szPos = sFilePath.find_last_of('.');//找到最后一个.获得后缀名的位置
	return sFilePath.substr(0, szPos);//然后返回子字符串
}


//找到一个唯一文件名
inline std::string GenerateUniqueFilename(const std::string &sBeg, const std::string &sEnd, uint32_t u32TryCount = 10)//默认最多重试10次
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

inline bool ReadJsonFile(const std::string &strFileName, Json &jsonRead)
{
	std::fstream fileStream(strFileName, std::ios_base::in | std::ios_base::binary);
	if (!fileStream.is_open())
	{
		printf("Json file [%s] open fail!\n", strFileName.c_str());
		return false;
	}

	//从语言文件创建json对象
	try
	{
		jsonRead = Json::parse(fileStream);
		return true;
	}
	catch (const Json::parse_error &e)
	{
		// 输出异常详细信息
		printf("Json parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);
		printf("Json file [%s] read fail!\n", strFileName.c_str());
		return false;
	}

	return false;
}

