#define ZLIB_CONST
#include <zlib.h>

#include <compress.hpp>
#include <config.hpp>
#include <decompress.hpp>
#include <utils.hpp>
#include <version.hpp>

#include <stdio.h>
#include <string>
#include <Windows.h>

#include "NBT_Tool.hpp"

bool OpenFileAndMapping(const char *pcFileName, uint8_t **pFileRet, uint64_t *pFileSizeRet)
{
	//打开输入文件并映射
	HANDLE hReadFile = CreateFileA(pcFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReadFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	//获得文件大小
	LARGE_INTEGER liFileSize = { 0 };
	if (!GetFileSizeEx(hReadFile, &liFileSize))
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}
	*pFileSizeRet = liFileSize.QuadPart;

	//判断文件为空
	if (liFileSize.QuadPart == 0)
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW(hReadFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hFileMapping)
	{
		CloseHandle(hReadFile);//关闭输入文件
		return false;
	}
	CloseHandle(hReadFile);//关闭输入文件

	//映射文件到内存
	LPVOID lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!lpReadMem)
	{
		CloseHandle(hFileMapping);//关闭文件映射对象
		return false;
	}
	CloseHandle(hFileMapping);//关闭文件映射对象

	*pFileRet = (uint8_t *)lpReadMem;
	return true;
}

bool UnMappingAndCloseFile(uint8_t *pFileClose)
{
	if (!UnmapViewOfFile(pFileClose))
	{
		return false;
	}
	return true;
}



int main(void)
{
	uint8_t *pFile;
	uint64_t qwFileSize;
	if (!OpenFileAndMapping(".\\散热器水流刷怪塔（有合成器）.litematic", &pFile, &qwFileSize))
	{
		return -1;
	}

	std::string nbt;

	if (gzip::is_compressed((char *)pFile, qwFileSize))//如果nbt已压缩，解压
	{
		nbt = gzip::decompress((char *)pFile, qwFileSize);
		FILE *f = fopen("opt.nbt", "wb");
		if (f == NULL)
		{
			return -3;
		}
		
		if (fwrite(nbt.c_str(), nbt.size(), 1, f) != 1)
		{
			return -4;
		}
		
		fclose(f);
	}
	else//否则
	{
		//直接给数据塞string里
		nbt.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
		memcpy(nbt.data(), pFile, qwFileSize);//直接写入data
	}
	
	if (!UnMappingAndCloseFile(pFile))
	{
		return -2;
	}
	
	//以下使用nbt
	NBT_Tool nt(nbt);
	nt.Print();
	//NBT_Node n;

	printf("\nok!\n");

	return 114514;
}