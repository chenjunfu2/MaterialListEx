#include "..\MaterialListEx\NBT_Node.hpp"
#include "..\MaterialListEx\NBT_Reader.hpp"
#include "..\MaterialListEx\NBT_Writer.hpp"
#include "..\MaterialListEx\NBT_Helper.hpp"
#include "..\MaterialListEx\NBT_IO.hpp"
#include "..\MaterialListEx\CodeTimer.hpp"

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
private:
	int32_t i = 0;

public:
	MyTestValue(int32_t _i) : i(_i)
	{}

	~MyTestValue(void) = default;

	MyTestValue(MyTestValue &&_Move) noexcept :i(_Move.i)
	{
		_Move.i = INT32_MIN;
	}

	MyTestValue(const MyTestValue &_Copy) :i(_Copy.i)
	{}

	void Print(void) const noexcept
	{
		printf("%d ", i);
	}
};

class MyCpdValue
{
private:
	MyTestValue v;

public:
	/*
	//错误的做法，会导致意外移动
	template <typename T>
	MyCpdValue(T &&value) noexcept : v(std::move(value))
	{}
	
	template <typename T>
	MyCpdValue(const T &value) noexcept : v(value)
	{}
	*/

	//正确的做法，完美转发
	template <typename T>
	MyCpdValue(T &&value) noexcept : v(std::forward<T>(value))
	{}

	MyCpdValue(MyCpdValue &&_Move) noexcept : v(std::move(_Move.v))
	{}

	MyCpdValue(const MyCpdValue &_Copy) : v(_Copy.v)
	{}

	MyCpdValue(void) = default;
	~MyCpdValue(void) = default;

	MyTestValue &Get(void)
	{
		return v;
	}

	const MyTestValue &Get(void) const
	{
		return v;
	}
};


class MyTestClass
{
private:
	std::vector<MyCpdValue> v;

public:
	MyTestClass(void) = default;
	~MyTestClass(void) = default;

	MyTestClass(MyTestClass &&) = default;
	MyTestClass(const MyTestClass &) = default;

	template<typename T>
	void AddBack(T &&tVal)
	{
		v.emplace_back(std::forward<T>(tVal));
	}

	void Print(void) const noexcept
	{
		for (const auto &it : v)
		{
			it.Get().Print();
		}
	}
};

/*
MyTestValue t0(0), t1(1), t2(2), t3(3), t4(4);
MyTestValue &t3_ = t3;

MyTestClass c0;

c0.AddBack(std::move(t0));
c0.AddBack(t1);
c0.AddBack((const MyTestValue &)t2);
c0.AddBack(t3_);

t0.Print();
t1.Print();
t2.Print();
t3.Print();
t4.Print();

putchar('\n');
c0.Print();

putchar('\n');
return 0;
*/

