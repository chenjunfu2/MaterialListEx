//#include "MemoryLeakCheck.hpp"

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

#include "Windows_ANSI.hpp"
#include "Language.hpp"

/*Compress*/
#include "Compression_Utils.hpp"
/*Compress*/

#include <stdio.h>
#include <string>
#include <functional>

//NBT一般使用GZIP压缩，也有部分使用ZLIB压缩

template<Language::KeyType enKeyType, typename T>
void PrintInfo(const T &info, const Language &lang)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();
		printf("%s[%s]%s:%lld\n",
			U8ANSI(lang.KeyTranslate(enKeyType, refItem.first.sName)).c_str(),
			refItem.first.sName.c_str(),
			U16ANSI(U16STR(NBT_Helper::Serialize(refItem.first.cpdTag))).c_str(),
			refItem.second);
	}
}

template<Language::KeyType enKeyType, typename T>
void PrintNoTagInfo(const T &info, const Language &lang)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();
		printf("%s[%s]:%lld\n",
			U8ANSI(lang.KeyTranslate(enKeyType, refItem.first.sName)).c_str(),
			refItem.first.sName.c_str(),
			refItem.second);
	}
}

int main(int argc, char *argv[])
{
	CodeTimer timer;

	if (argc != 2)
	{
		printf("Only one input file is needed\n");
		return -1;
	}

	std::string sNbtData;
	timer.Start();
	if (!ReadFile(argv[1], sNbtData))
	{
		printf("Nbt File read fail\n");
		return -1;
	}
	timer.Stop();

	 printf("File read size: [%zu]\n", sNbtData.size());
	 timer.PrintElapsed("File read time:[", "]\n");

	if (gzip::is_compressed(sNbtData.data(), sNbtData.size()))//如果nbt已压缩，解压，否则保持原样
	{
		timer.Start();
		sNbtData = gzip::decompress(sNbtData.data(), sNbtData.size());
		timer.Stop();

		printf("File decompressed size: [%lld]\n", (uint64_t)sNbtData.size());
		timer.PrintElapsed("decompress time:[", "]\n");

		//路径预处理
		std::string sPath{ argv[1] };
		size_t szPos = sPath.find_last_of('.');//找到最后一个.获得后缀名前的位置
		if (szPos == std::string::npos)
		{
			szPos = sPath.size() - 1;//没有后缀就从末尾开始
		}
		//后缀名前插入自定义尾缀
		sPath.insert(szPos, "_decompress");
		printf("Output file: \"%s\" ", sPath.c_str());

		//判断文件存在性
		FILE *pTest = fopen(sPath.c_str(), "rb");
		if (pTest != NULL)
		{
			//文件已存在，不进行覆盖输出，跳过
			printf("is already exist, skipped\n");
			fclose(pTest);
			pTest = NULL;
		}
		else
		{
			//输出一个解压过的文件，用于在报错发生后供分析
			FILE *pFile = fopen(sPath.c_str(), "wb");
			if (pFile == NULL)
			{
				return -1;
			}

			timer.Start();
			if (fwrite(sNbtData.data(), sizeof(sNbtData[0]), sNbtData.size(), pFile) != sNbtData.size())
			{
				return -1;
			}
			timer.Stop();

			fclose(pFile);
			pFile = NULL;

			printf("is maked successfuly\n");
			timer.PrintElapsed("Write file time:[", "]\n");
		}
	}
	else
	{
		printf("File is not compressed\n");
	}

	//以下使用nbt
	NBT_Node nRoot;
	timer.Start();
	if (!NBT_Reader<std::string>::ReadNBT(nRoot, sNbtData))
	{
		printf("\nData before the error was encountered:");
		NBT_Helper::Print(nRoot);
		return -1;
	}
	timer.Stop();

	printf("NBT read ok!\n");
	timer.PrintElapsed("Read NBT time:[", "]\n");

	//NBT_Helper::Print(nRoot);
	//return 0;

	const auto &tmp = nRoot.GetCompound();//获取根下第一个compound，正常情况下根部下只有这一个compound
	if (tmp.size() != 1)
	{
		printf("Error root size");
		return -1;
	}

	//输出名称（一般是空字符串）
	const auto &root = *tmp.begin();
	printf("root:\"%s\"\n", U16ANSI(U16STR(root.first)).c_str());
	

	//获取regions，也就是区域，一个投影可能有多个区域（选区）
	const auto &cpdRegions = GetCompound(root.second).GetCompound(MU8STR("Regions"));

	timer.Start();
	auto vtRegionStats = RegionProcess(cpdRegions);
	timer.Stop();
	timer.PrintElapsed("RegionProcess time:[", "]\n");

	Language lang;
	timer.Start();
	lang.ReadLanguageFile("zh_cn.json");
	timer.Stop();
	timer.PrintElapsed("Language file read time:[", "]\n");

	for (const auto &it : vtRegionStats)
	{
		printf("==============Region:[%s]==============\n", U16ANSI(U16STR(it.sRegionName)).c_str());
		
		printf("\n================[block]================\n");
		PrintInfo<Language::Item>(it.mslBlock.listSort, lang);//方块
		printf("\n==============[block item]==============\n");
		PrintNoTagInfo<Language::Item>(it.mslBlockItem.listSort, lang);//方块转物品
		printf("\n========[tile entity container]========\n");
		PrintInfo<Language::Item>(it.mslTileEntityContainer.listSort, lang);//方块实体容器
		printf("\n=============[entity info]=============\n");
		PrintNoTagInfo<Language::Entity>(it.mslEntity.listSort, lang);//实体
		printf("\n===========[entity container]===========\n");
		PrintInfo<Language::Item>(it.mslEntityContainer.listSort, lang);//实体容器
		printf("\n===========[entity inventory]===========\n");
		PrintInfo<Language::Item>(it.mslEntityInventory.listSort, lang);//实体物品栏
	}
	
	return 0;
}

