//#include "MemoryLeakCheck.hpp"
#include "Windows_ANSI.hpp"

#include "MUTF8_Tool.hpp"
#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"
#include "NBT_Reader.hpp"
#include "NBT_Writer.hpp"
#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "File_Tool.hpp"
#include "RegionProcess.h"
#include "CodeTimer.hpp"

#include "Language.hpp"
#include "CSV_Tool.hpp"
#include "CountFormatter.hpp"
#include "Windows_FileSystem.hpp"

/*Compress*/
#include "Compression_Utils.hpp"
/*Compress*/

#include <stdio.h>
#include <string>
#include <functional>
#include <format>
#include <stdexcept>
#include <filesystem>

//NBT一般使用GZIP压缩，也有部分使用ZLIB压缩

std::string CountFormat(int64_t u64Count)
{
	return CountFormatter::Level2String(CountFormatter::CalculateLevels(u64Count));
}


//判断是否存在cpdTag成员
template <typename T>
concept HasCpdTag = requires(T t)
{
	t.cpdTag;
};


template<Language::KeyType enKeyType, typename T>
void PrintInfo(const T &info, const Language &lang)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();

		//判断是否存在cpd成员，有则输出
		if constexpr (HasCpdTag<std::decay_t<decltype(refItem.first)>>)
		{
			printf("%s[%s]%s:%lld = %s\n",
				U8ANSI(lang.KeyTranslate(enKeyType, refItem.first.sName)).c_str(),
				refItem.first.sName.c_str(),
				U16ANSI(U16STR(NBT_Helper::Serialize(refItem.first.cpdTag))).c_str(),
				refItem.second,
				CountFormat(refItem.second).c_str());
		}
		else//无则跳过
		{
			printf("%s[%s]:%lld = %s\n",
				U8ANSI(lang.KeyTranslate(enKeyType, refItem.first.sName)).c_str(),
				refItem.first.sName.c_str(),
				refItem.second,
				CountFormat(refItem.second).c_str());
		}
	}
}


template<Language::KeyType enKeyType, typename T>
void PrintInfo(const T &info, const Language &lang, CSV_Tool &csv)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		PrintInfo<enKeyType>(info, lang);
		return;
	}

	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();

		csv.WriteOnce<true>(U8ANSI(lang.KeyTranslate(enKeyType, refItem.first.sName)));
		csv.WriteOnce<true>(refItem.first.sName);

		//判断是否存在cpd成员，有则输出
		if constexpr (HasCpdTag<std::decay_t<decltype(refItem.first)>>)
		{
			csv.WriteOnce<true>(U16ANSI(U16STR(NBT_Helper::Serialize(refItem.first.cpdTag))));
		}

		csv.WriteOnce<true>(std::format("{}个 = {}", refItem.second, CountFormat(refItem.second)));
		csv.NewLine();
	}
}


template<Language::KeyType enKeyType, Language::KeyType enParentKeyType, typename T>
void PrintInfo(const MapMSL<T> &info, const Language &lang)
{
	for (const auto &itParent : info)
	{
		if (itParent.second.listSort.empty())
		{
			continue;
		}

		printf("%s(%s):\n", U8ANSI(lang.KeyTranslate(enParentKeyType, itParent.first)).c_str(), itParent.first.c_str());
		PrintInfo<enKeyType>(itParent.second.listSort, lang);
	}
}


template<Language::KeyType enKeyType, Language::KeyType enParentKeyType, typename T>
void PrintInfo(const MapMSL<T> &info, const Language &lang, CSV_Tool &csv)
{
	//控制台重定向
	if (!csv)
	{
		PrintInfo<enKeyType, enParentKeyType>(info, lang);
		return;
	}

	//遍历不同的所有者
	for (const auto &itParent : info)
	{
		if (itParent.second.listSort.empty())
		{
			continue;
		}

		csv.WriteEmpty(5);//从第五个空格开始写入
		csv.WriteOnce<true>(U8ANSI(lang.KeyTranslate(enParentKeyType, itParent.first)));
		csv.WriteContinue<false>("(");
		csv.WriteContinue<true>(itParent.first);
		csv.WriteContinue<false>(")");
		PrintInfo<enKeyType>(itParent.second.listSort, lang, csv);
	}
}

//template<bool bEscape = true>
//void PrintLine(const std::string &s, CSV_Tool &csv, size_t szSlot = 0)
//{
//	if (!csv)//非就绪状态重定向输出到控制台
//	{
//		printf("==============%s==============\n", s.c_str());
//		return;
//	}
//
//	while (szSlot-- > 0)
//	{
//		csv.WriteEmpty();
//	}
//
//	csv.WriteOnce<bEscape>(s);
//	csv.NewLine();
//}

