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
	fread(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile);//ֱ�Ӷ���data
	//��ɣ��ر��ļ�
	fclose(pFile);
	pFile = NULL;

	return true;
}