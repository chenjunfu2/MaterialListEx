#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>//size_t
#include <fstream>
#include <vector>
#include <filesystem>

#ifdef USE_GZIPZLIB

/*zlib*/
#define ZLIB_CONST
#include <zlib.h>
/*zlib*/

/*gzip*/
#include <compress.hpp>
#include <config.hpp>
#include <decompress.hpp>
#include <utils.hpp>
#include <version.hpp>
/*gzip*/

#endif

class NBT_IO
{
	NBT_IO(void) = delete;
	~NBT_IO(void) = delete;

public:
	template<typename T = std::vector<uint8_t>>
	static bool WriteFile(const std::string &strFileName, const T &tData)
	{
		std::fstream fWrite;
		fWrite.open(strFileName, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		if (!fWrite)
		{
			return false;
		}

		//获取文件大小并写出
		uint64_t qwFileSize = tData.size();
		if (!fWrite.write((const char *)tData.data(), sizeof(tData[0]) * qwFileSize))
		{
			return false;
		}

		//完成，关闭文件
		fWrite.close();

		return true;
	}

	template<typename T = std::vector<uint8_t>>
	static bool ReadFile(const std::string &strFileName, T &tData)
	{
		std::fstream fRead;
		fRead.open(strFileName, std::ios_base::binary | std::ios_base::in);
		if (!fRead)
		{
			return false;
		}

		//获取文件大小
		// 移动到文件末尾
		if (!fRead.seekg(0, std::ios::end))
		{
			return false;
		}

		// 获取文件大小
		size_t szFileSize = fRead.tellg();

		//回到文件开头
		if (!fRead.seekg(0, std::ios::beg))
		{
			return false;
		}

		//直接给数据塞string里
		tData.resize(szFileSize);//设置长度 c++23用resize_and_overwrite
		if (!fRead.read((char *)tData.data(), sizeof(tData[0]) * szFileSize))//直接读入data
		{
			return false;
		}

		//完成，关闭文件
		fRead.close();

		return true;
	}

	static bool IsFileExist(const std::string &sFileName)
	{
		std::error_code ec;//判断这东西是不是true确定有没有error
		bool bExists = std::filesystem::exists(sFileName, ec);

		return !ec && bExists;//没有错误并且存在
	}

#ifdef USE_GZIPZLIB

	template<typename T = std::vector<uint8_t>>
	static bool DecompressIfZipped(T &tData)
	{
		if (!gzip::is_compressed((const char *)tData.data(), tData.size()))
		{
			return true;
		}

		try
		{
			T tmpData{};
			gzip::Decompressor{ SIZE_MAX }.decompress(tmpData, (const char *)tData.data(), tData.size());
			tData = std::move(tmpData);
		}
		catch (const std::bad_alloc& e)
		{
			printf("std::bad_alloc:[%s]\n", e.what());
			return false;
		}
		catch (const std::exception &e)
		{
			printf("std::exception:[%s]\n", e.what());
			return false;
		}
		catch (...)
		{
			printf("Unknown Error\n");
			return false;
		}

		return true;
	}

#endif
};