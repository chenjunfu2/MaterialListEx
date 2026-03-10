#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "CurrentModulePath.hpp"

#ifdef _WIN32
#include "Windows_ANSI.hpp"
#endif

//前向声明
bool Convert(const std::string pathFile);

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	if (argc >= 1)
	{
		SetCurrentModulePath(argv[0]);
	}

	if (setlocale(LC_ALL, "zh_CN.UTF-8") == NULL &&
		setlocale(LC_ALL, "en_US.UTF-8") == NULL)
	{
		printf("Warning: setlocale failed. Text might not display correctly.\n\n");
	}

	if (argc <= 1)
	{
		printf("At least one input file is required\n");
		return -1;
	}

	int iTotal = argc - 1;

	printf("total [%d] file(s)\n", iTotal);

	int iSucceed = 0;
	for (int i = 1; i < argc; ++i)//注意argc从1开始作为索引访问argv，因为argv[0]是程序自身路径
	{
		printf("\n[%d] ", i);
#ifdef _WIN32
		if (!Convert(ConvertUtf16ToUtf8<wchar_t, char>(argv[i]).c_str()))
#else
		if (!Convert(argv[i])
#endif
		{
			printf("Convert Error, Skip\n");
		}
		else
		{
			++iSucceed;
		}
	}

	printf("\nConversion completed.\n[%d]total, [%d]successful, [%d]failed\n\n", iTotal, iSucceed, iTotal - iSucceed);

#ifdef _WIN32
	//借用一下cmd命令暂停，防止闪现看不到输出
	system("pause");//仅Windows
#endif

	return 0;
}
