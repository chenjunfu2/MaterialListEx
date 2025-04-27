#include "..\MaterialListEx\NBT_Node.hpp"
#include "..\MaterialListEx\NBT_Reader.hpp"
#include "..\MaterialListEx\NBT_Writer.hpp"
#include "..\MaterialListEx\NBT_Helper.hpp"
#include "..\MaterialListEx\MUTF8_Tool.hpp"
#include "..\MaterialListEx\Windows_ANSI.hpp"
#include "..\MaterialListEx\File_Tool.hpp"
#include "..\MaterialListEx\Compression_Utils.hpp"

#include <stdio.h>
//#include <source_location>
#include <map>


//template <typename T>
//T func(T)
//{
//	auto c = std::source_location::current();
//	printf("%s %s [%d:%d]\n", c.file_name(), c.function_name(), c.line(), c.column());
//	return {};
//}



int main(int argc, char *argv[])
{
	//auto c = std::source_location::current();
	//
	//printf("%s %s [%d:%d]\n", c.file_name(), c.function_name(), c.line(), c.column());
	//
	//func<int>(1);
	//func<std::vector<int>>({});


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
	const auto &cpRegions = GetCompound(root.second).GetCompound(MU8STR("Regions"));
	for (const auto &[RgName, RgCompound] : cpRegions)//遍历选区
	{
		//获取方块实体列表
		const auto &listTileEntity = GetCompound(RgCompound).GetList(MU8STR("TileEntities"));

		//创建map，同名方块实体只保留第一个
		NBT_Node::NBT_Compound cpdTileEntity{};

		for (const auto &it : listTileEntity)
		{
			cpdTileEntity.try_emplace(GetCompound(it).GetString(MU8STR("id")), it);
		}

		for (const auto &it : cpdTileEntity)
		{
			printf("give @s %s\n", it.first.c_str());
		}
		//printf("Region:[%s]\n", ANSISTR(U16STR(RgName)).c_str());
		//NBT_Helper::Print(NBT_Node{ std::move(cpdTileEntity) });
	}

	
	return 0;
}