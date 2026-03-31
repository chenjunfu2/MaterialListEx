#include "LitematicToMaterialList.h"

#include <stdexcept>
#include <stdio.h>
#include <inttypes.h>

#include <NBT_All.hpp>

#include "FileSystemHelper.hpp"

#include "PrintLog.hpp"
#include "CSV_Tool.hpp"
#include "CodeTimer.hpp"

#include "RegionProcess.h"

//判断是否存在cpdTag成员
template <typename T>
concept HasCpdTag = requires(T t)
{
	t.cpdTag;
};


template<Language::KeyType enKeyType, typename T>
void PrintInfo(const T &info, const Language &lang, const CountFormatter &cf)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();
		auto u8ItemName = refItem.first.sName.ToCharTypeUTF8();

		//判断是否存在cpd成员，有则输出
		if constexpr (HasCpdTag<std::decay_t<decltype(refItem.first)>>)
		{
			printf("%s[%s]%s:%" PRIu64 " = %s\n",
				lang.KeyTranslate(enKeyType, u8ItemName).c_str(),
				refItem.first.sName.c_str(),
				NBT_Helper::Serialize(refItem.first.cpdTag).c_str(),
				refItem.second,
				cf.CalculateLevels(u8ItemName, refItem.second).ToString().c_str());
		}
		else//无则跳过
		{
			printf("%s[%s]:%" PRIu64 " = %s\n",
				lang.KeyTranslate(enKeyType, u8ItemName).c_str(),
				refItem.first.sName.c_str(),
				refItem.second,
				cf.CalculateLevels(u8ItemName, refItem.second).ToString().c_str());
		}
	}
}


template<Language::KeyType enKeyType, typename T, typename U>
void PrintInfo(const T &info, const Language &lang, CSV_Tool<U> &csv, const CountFormatter &cf)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		PrintInfo<enKeyType>(info, lang, cf);
		return;
	}

	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();

		auto u8ItemName = refItem.first.sName.ToCharTypeUTF8();
		csv.template WriteOnce<true>(lang.KeyTranslate(enKeyType, u8ItemName));
		csv.template WriteOnce<true>(u8ItemName);

		//判断是否存在cpd成员，有则输出
		if constexpr (HasCpdTag<std::decay_t<decltype(refItem.first)>>)
		{
			csv.template WriteOnce<true>(NBT_Helper::Serialize(refItem.first.cpdTag));
		}

		csv.template WriteOnce<true>(std::format("{}个 = {}", refItem.second, cf.CalculateLevels(u8ItemName, refItem.second).ToString()));
		csv.NewLine();
	}
}


template<Language::KeyType enKeyType, Language::KeyType enParentKeyType, typename T>
void PrintInfo(const MapMSL<T> &info, const Language &lang, const CountFormatter &cf)
{
	for (const auto &itParent : info)
	{
		if (itParent.second.listSort.empty())
		{
			continue;
		}

		auto u8ParentName = itParent.first.ToCharTypeUTF8();
		printf("%s(%s):\n", lang.KeyTranslate(enParentKeyType, u8ParentName).c_str(), itParent.first.c_str());
		PrintInfo<enKeyType>(itParent.second.listSort, lang, cf);
	}
}


