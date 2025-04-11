//#include "MemoryLeakCheck.hpp"

#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "NBT_Helper.hpp"
#include "NBT_Reader.hpp"
#include "NBT_Writer.hpp"
#include "BlockProcess.hpp"

#include <nlohmann\json.hpp>
using Json = nlohmann::json;

#define ZLIB_CONST
#include <zlib.h>

#include <compress.hpp>
#include <config.hpp>
#include <decompress.hpp>
#include <utils.hpp>
#include <version.hpp>

#include <stdio.h>
#include <string>


bool ReadFile(const char *const _FileName, std::string &_Data)
{
	FILE *pFile = fopen(_FileName, "rb");
	if (pFile == NULL)
	{
		return false;
	}
	//获取文件大小
	if (_fseeki64(pFile, 0, SEEK_END) != 0)
	{
		return false;
	}
	uint64_t qwFileSize = _ftelli64(pFile);
	//回到文件开头
	rewind(pFile);

	//直接给数据塞string里
	_Data.resize(qwFileSize);//设置长度 c++23用resize_and_overwrite
	fread(_Data.data(), sizeof(_Data[0]), qwFileSize, pFile);//直接读入data
	//完成，关闭文件
	fclose(pFile);
	pFile = NULL;

	return true;
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
		return -1;
	}
	else
	{
		printf("Read Ok!\n");
	}

	//NBT_Helper::Print(nRoot);

	const auto &tmp = nRoot.GetCompound();//获取根下第一个compound，正常情况下根部下只有这一个compound
	if (tmp.size() != 1)
	{
		printf("Error root size");
		return -1;
	}

	//输出名称（一般是空字符串）
	const auto &root = *tmp.begin();
	printf("root:\"%s\"\n", ANSISTR(U16STR(root.first)).c_str());
	

	auto vtBlockStatistics = BlockProcess::GetBlockStatistics(root.second);
	//方块状态过滤（过滤掉部分不需要统计的方块，比如床的半边，门的半边，不是水源的水，不是岩浆源的岩浆，洞穴空气 虚空空气 空气，等等等等）

	//方块状态转换（把所有需要转换到目标的方块状态进行转换，比如不同类型花的花盆）
	std::map<NBT_Node::NBT_String, uint64_t> mapItemCounter;//创建方块状态到物品映射map

	for (const auto &[sRegionName, bsList] : vtBlockStatistics)
	{
		//处理sRegionName
		//auto OptStr = ANSISTR(U16STR(sRegionName));

		//处理方块
		for (const auto &itBlock : bsList)
		{
			auto istItemList = BlockProcess::BlockStatisticsToItemStack(itBlock);
			for (const auto &itItem : istItemList)
			{
				mapItemCounter[itItem.sItemName] += itItem.u64Counter;//如果key不存在，则自动创建，且保证value为0
			}
		}
	}

	//读取json语言文件
	std::string sJsonData;
	if (!ReadFile("zh_cn__.json", sJsonData))
	{
		printf("Json File Read Fail\n");
		return -1;
	}


	Json zh_cn;

	try
	{
		zh_cn = Json::parse(sJsonData);
	}
	catch (const Json::parse_error &e)
	{
		// 输出异常详细信息
		printf("JSON parse error: %s\nError Pos: [%zu]\n", e.what(), e.byte);
		return -1;
	}
	

	//遍历mapItemCounter，转化为中文输出
	for (const auto &[sItemName, u64ItemCount] : mapItemCounter)
	{
		const auto it = zh_cn.find(sItemName);
		if (it != zh_cn.end() && it->is_string())
		{
			printf("%s [%s]:%llu\n", ConvertUtf8ToAnsi(it->get<std::string>()).c_str(), sItemName.c_str(), u64ItemCount);
		}
		else
		{
			printf("[%s]:%llu\n", sItemName.c_str(), u64ItemCount);
		}
	}

	
	return 0;
}

