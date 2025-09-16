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

		//��ȡ�ļ���С��д��
		uint64_t qwFileSize = _Data.size();
		if (fwrite(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile) != qwFileSize)
		{
			return false;
		}

		//��ɣ��ر��ļ�
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
		//��ȡ�ļ���С
		if (_fseeki64(pFile, 0, SEEK_END) != 0)
		{
			return false;
		}
		uint64_t qwFileSize = _ftelli64(pFile);
		//�ص��ļ���ͷ
		rewind(pFile);

		//ֱ�Ӹ�������string��
		_Data.resize(qwFileSize);//���ó��� c++23��resize_and_overwrite
		if (fread(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile) != qwFileSize)//ֱ�Ӷ���data
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
};