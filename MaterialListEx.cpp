//#include "MemoryLeakCheck.hpp"

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "NBT_Helper.hpp"
#include "NBT_Reader.hpp"
#include "NBT_Writer.hpp"
#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "File_Tool.hpp"
#include "Compression_Utils.hpp"

/*Json*/
#include <nlohmann\json.hpp>
using Json = nlohmann::json;
/*Json*/

#include <stdio.h>
#include <string>
#include <functional>

//NBT一般使用GZIP压缩，也有部分使用ZLIB压缩

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
	

	struct RegionStats
	{
		const NBT_Node::NBT_String *psRegionName{};
		struct
		{
			std::map<NBT_Node::NBT_String, uint64_t> mapItemCounter;//创建方块状态到物品映射map
			using MapPair = decltype(mapItemCounter)::value_type;
			std::vector<std::reference_wrapper<MapPair>> vecSortItem{};//通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>
		};

		static bool SortCmp(MapPair &l, MapPair &r)
		{
			if (l.second == r.second)//相等情况下按key的字典序
			{
				return l.first < r.first;//升序
			}
			else//否则按数值大小
			{
				return l.second > r.second;//降序
			}
		}
	};

	//获取regions，也就是区域，一个投影可能有多个区域（选区）
	const auto &cpRegions = GetCompound(root.second).GetCompound(MU8STR("Regions"));
	std::vector<RegionStats> vtRegionStats;
	vtRegionStats.reserve(cpRegions.size());//提前扩容
	for (const auto &[RgName, RgVal] : cpRegions)//遍历选区
	{
		RegionStats rgsData{ &RgName };
		auto &RgCompound = GetCompound(RgVal);

		//方块处理
		{
			auto vtBlockStats = BlockProcess::GetBlockStats(RgCompound);//获取方块统计列表
			for (const auto &itBlock : vtBlockStats)
			{
				auto istItemList = BlockProcess::BlockStatsToItemStack(itBlock);
				for (const auto &itItem : istItemList)
				{
					rgsData.mapItemCounter[itItem.sItemName] += itItem.u64Counter;//如果key不存在，则自动创建，且保证value为0
				}
			}

			//提前扩容减少插入开销
			rgsData.vecSortItem.reserve(rgsData.mapItemCounter.size());
			rgsData.vecSortItem.assign(rgsData.mapItemCounter.begin(), rgsData.mapItemCounter.end());//迭代器范围插入
			//进行排序
			std::sort(rgsData.vecSortItem.begin(), rgsData.vecSortItem.end(), RegionStats::SortCmp);
		}

		//方块实体处理
		{
			auto vtTEItemStats = TileEntityProcess::GetTileEntityItemStats(RgCompound);

		}


		vtRegionStats.emplace_back(std::move(rgsData));
	}

	
	for (const auto &itRgSt : vtRegionStats)
	{
		printf("Region:[%s]\n", ANSISTR(U16STR(*itRgSt.psRegionName)).c_str());
		for (const auto &itItem : itRgSt.vecSortItem)
		{
			const auto &[sItemName, u64ItemCount] = itItem.get();
			printf("%s: %llu\n", sItemName.c_str(), u64ItemCount);
		}

	}


	return 0;



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
	
	//遍历mapItemCounter，转化为中文输出
	for (const auto &it : vecSortItem)
	{
		const auto &[sItemName, u64ItemCount] = it.get();

		if (bLanguage)
		{
			const auto it = zh_cn.find(sItemName);
			if (it != zh_cn.end() && it->is_string())
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
	*/
	
	return 0;
}

