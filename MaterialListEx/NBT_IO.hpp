#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <filesystem>

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

class NBT_IO
{
	NBT_IO(void) = delete;
	~NBT_IO(void) = delete;
public:
	static bool WriteFile(const char *const pcFileName, const std::basic_string<uint8_t> &u8Data)
	{
		FILE *pFile = fopen(pcFileName, "wb");
		if (pFile == NULL)
		{
			return false;
		}

		//获取文件大小并写出
		uint64_t qwFileSize = u8Data.size();
		if (fwrite(u8Data.data(), sizeof(u8Data[0]), qwFileSize, pFile) != qwFileSize)
		{
			return false;
		}

		//完成，关闭文件
		fclose(pFile);
		pFile = NULL;

		return true;
	}

	static bool ReadFile(const char *const pcFileName, std::basic_string<uint8_t> &u8Data)
	{
		FILE *pFile = fopen(pcFileName, "rb");
		if (pFile == NULL)
		{
			return false;
		}
		//获取文件大小
		if (_fseeki64(pFile, 0, SEEK_END) != 0)
		{
			return false;
		}
		uint64_t qwFileSize = _ftelli64(pFile);
		//回到文件开头
		rewind(pFile);

		//直接给数据塞string里
		u8Data.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
		if (fread(u8Data.data(), sizeof(u8Data[0]), qwFileSize, pFile) != qwFileSize)//直接读入data
		{
			return false;
		}
		//完成，关闭文件
		fclose(pFile);
		pFile = NULL;

		return true;
	}

	static bool IsFileExist(const std::string &sFileName)
	{
		std::error_code ec;//判断这东西是不是true确定有没有error
		bool bExists = std::filesystem::exists(sFileName, ec);

		return !ec && bExists;//没有错误并且存在
	}

	static bool DecompressIfZipped(std::basic_string<uint8_t> &u8Data)
	{
		if (!gzip::is_compressed((const char *)u8Data.data(), u8Data.size()))
		{
			return true;
		}

		try
		{
			std::basic_string<uint8_t> tmpData{};
			gzip::Decompressor{ SIZE_MAX }.decompress(tmpData, (const char *)u8Data.data(), u8Data.size());
			u8Data = std::move(tmpData);
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
};