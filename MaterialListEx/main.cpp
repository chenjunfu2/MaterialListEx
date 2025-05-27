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

#include "Windows_ANSI.hpp"

/*Json*/
#include <nlohmann\json.hpp>
using Json = nlohmann::json;
/*Json*/

/*Compress*/
#include "Compression_Utils.hpp"
/*Compress*/

#include <stdio.h>
#include <string>
#include <functional>

//NBT一般使用GZIP压缩，也有部分使用ZLIB压缩

template<typename T>
void PrintInfo(const T &info)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();
		printf("[%s]%s:%lld\n", refItem.first.sName.c_str(), ANSISTR(U16STR(NBT_Helper::Serialize(refItem.first.cpdTag))).c_str(), refItem.second);
	}
}

template<typename T>
void PrintNoTagInfo(const T &info)
{
	for (const auto &itItem : info)
	{
		const auto &refItem = itItem.get();
		printf("[%s]:%lld\n", refItem.first.sName.c_str(), refItem.second);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Only one input file is needed\n");
		return -1;
	}

	std::string sNbtData;
	if (!ReadFile(argv[1], sNbtData))
	{
		printf("Nbt File Read Fail\n");
		return -1;
	}

	 printf("NBT file read size: [%zu]\n", sNbtData.size());

	if (gzip::is_compressed(sNbtData.data(), sNbtData.size()))//如果nbt已压缩，解压，否则保持原样
	{
		sNbtData = gzip::decompress(sNbtData.data(), sNbtData.size());
		printf("NBT file decompressed size: [%lld]\n", (uint64_t)sNbtData.size());

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

			if (fwrite(sNbtData.data(), sizeof(sNbtData[0]), sNbtData.size(), pFile) != sNbtData.size())
			{
				return -1;
			}

			fclose(pFile);
			pFile = NULL;

			printf("is maked successfuly\n");
		}
	}
	else
	{
		printf("NBT file is not compressed\n");
	}

	//以下使用nbt
	NBT_Node nRoot;
	if (!NBT_Reader<std::string>::ReadNBT(nRoot, sNbtData))
	{
		printf("\nData before the error was encountered:");
		NBT_Helper::Print(nRoot);
		return -1;
	}
	else
	{
		printf("Read Ok!\n");
	}

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
	printf("root:\"%s\"\n", ANSISTR(U16STR(root.first)).c_str());
	

	//获取regions，也就是区域，一个投影可能有多个区域（选区）
	const auto &cpdRegions = GetCompound(root.second).GetCompound(MU8STR("Regions"));

	auto start = std::chrono::steady_clock::now();
	auto vtRegionStats = RegionProcess(cpdRegions);
	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	printf("use:%lldus\n", duration.count());

	for (const auto &it : vtRegionStats)
	{
		printf("==============Region:[%s]==============\n", ANSISTR(U16STR(it.sRegionName)).c_str());
		
		printf("\n==============[block item]==============\n");
		PrintNoTagInfo(it.mslBlockItem.vecSortItem);//方块转物品
		printf("\n========[tile entity container]========\n");
		PrintInfo(it.mslTileEntityContainer.vecSortItem);//方块实体容器
		printf("\n=============[entity info]=============\n");
		PrintNoTagInfo(it.mslEntity.vecSortItem);//实体
		printf("\n===========[entity container]===========\n");
		PrintInfo(it.mslEntityContainer.vecSortItem);//实体容器
		printf("\n===========[entity inventory]===========\n");
		PrintInfo(it.mslEntityInventory.vecSortItem);//实体物品栏
	}
	
	return 1145;

	/*

	//准备读取
	Json zh_cn;
	bool bLanguage = true;

	//读取json语言文件
	std::string sJsonData;
	if (!ReadFile("zh_cn__.json", sJsonData))
	{
		printf("Json File Read Fail\n");
		bLanguage = false;
	}
	else
	{
		//从语言文件创建json对象
		try
		{
			zh_cn = Json::parse(sJsonData);
		}
		catch (const Json::parse_error &e)
		{
			// 输出异常详细信息
			printf("JSON parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);
			bLanguage = false;
		}
	}


	for (const auto &itRgSt : vtRegionStats)
	{
		printf("Region:[%s]\n", ANSISTR(U16STR(*itRgSt.psRegionName)).c_str());
		for (const auto &itItem : itRgSt.mslBlock.vecSortItem)
		{
			const auto &[sItemName, u64ItemCount] = itItem.get();
			if (bLanguage)
			{
				const auto it = zh_cn.find(sItemName);
				if (it != zh_cn.end() && it->is_string())//转化为中文输出
				{
					printf("%s [%s]: %llu\n", ConvertUtf8ToAnsi(it->get<std::string>()).c_str(), sItemName.c_str(), u64ItemCount);
				}
				else
				{
					printf("[Unknown] [%s]: %llu\n", sItemName.c_str(), u64ItemCount);
				}
			}
			else
			{
				printf("%s: %llu\n", sItemName.c_str(), u64ItemCount);
			}
		}

	}
	*/
	return 0;
}