template<Language::KeyType enKeyType, Language::KeyType enParentKeyType, typename T, typename U>
void PrintInfo(const MapMSL<T> &info, const Language &lang, CSV_Tool<U> &csv, const CountFormatter &cf)
{
	//控制台重定向
	if (!csv)
	{
		PrintInfo<enKeyType, enParentKeyType>(info, lang, cf);
		return;
	}

	//遍历不同的所有者
	for (const auto &itParent : info)
	{
		if (itParent.second.listSort.empty())
		{
			continue;
		}

		auto u8ParentName = itParent.first.ToCharTypeUTF8();
		csv.WriteEmpty(5);//从第五个空格开始写入
		csv.WriteStart();//连续写入开始
		csv.template WriteContinue<true>(lang.KeyTranslate(enParentKeyType, u8ParentName));
		csv.template WriteContinue<false>(u8"(");
		csv.template WriteContinue<true>(u8ParentName);
		csv.template WriteContinue<false>(u8")");
		csv.WriteStop();//连续写入结束
		csv.NewLine();//换行

		PrintInfo<enKeyType>(itParent.second.listSort, lang, csv, cf);
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

void PrintConsole(const std::basic_string_view<char> &s)
{
	printf("|%s", s.data());
}

void PrintConsole(const std::basic_string_view<char8_t> &s)
{
	printf("|%s", (const char *)s.data());
}


template<bool bEscape = true, typename T, typename... Args>
void PrintLine(CSV_Tool<T> &csv, Args&&... args)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		(PrintConsole(std::forward<Args>(args)), ...);
		printf("|\n");
		return;
	}

	csv.WriteLine(std::forward<Args>(args)...);
}

template<typename T>
void PrintLine(CSV_Tool<T> &csv)
{
	if (!csv)//非就绪状态重定向输出到控制台
	{
		printf("\n");
		return;
	}

	csv.NewLine();
}


