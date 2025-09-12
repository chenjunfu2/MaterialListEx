#include "..\MaterialListEx\NBT_Node.hpp"
#include "..\MaterialListEx\NBT_Reader.hpp"
#include "..\MaterialListEx\NBT_Writer.hpp"
#include "..\MaterialListEx\NBT_Helper.hpp"
#include "..\MaterialListEx\File_Tool.hpp"
#include "..\MaterialListEx\Compression_Utils.hpp"

#include <stdio.h>
//#include <source_location>
#include <map>
#include <iostream>


//template <typename T>
//T func(T)
//{
//	auto c = std::source_location::current();
//	printf("%s %s [%d:%d]\n", c.file_name(), c.function_name(), c.line(), c.column());
//	return {};
//}

void PrintBool(bool b)
{
	printf("%s", b ? "true" : "false");
}

class MyTestValue
{
public:
	int32_t i = 0;

	void Print(void) const noexcept
	{
		printf("%d ", i);
	}
};


class MyTestClass
{
private:
	std::vector<MyTestValue> v;

public:
	MyTestClass(void) = default;
	~MyTestClass(void) = default;

	MyTestClass(MyTestClass &&) = default;
	MyTestClass(const MyTestClass &) = default;

	template<
	void Add()


	void Print(void) const noexcept
	{
		for (const auto it : v)
		{
			it.Print();
		}
	}
};






int main(int argc, char *argv[])
{
	//auto c = std::source_location::current();
	//
	//printf("%s %s [%d:%d]\n", c.file_name(), c.function_name(), c.line(), c.column());
	//
	//func<int>(1);
	//func<std::vector<int>>({});

	//*(long *)NULL = NULL;

	NBT_Node nn0{ NBT_Type::Compound{} }, nn1{ NBT_Type::List{} };

	NBT_Type::Compound &test = nn0.GetCompound();
	//NBT_Type::Compound &test = GetCompound(nn0);
	test.Put("test", 1LL);
	test.PutByte("test2", 1LL);
	test.PutInt("test3", 1LL);
	test.PutEnd("testend", {});


	//NBT_Type::List &list = nn1.GetList();
	NBT_Type::List &list = GetList(nn1);
	list.AddBack((NBT_Type::Int)1);
	list.AddBack((NBT_Type::Int)2);
	list.AddBack((NBT_Type::Int)3);
	list.AddBack(NBT_Type::End{});

	test.Put("list", list);

	NBT_Type::List l2(NBT_TAG::Compound);
	l2.AddBack(test);


	NBT_Helper::Print(l2);


	NBT_Helper::Print(list);
	putchar('\n');
	PrintBool(test.Contains("test"));
	putchar('\n');
	PrintBool(test.Contains("test1"));
	putchar('\n');
	PrintBool(test.Contains("test", NBT_TAG::Int));
	putchar('\n');
	PrintBool(test.Contains("test2", NBT_TAG::Byte));
	putchar('\n');

	NBT_Helper::Print(test);

	return 0;



	using mp = std::pair<const NBT_Type::String, NBT_Node>;

	NBT_Node 
	a
	{
		NBT_Type::Compound
		{
			mp{"2",NBT_Node{}},
			mp{"test0",NBT_Node{NBT_Type::String{"test1"}}},
			mp{"test1",NBT_Node{1}},
			mp{"test3",NBT_Node{}},
			mp{"test2",NBT_Node{66.6}},
			mp{"test4",NBT_Node{NBT_Type::List{NBT_Node(0),NBT_Node(1)}}},
			mp{"test5",NBT_Node
			{
				NBT_Type::Compound
				{
					mp{"1",NBT_Node{}},
					mp{"test0",NBT_Node{NBT_Type::String{"test2"}}},
					mp{"test1",NBT_Node{1}},
					mp{"test3",NBT_Node{1}},
					mp{"test2",NBT_Node{66.6}},
					mp{"test4",NBT_Node{NBT_Type::List{NBT_Node(0),NBT_Node(1)}}},
				}
			}}
		}
	},
	b
	{
		NBT_Type::Compound
		{
			mp{"2",NBT_Node{}},
			mp{"test0",NBT_Node{NBT_Type::String{"test1"}}},
			mp{"test1",NBT_Node{1}},
			mp{"test3",NBT_Node{}},
			mp{"test2",NBT_Node{66.6}},
			mp{"test4",NBT_Node{NBT_Type::List{NBT_Node(0),NBT_Node(1)}}},
			mp{"test5",NBT_Node
			{
				NBT_Type::Compound
				{
					mp{"1",NBT_Node{}},
					mp{"test0",NBT_Node{NBT_Type::String{"test2"}}},
					mp{"test1",NBT_Node{1}},
					mp{"test3",NBT_Node{2}},
					mp{"test2",NBT_Node{66.6}},
					mp{"test4",NBT_Node{NBT_Type::List{NBT_Node(0),NBT_Node(1)}}},
				}
			}}
		}
	};

	NBT_Helper::Print(a);
	NBT_Helper::Print(b);

	//std::partial_ordering tmp = a <=> b;

	NBT_Node_View<true> n0{ NBT_Type::Byte{0} }, n1{ NBT_Type::Byte{0} };

	auto tmp = n0 <=> n1;

	//std::strong_ordering;//强序
	//std::weak_ordering;//弱序
	//std::partial_ordering;//偏序

	printf("a <=> b : a [");

	if (tmp == std::partial_ordering::equivalent)
	{
		printf("equivalent");
	}
	else if (tmp == std::partial_ordering::greater)
	{
		printf("greater");
	}
	else if (tmp == std::partial_ordering::less)
	{
		printf("less");
	}
	else if (tmp == std::partial_ordering::unordered)
	{
		printf("unordered");
	}
	else
	{
		printf("unknown");
	}
	
	printf("] b\n");

	

	return 0;


	/*

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
		NBT_Type::Compound cpdTileEntity{};

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

	*/
	return 0;
}