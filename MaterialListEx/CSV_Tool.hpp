#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string>

//Ŀǰֻ����д�룬��Ҫ�ö�ȡģʽ��
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
		pFile(_Other.pFile),//��Ϊ�ǹ������ֱ�Ӹ�ֵ�������ȹر�ԭ�ȵ�
		bNewLine(_Other.bNewLine)
	{
		_Other.pFile = NULL;
		_Other.bNewLine = true;
	}

	CSV_Tool(FILE *_pFile) :pFile(_pFile)
	{}

	CSV_Tool(const char *const pcFileName, RW_Mode enMode) :
		pFile(fopen(pcFileName, listMode[enMode])),//��Ϊ�ǹ������ֱ�Ӹ�ֵ�������ȹر�ԭ�ȵ�
		bNewLine(true)
	{}

	~CSV_Tool(void)
	{
		CloseFile();
	}

	CSV_Tool &operator=(const CSV_Tool &) = delete;
	CSV_Tool &operator=(CSV_Tool &&_Other) noexcept
	{
		CloseFile();//�м��ȹر��ļ����ⲻ�ǹ��죬֮ǰ���ܴ����Ѵ򿪵��ļ�
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
		fflush(pFile);//ˢ��һ��д�����
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
				if (it == '\"')//ת�����е�"Ϊ""
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
		if (!bNewLine)//�������п�ͷ�����һ������
		{
			fputc(',', pFile);
		}
		else
		{
			bNewLine = false;//��һ������Ͳ���������
		}
	}

	void NewLine(void)
	{
		//д��CRLF
		fputc('\r', pFile);
		fputc('\n', pFile);
		bNewLine = true;//��������Ϊtrue
	}
};