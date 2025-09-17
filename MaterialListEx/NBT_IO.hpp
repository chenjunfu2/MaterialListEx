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

		//��ȡ�ļ���С��д��
		uint64_t qwFileSize = u8Data.size();
		if (fwrite(u8Data.data(), sizeof(u8Data[0]), qwFileSize, pFile) != qwFileSize)
		{
			return false;
		}

		//��ɣ��ر��ļ�
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
		//��ȡ�ļ���С
		if (_fseeki64(pFile, 0, SEEK_END) != 0)
		{
			return false;
		}
		uint64_t qwFileSize = _ftelli64(pFile);
		//�ص��ļ���ͷ
		rewind(pFile);

		//ֱ�Ӹ�������string��
		u8Data.resize(qwFileSize);//���ó��� c++23��resize_and_overwrite
		if (fread(u8Data.data(), sizeof(u8Data[0]), qwFileSize, pFile) != qwFileSize)//ֱ�Ӷ���data
		{
			return false;
		}
		//��ɣ��ر��ļ�
		fclose(pFile);
		pFile = NULL;

		return true;
	}

	static bool IsFileExist(const std::string &sFileName)
	{
		std::error_code ec;//�ж��ⶫ���ǲ���trueȷ����û��error
		bool bExists = std::filesystem::exists(sFileName, ec);

		return !ec && bExists;//û�д����Ҵ���
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