void PrintConsole(const std::string &s)
{
	printf("|%s", s.c_str());
}


template<bool bEscape = true, typename... Args>
void PrintLine(CSV_Tool &csv, Args&&... args)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		(PrintConsole(std::forward<Args>(args)), ...);
		printf("|\n");
		return;
	}

	csv.WriteLine(std::forward<Args>(args)...);
}

void PrintLine(CSV_Tool &csv)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		printf("\n");
		return;
	}

	csv.NewLine();
}


void Convert(const char *const pFileName)
{
	//计数器
	CodeTimer timer;
	printf("Current file:[%s]\n", pFileName);

	std::string sNbtData;
	timer.Start();
	if (!ReadFile(pFileName, sNbtData))
	{
		printf("Nbt File read fail\n");
		return;
	}
	timer.Stop();

	 printf("File read size:[%zu]\n", sNbtData.size());
	 timer.PrintElapsed("File read time:[", "]\n");

	 if (gzip::is_compressed(sNbtData.data(), sNbtData.size()))//如果nbt已压缩，解压，否则保持原样
	 {
		 timer.Start();
		 sNbtData = gzip::decompress(sNbtData.data(), sNbtData.size());
		 timer.Stop();

		 printf("File decompressed size:[%lld]\n", (uint64_t)sNbtData.size());
		 timer.PrintElapsed("Decompress time:[", "]\n");

#ifdef _DEBUG
		 //路径预处理
		 std::string sPath{ pFileName };
		 size_t szPos = sPath.find_last_of('.');//找到最后一个.获得后缀名的位置
		 if (szPos == std::string::npos)
		 {
			 szPos = sPath.size() - 1;//没有后缀就从末尾开始
		 }
		 //后缀名前插入自定义尾缀
		 sPath.insert(szPos, "_decompress");
		 printf("Output file:\"%s\" ", sPath.c_str());

		 //判断文件存在性
		 if (!IsFileExist(sPath))
		 {
			 //输出一个解压过的文件，用于在报错发生后供分析
			 FILE *pFile = fopen(sPath.c_str(), "wb");
			 if (pFile == NULL)
			 {
				 return;
			 }

			 timer.Start();
			 if (fwrite(sNbtData.data(), sizeof(sNbtData[0]), sNbtData.size(), pFile) != sNbtData.size())
			 {
				 return;
			 }
			 timer.Stop();

			 fclose(pFile);
			 pFile = NULL;

			 printf("is maked successfuly\n");
			 timer.PrintElapsed("Write file time:[", "]\n");
		 }
		 else
		 {
			 //文件已存在，不进行覆盖输出，跳过
			 printf("is already exist, skipped\n");
		 }
	 }
	 else
	 {
		 printf("File is not compressed\n");
	 }

#else
	 }
#endif // DEBUG


	//以下使用nbt
	NBT_Node nRoot;
	timer.Start();
	if (!NBT_Reader<std::string>::ReadNBT(nRoot, sNbtData))
	{
		printf("\nData before the error was encountered:");
		NBT_Helper::Print(nRoot);
		return;
	}
	timer.Stop();
	timer.PrintElapsed("Read NBT time:[", "]\n");

	//NBT_Helper::Print(nRoot);
	//return 0;

	const auto &tmp = nRoot.GetCompound();//获取根下第一个compound，正常情况下根部下只有这一个compound
	if (tmp.size() != 1)
	{
		printf("Error root size");
		return;
	}

	//输出名称（一般是空字符串）
	const auto &root = *tmp.begin();
#ifdef _DEBUG
	printf("root:\"%s\"\n", U16ANSI(U16STR(root.first)).c_str());
#endif

	timer.Start();
	RegionStatsList listRegionStats{};
	try
	{
		//获取regions，也就是区域，一个投影可能有多个区域（选区）
		const auto &cpdRegions = GetCompound(root.second).GetCompound(MU8STR("Regions"));
		listRegionStats = RegionProcess(cpdRegions);
	}
	catch (const std::out_of_range &e)//map的at查不到
	{
		printf("RegionProcess error: %s\n", e.what());
		return;
	}
	timer.Stop();
	timer.PrintElapsed("RegionProcess time:[", "]\n");

	Language lang;
	auto pathModule = GetCurrentModulePath();//使用程序当前路径查找语言文件，而非工作目录
	pathModule.append("zh_cn.json");
	timer.Start();
	bool bLangRead = lang.ReadLanguageFile(pathModule.string().c_str());
	timer.Stop();
	if (bLangRead)
	{
		timer.PrintElapsed("Language file read time:[", "]\n");
	}
	else
	{
		printf("Language file read fail\n");
	}

	//准备csv文件
	std::string sCsvPath{ pFileName };
	size_t szPos = sCsvPath.find_last_of('.');//找到最后一个.获得后缀名的位置
	if (szPos == std::string::npos)
	{
		szPos = sCsvPath.size() - 1;//没有后缀就从末尾开始
	}

	CSV_Tool csv;
