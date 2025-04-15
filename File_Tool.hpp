#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>

bool ReadFile(const char *const _FileName, std::string &_Data)
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
	fread(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile);//直接读入data
	//完成，关闭文件
	fclose(pFile);
	pFile = NULL;

	return true;
}