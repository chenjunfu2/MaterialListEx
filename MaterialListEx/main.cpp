#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "CodeTimer.hpp"
#include "CurrentModulePath.hpp"
#include "LitematicToMaterialList.h"

#ifdef _WIN32
#include "Windows_ANSI.hpp"
#endif


//Win使用宽字符wmain
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

	if (argc < 2)
	{
		printf("At least one input file is required\n");
		return -1;
	}

	//计时器
	CodeTimer timer;

	//获取语言文件
	Language lang;
	auto pathLanguage = GetCurrentModulePath();//使用程序当前路径查找语言文件，而非工作目录
	pathLanguage.append("zh_cn.json");
	timer.Start();
	bool bLangRead = lang.ReadLanguageFile(pathLanguage.string());
	timer.Stop();
	if (bLangRead)
	{
		timer.PrintElapsed("Language file read time:[", "]\n");
	}
	else
	{
		printf("Language file read fail, ignore\n");//获取失败忽略错误
	}

	//获取物品堆叠文件
	CountFormatter cfItemStackCount;
	auto pathItemStackCount = GetCurrentModulePath();//使用程序当前路径查找物品堆叠格式化文件，而非工作目录
	pathItemStackCount.append("item_stack_count.json");
	timer.Start();
	bool bCfRead = cfItemStackCount.ReadCountFormatterFile(pathItemStackCount.string());
	timer.Stop();
	if (bCfRead)
	{
		timer.PrintElapsed("ItemStackCount file read time:[", "]\n");
	}
	else
	{
		printf("ItemStackCount file read fail, ignore\n");//获取失败忽略错误
	}

	//初始化完成
	printf("Initialization completed!\n\n");

	//打印文件计数
	int iTotal = argc - 1;
	printf("Total [%d] file(s)\n", iTotal);

	int iSucceed = 0;
	for (int i = 1; i < argc; ++i)//注意argc从1开始作为索引访问argv，因为argv[0]是程序自身路径
	{
		printf("\nCurrent [%d] ", i);
#ifdef _WIN32
		if (!LitematicToMaterialList(ConvertUtf16ToUtf8<wchar_t, char>(argv[i]), lang, cfItemStackCount))
#else
		if (!LitematicToMaterialList(argv[i], lang, cfItemStackCount))
#endif
		{
			printf("LitematicToMaterialList fail, skip\n");
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