bool LitematicToMaterialList(const std::string &pathFile, const Language &lang, const CountFormatter &cf)
{
	//计数器
	CodeTimer timer;
	printf("Current file:[%s]\n", pathFile.c_str());

	std::vector<uint8_t> sNbtData;
	timer.Start();
	if (!NBT_IO::ReadFile(pathFile, sNbtData))
	{
		printf("Nbt File read fail\n");
		return false;
	}
	timer.Stop();

	printf("File read size:[%zu]\n", sNbtData.size());
	timer.PrintElapsed("File read time:[", "]\n");

	if (NBT_IO::IsDataZipped(sNbtData))//如果nbt已压缩，解压，否则保持原样
	{
		std::vector<uint8_t> tmp;
		timer.Start();
		bool bDcp = NBT_IO::DecompressDataNoThrow(tmp, sNbtData);
		timer.Stop();

		if (!bDcp)
		{
			printf("File decompress fail\n");
			printf("The data may not be compressed, try parsing it directly\n");
			goto process_nbt_data;//跳到下面直接处理
		}

		sNbtData = std::move(tmp);

		printf("File decompressed size:[%zu]\n", sNbtData.size());
		timer.PrintElapsed("File decompress time:[", "]\n");

#ifdef _DEBUG
		//路径预处理
		std::string sPath{ pathFile };
		size_t szPos = sPath.find_last_of('.');//找到最后一个.获得后缀名的位置
		if (szPos == std::string::npos)
		{
			szPos = sPath.size() - 1;//没有后缀就从末尾开始
		}
		//后缀名前插入自定义尾缀
		sPath.insert(szPos, "_decompress");
		printf("Output file:\"%s\" ", sPath.c_str());

		//判断文件存在性
		if (!NBT_IO::IsFileExist(sPath))
		{
			//输出一个解压过的文件，用于在报错发生后供分析
			FILE *pFile = fopen(sPath.c_str(), "wb");
			if (pFile == NULL)
			{
				return false;
			}

			timer.Start();
			if (fwrite(sNbtData.data(), sizeof(sNbtData[0]), sNbtData.size(), pFile) != sNbtData.size())
			{
				return false;
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

process_nbt_data:
	//以下使用nbt
	NBT_Type::Compound nRoot{};
	timer.Start();
	if (!NBT_Reader::ReadNBT(sNbtData, 0, nRoot, 512, PrintLog<true>{ GenerateUniqueFilename(GetFilenameWithoutExtension(pathFile), "_err.log"), "ReadNBT Error!\n\n" }))
	{
		NBT_Helper::Print(nRoot, 0, "   ", PrintLog<false>{ GenerateUniqueFilename(GetFilenameWithoutExtension(pathFile), "_other_info.log"), "Data before the error was encountered:\n\n" });
		printf("Read NBT Error, Please check the log in the target folder.\n");
		return false;
	}
	timer.Stop();
	timer.PrintElapsed("Read NBT time:[", "]\n");

	//NBT_Helper::Print(nRoot);

	//正常mc投影nbt内只有一个Compound
	if (nRoot.Size() != 1)
	{
		printf("Error root size");
		return false;
	}

	//输出名称（一般是空字符串）
	const auto &root = *nRoot.begin();
#ifdef _DEBUG
	printf("root:\"%s\"\n", root.first.ToCharTypeUTF8().c_str());
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
		return false;
	}
	catch (const std::exception &e)
	{
		printf("RegionProcess error: %s\n", e.what());
		return false;
	}
	catch (...)
	{
		printf("RegionProcess error: Unknown Error!\n");
		return false;
	}
	timer.Stop();
	timer.PrintElapsed("RegionProcess time:[", "]\n");

	//准备csv文件
	//注意此处还未初始化
	CSV_Tool<char8_t> csv{};
#ifndef _DEBUG
	//找到一个合法文件名
	auto sCsvPath = GenerateUniqueFilename(GetFilenameWithoutExtension(pathFile), ".csv");

	if (!sCsvPath.empty())
	{
		csv.OpenFile(sCsvPath.c_str(), csv.Write);
		printf("Output file:[%s]", sCsvPath.c_str());
	}
#else
	csv.OpenFile("opt.csv", csv.Write);
	printf("Output file:[opt.csv]");
#endif // !_DEBUG

	if (!csv)
	{
		printf("CSV file open fail\n");
	}

	//写入UTF-8 BOM头
	csv.WriteRaw({ "\xEF\xBB\xBF", 3 });

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
		PrintLine(csv, u8"区域(Region)", u8'[' + it.sRegionName.ToUTF8() + u8']');

		//PrintLine(csv);
		//PrintLine(csv, u8"类型(Type)", u8"[block]");
		//PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)");
		//PrintInfo<Language::Item, true>(it.mslBlock.listSort, lang, csv);//方块

		PrintLine(csv);
		PrintLine(csv, u8"类型(Type)", u8"[block item]");
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"数量(Count)");
		PrintInfo<Language::Item>(it.mslBlockItem.listSort, lang, csv, cf);//方块转物品

		PrintLine(csv);
		PrintLine(csv, u8"类型(Type)", u8"[tile entity container]");
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)");
		PrintInfo<Language::Item>(it.mslTileEntityContainer.listSort, lang, csv, cf);//方块实体容器
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)", u8"来源(source)");
		PrintInfo<Language::Item, Language::Item>(it.mmslParentInfoTEC, lang, csv, cf);//方块实体容器带容器名

		PrintLine(csv);
		PrintLine(csv, u8"类型(Type)", u8"[entity info]");
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"数量(Count)");
		PrintInfo<Language::Entity>(it.mslEntity.listSort, lang, csv, cf);//实体

		PrintLine(csv);
		PrintLine(csv, u8"类型(Type)", u8"[entity container]");
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)");
		PrintInfo<Language::Item>(it.mslEntityContainer.listSort, lang, csv, cf);//实体容器
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)", u8"来源(source)");
		PrintInfo<Language::Item, Language::Entity>(it.mmslParentInfoEC, lang, csv, cf);//实体容器带实体名

		PrintLine(csv);
		PrintLine(csv, u8"类型(Type)", u8"[entity inventory]");
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)");
		PrintInfo<Language::Item>(it.mslEntityInventory.listSort, lang, csv, cf);//实体物品栏
		PrintLine(csv, u8"名称(Name)", u8"键名(Key)", u8"标签(Tag)", u8"数量(Count)", u8"来源(source)");
		PrintInfo<Language::Item, Language::Entity>(it.mmslParentInfoEI, lang, csv, cf);//实体物品栏带实体名

		PrintLine(csv);
	}
	timer.Stop();
	timer.PrintElapsed("\nOutput time:[", "]\n");

	return true;
}
