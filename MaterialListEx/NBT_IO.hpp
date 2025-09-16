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
public:
	static bool WriteFile(const char *const _FileName, const std::basic_string<uint8_t> &_Data)
	{
		FILE *pFile = fopen(_FileName, "wb");
		if (pFile == NULL)
		{
			return false;
		}

		//获取文件大小并写出
		uint64_t qwFileSize = _Data.size();
		if (fwrite(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile) != qwFileSize)
		{
			return false;
		}

		//完成，关闭文件
		fclose(pFile);
		pFile = NULL;

		return true;
	}

	static bool ReadFile(const char *const _FileName, std::basic_string<uint8_t> &_Data)
	{
		FILE *pFile = fopen(_FileName, "rb");
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
		_Data.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
		if (fread(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile) != qwFileSize)//直接读入data
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
};