#ifndef _DEBUG
	//找到一个合法文件名
	int32_t i32Count = 10;//最多重试10次
	do
	{
		//时间用[]包围
		auto tmpCurTime = std::string{ '[' } + std::to_string(CodeTimer::GetNowTime()) + std::string{ ']' };//获取当前系统时间
		sCsvPath.replace(szPos, std::string::npos, tmpCurTime);//放入尾部
		sCsvPath.append(".csv");//后缀名改成csv
	} while (IsFileExist(sCsvPath) && i32Count-- > 0);//如果文件已经存在，重试

	if (i32Count > 0)//如果没次数了就别打开了，直接让它失败
	{
		csv.OpenFile(sCsvPath.c_str(), CSV_Tool::Write);
		printf("Output file:[%s]", sCsvPath.c_str());
	}
#else
	csv.OpenFile("opt.csv", CSV_Tool::Write);
	printf("Output file:[opt.csv]");
#endif // !_DEBUG
	
	if (!csv)
	{
		printf("CSV file open fail\n");
	}
	//else
	//{
	//	csv.WriteOnce("名称(Name)");
	//	csv.WriteOnce("键名(Key)");
	//	csv.WriteOnce("标签(Tag)");
	//	csv.WriteOnce("数量(Count)");
	//	csv.WriteOnce("类型(Type)");
	//	csv.WriteOnce("区域(Region)");
	//	csv.WriteOnce("来源");
	//	csv.NewLine();
	//}


	timer.Start();
	//处理是否要合并选区（如果区域只有1个那无需合并，否则合并）
	if (listRegionStats.size() > 1)
	{
		auto tmpMerge = MergeRegionStatsList(listRegionStats);
		tmpMerge.sRegionName = MU8STR("[All Region]");
		listRegionStats.push_back(std::move(tmpMerge));//合并后插入尾部
	}
	//处理所有区域
	for (const auto &it : listRegionStats)
	{
		PrintLine(csv, "区域(Region)", '[' + U16ANSI(U16STR(it.sRegionName)) + ']');
	
		//PrintLine(csv);
		//PrintLine(csv, "类型(Type)", "[block]");
		//PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)");
		//PrintInfo<Language::Item, true>(it.mslBlock.listSort, lang, csv);//方块

		PrintLine(csv);
		PrintLine(csv, "类型(Type)", "[block item]");
		PrintLine(csv, "名称(Name)", "键名(Key)", "数量(Count)");
		PrintInfo<Language::Item>(it.mslBlockItem.listSort, lang, csv);//方块转物品

		PrintLine(csv);
		PrintLine(csv, "类型(Type)", "[tile entity container]");
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)");
		PrintInfo<Language::Item>(it.mslTileEntityContainer.listSort, lang, csv);//方块实体容器
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)", "来源(source)");
		PrintInfo<Language::Item, Language::Item>(it.mmslParentInfoTEC, lang, csv);//方块实体容器带容器名

		PrintLine(csv);
		PrintLine(csv, "类型(Type)", "[entity info]");
		PrintLine(csv, "名称(Name)", "键名(Key)", "数量(Count)");
		PrintInfo<Language::Entity>(it.mslEntity.listSort, lang, csv);//实体

		PrintLine(csv);
		PrintLine(csv, "类型(Type)", "[entity container]");
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)");
		PrintInfo<Language::Item>(it.mslEntityContainer.listSort, lang, csv);//实体容器
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)", "来源(source)");
		PrintInfo< Language::Item, Language::Entity>(it.mmslParentInfoEC, lang, csv);//实体容器带实体名

		PrintLine(csv);
		PrintLine(csv, "类型(Type)", "[entity inventory]");
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)");
		PrintInfo<Language::Item>(it.mslEntityInventory.listSort, lang, csv);//实体物品栏
		PrintLine(csv, "名称(Name)", "键名(Key)", "标签(Tag)", "数量(Count)", "来源(source)");
		PrintInfo<Language::Item, Language::Entity>(it.mmslParentInfoEI, lang, csv);//实体物品栏带实体名

		PrintLine(csv);
	}
	timer.Stop();
	timer.PrintElapsed("\nOutput time:[", "]\n");
}

int main(int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("At least one input file is required\n");
		return -1;
	}

	printf("[%d] file(s)\n", argc - 1);

	for (int i = 1; i < argc; ++i)
	{
		printf("\n[%d] ", i);
		Convert(argv[i]);
	}

	return 0;
}
