#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string>

//目前只做了写入，不要用读取模式打开
class CSV_Tool
{
private:
	FILE *pFile = NULL;
	bool bNewLine = true;
public:
	enum RW_Mode : uint8_t
	{
		Read = 0,
		Write,
		END_ENUM,
	};

	static inline constexpr const char *const listMode[] =
	{
		"rb",
		"wb",
	};

	static_assert(sizeof(listMode) / sizeof(listMode[0]) == END_ENUM);

	CSV_Tool(void) = default;

	CSV_Tool(const CSV_Tool &) = delete;
	CSV_Tool(CSV_Tool &&_Other) noexcept :
		pFile(_Other.pFile),//因为是构造可以直接赋值，不用先关闭原先的
		bNewLine(_Other.bNewLine)
	{
		_Other.pFile = NULL;
		_Other.bNewLine = true;
	}

	CSV_Tool(FILE *_pFile) :pFile(_pFile)
	{}

	CSV_Tool(const char *const pcFileName, RW_Mode enMode) :
		pFile(fopen(pcFileName, listMode[enMode])),//因为是构造可以直接赋值，不用先关闭原先的
		bNewLine(true)
	{}

	~CSV_Tool(void)
	{
		CloseFile();
	}

	CSV_Tool &operator=(const CSV_Tool &) = delete;
	CSV_Tool &operator=(CSV_Tool &&_Other) noexcept
	{
		CloseFile();//切记先关闭文件，这不是构造，之前可能存在已打开的文件
		pFile = _Other.pFile;
		bNewLine = _Other.bNewLine;
		_Other.pFile = NULL;
		_Other.bNewLine = true;
	}
	
	operator bool(void)
	{
		return pFile != NULL;
	}
	

	bool OpenFile(const char *const pcFileName, RW_Mode enMode)
	{
		CloseFile();
		pFile = fopen(pcFileName, listMode[enMode]);
		return pFile != NULL;
	}

	void CloseFile(void)
	{
		if (pFile != NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
	}

	static bool IsFileExist(const char *const pcFileName)
	{	
		FILE *pTmp = fopen(pcFileName, "rb");
		bool bExist = pTmp != NULL;
		if (bExist)
		{
			fclose(pTmp);
		}

		return bExist;
	}

	bool IsEof(void)
	{
		return feof(pFile);
	}

	void Flush(void)
	{
		fflush(pFile);//刷新一下写入磁盘
	}

	template<bool bEscape = true>
	void WriteOnce(const std::string &str)
	{
		WriteEmpty();

		fputc('\"', pFile);
		for (const auto &it : str)
		{
			if constexpr (bEscape)
			{
				if (it == '\"')//转义其中的"为""
				{
					fputc('\"', pFile);
				}
			}
			
			fputc(it, pFile);
		}
		fputc('\"', pFile);
		
	}

	void WriteEmpty(void)
	{
		if (!bNewLine)//不是新行开头，输出一个逗号
		{
			fputc(',', pFile);
		}
		else
		{
			bNewLine = false;//下一次输出就不是新行了
		}
	}

	void NewLine(void)
	{
		//写入CRLF
		fputc('\r', pFile);
		fputc('\n', pFile);
		bNewLine = true;//设置新行为true
	}
};