/*
测试流程：
先读取原始nbt，解压后从nbt写出，然后再读入
最后用nbt分别解析这两个是否与原始等价（注意不是相同而是等价）
因为受到map排序的问题，nbt并非总是相同的，但是元素却是等价的
*/
//#define WRITE_TO_FILE

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("[Line:%d]Argc Not 2", __LINE__);
		return -114514;
	}

	const char *pInputFileName = argv[1];
	printf("Current File:\"%s\"\n", pInputFileName);

	CodeTimer ct{};
	printf("Test Start:\n");

	//从文件中读入
	std::basic_string<uint8_t> dataOriginal{};
	printf("\ndataOriginal ReadFile Start\n");
	ct.Start();
	if (!NBT_IO::ReadFile(pInputFileName, dataOriginal))
	{
		printf("[Line:%d]dataOriginal Read Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataOriginal ReadFile Time: ");

	printf("File Read Size: [%zu]B ≈ [%.4lf]KB ≈ [%.4lf]MB ≈ [%.4lf]GB\n",
		dataOriginal.size(),
		(double)dataOriginal.size() / 1024.0,
		(double)dataOriginal.size() / 1024.0 / 1024.0,
		(double)dataOriginal.size() / 1024.0 / 1024.0 / 1024.0);

	//如果已经压缩则解压
	printf("\ndataOriginal DecompressIfZipped Start\n");
	ct.Start();
	if (!NBT_IO::DecompressIfZipped(dataOriginal))
	{
		printf("[Line:%d]dataOriginal DecompressIfZipped Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataOriginal DecompressIfZipped Time: ");

	printf("File Decompress Size: [%zu]B ≈ [%.4lf]KB ≈ [%.4lf]MB ≈ [%.4lf]GB\n",
		dataOriginal.size(),
		(double)dataOriginal.size() / 1024.0,
		(double)dataOriginal.size() / 1024.0 / 1024.0,
		(double)dataOriginal.size() / 1024.0 / 1024.0 / 1024.0);

#ifdef WRITE_TO_FILE
	printf("\ndataOriginal WriteFile Start\n");
	ct.Start();
	if (!NBT_IO::WriteFile("read_dcp.nbt", dataOriginal))
	{
		printf("[Line:%d]dataOriginal WriteFile Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataOriginal WriteFile Time: ");
#endif // WRITE_TO_FILE

	//从已确保解压的二进制数据中解析到nbt
	printf("\ndataOriginal ReadNBT To cpdOriginal Start\n");
	ct.Start();
	NBT_Type::Compound cpdOriginal{};
	if (!NBT_Reader<std::basic_string_view<uint8_t>>::ReadNBT(cpdOriginal, dataOriginal))
	{
		printf("[Line:%d]dataOriginal ReadNBT To cpdOriginal Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataOriginal ReadNBT To cpdOriginal Time: ");

	//删除数据但不释放内存以便复用
	dataOriginal.resize(0);
	auto &dataNew = dataOriginal;//起个别名继续用，不删除内存减少重复分配开销
	printf("\ndataOriginal WriteNBT To dataNew Start\n");
	ct.Start();
	if (!NBT_Writer<std::basic_string<uint8_t>>::WriteNBT(dataNew, cpdOriginal))
	{
		printf("[Line:%d]dataOriginal WriteNBT To dataNew Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataOriginal WriteNBT To dataNew Time: ");

	printf("File WriteNBT Size: [%zu]B ≈ [%.4lf]KB ≈ [%.4lf]MB ≈ [%.4lf]GB\n",
		dataNew.size(),
		(double)dataNew.size() / 1024.0,
		(double)dataNew.size() / 1024.0 / 1024.0,
		(double)dataNew.size() / 1024.0 / 1024.0 / 1024.0);

#ifdef WRITE_TO_FILE
	printf("\ndataNew WriteFile Start\n");
	ct.Start();
	if (!NBT_IO::WriteFile("write_dcp.nbt", dataNew))
	{
		printf("[Line:%d]dataNew WriteFile Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataNew WriteFile Time: ");
#endif // WRITE_TO_FILE

	//写出到dataWrite之后，假装是文件读取的，直接用readnbt解析，然后比较之前的解析与后面的解析
	NBT_Type::Compound cpdNew{};
	printf("\ndataNew ReadNBT To cpdNew Start\n");
	ct.Start();
	if (!NBT_Reader<std::basic_string_view<uint8_t>>::ReadNBT(cpdNew, dataNew))
	{
		printf("[Line:%d]dataNew ReadNBT To cpdNew Fail\n", __LINE__);
		return -1;
	}
	ct.Stop();
	ct.PrintElapsed("dataNew ReadNBT To cpdNew Time: ");

	printf("\nFree Memory Start\n");
	ct.Start();
	dataNew.resize(0);
	dataNew.shrink_to_fit();//删除内存
	ct.Stop();
	ct.PrintElapsed("Free Memory Time: ");

	//递归比较
	printf("\nData Compare Start\n");
	ct.Start();
	std::partial_ordering cmp = cpdOriginal <=> cpdNew;
	ct.Stop();
	ct.PrintElapsed("Data Compare Time: ");
	//std::strong_ordering;//强序
	//std::weak_ordering;//弱序
	//std::partial_ordering;//偏序

	printf("\nComparison results :\ncpdOriginal [");

	if (cmp == std::partial_ordering::equivalent)
	{
		printf("equivalent");
	}
	else if (cmp == std::partial_ordering::greater)
	{
		printf("greater");
	}
	else if (cmp == std::partial_ordering::less)
	{
		printf("less");
	}
	else if (cmp == std::partial_ordering::unordered)
	{
		printf("unordered");
	}
	else
	{
		printf("unknown");
	}

	printf("] cpdNew\n");

	return 0;
}



int main0(int argc, char *argv[])
{
	std::basic_string<uint8_t> tR{};
	NBT_Type::Compound cpd;
	if (!NBT_IO::ReadFile("TestNbt.nbt", tR) ||
		!NBT_Reader<std::basic_string_view<uint8_t>>::ReadNBT(cpd, tR))
	{
		printf("TestNbt Read Fail!\nData before the error was encountered:\n");
	}
	else
	{
		printf("TestNbt Read Ok!\nData:\n");
	}
	NBT_Helper::Print(cpd);
	

	NBT_Node nn0{ NBT_Type::Compound{} }, nn1{ NBT_Type::List{} };

	NBT_Type::Compound &test = nn0.GetCompound();
	//NBT_Type::Compound &test = GetCompound(nn0);
	test.PutEnd(MU8STR("testend"), {});
	test.PutString(MU8STR("str0"), MU8STR("0000000"));
	test.PutByte(MU8STR("byte1"), 1);
	test.PutShort(MU8STR("short2"), 2);
	test.PutInt(MU8STR("int3"), 3);
	test.PutLong(MU8STR("long4"), 4);
	test.PutFloat(MU8STR("float5"), 5.0f);
	test.PutDouble(MU8STR("double6"), 6.0);

	//NBT_Type::List &list = nn1.GetList();
	NBT_Type::List & list = GetList(nn1);
	list.AddBack(NBT_Type::End{});
	list.AddBack((NBT_Type::Int)1);
	list.AddBack((NBT_Type::Int)9);
	list.AddBack((NBT_Type::Int)3);
	list.AddBack((NBT_Type::Int)5);
	list.AddBack((NBT_Type::Int)7);
	list.AddBack((NBT_Type::Int)0);
	list.AddBack(NBT_Type::End{});

	test.Put(MU8STR("list7"), std::move(list));

	test.PutCompound(MU8STR("compound8"), test);

	test.PutByteArray(MU8STR("byte array9"), { (NBT_Type::Byte)'a',(NBT_Type::Byte)'b',(NBT_Type::Byte)'c',(NBT_Type::Byte)255,(NBT_Type::Byte)0 });
	test.PutIntArray(MU8STR("int array10"), { 192,168,0,1,6666,555,99999999,2147483647,(NBT_Type::Int)2147483647 + 1 });
	test.PutLongArray(MU8STR("long array11"), { 0,0,0,0,-1,-1,-9,1'1451'4191'9810,233 });


	list.AddBack(std::move(test));
	list.AddBack(NBT_Type::Compound{ {MU8STR("yeee"),NBT_Type::String(MU8STR("eeeey"))} });

	test.PutList(MU8STR("root"), std::move(list));

	//putchar('\n');
	//PrintBool(test.Contains("test"));
	//putchar('\n');
	//PrintBool(test.Contains("test1"));
	//putchar('\n');
	//PrintBool(test.Contains("test", NBT_TAG::Int));
	//putchar('\n');
	//PrintBool(test.Contains("test2", NBT_TAG::Byte));
	//putchar('\n');

	printf("before write:\n");
	NBT_Helper::Print(list);
	NBT_Helper::Print(test);

	//NBT_IO::IsFileExist("TestNbt.nbt");
	std::basic_string<uint8_t> tData{};
	if (!NBT_Writer<std::basic_string<uint8_t>>::WriteNBT(tData, NBT_Type::Compound{ {MU8STR(""),std::move(test)} }) ||//构造一个空名称compound
		!NBT_IO::WriteFile("TestNbt.nbt", tData))
	{
		printf("write fail\n");
		return -1;
	}

	printf("write success\n");




	return 0;
	/*
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
	*/
}