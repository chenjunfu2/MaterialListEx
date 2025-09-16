#include "..\MaterialListEx\NBT_Node.hpp"
#include "..\MaterialListEx\NBT_Reader.hpp"
#include "..\MaterialListEx\NBT_Writer.hpp"
#include "..\MaterialListEx\NBT_Helper.hpp"
#include "..\MaterialListEx\NBT_IO.hpp"

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




int main(int argc, char *argv[])
{
	std::basic_string<uint8_t> tR{};
	NBT_IO::ReadFile("TestNbt.nbt", tR);

	NBT_Type::Compound cpd;
	NBT_Reader<std::basic_string<uint8_t>>::ReadNBT(cpd, tR);
	NBT_Helper::Print(cpd);

	NBT_Node nn0{ NBT_Type::Compound{} }, nn1{ NBT_Type::List{} };

	NBT_Type::Compound &test = nn0.GetCompound();
	//NBT_Type::Compound &test = GetCompound(nn0);
	test.Put("test", 1LL);
	test.PutByte("test2", 1LL);
	test.PutInt("test3", 1LL);
	//test.PutEnd("testend", {});


	//NBT_Type::List &list = nn1.GetList();
	NBT_Type::List &list = GetList(nn1);
	list.AddBack(NBT_Type::End{});
	list.AddBack((NBT_Type::Int)1);
	list.AddBack((NBT_Type::Int)2);
	list.AddBack((NBT_Type::Int)3);
	list.AddBack(NBT_Type::End{});

	test.Put("list", list);
	NBT_Helper::Print(list);
	
	//putchar('\n');
	//PrintBool(test.Contains("test"));
	//putchar('\n');
	//PrintBool(test.Contains("test1"));
	//putchar('\n');
	//PrintBool(test.Contains("test", NBT_TAG::Int));
	//putchar('\n');
	//PrintBool(test.Contains("test2", NBT_TAG::Byte));
	//putchar('\n');

	NBT_Helper::Print(test);

	//NBT_IO::IsFileExist("TestNbt.nbt");
	std::basic_string<uint8_t> tData{};
	if (!NBT_Writer<std::basic_string<uint8_t>>::WriteNBT(tData, { {"",std::move(test)} }) ||
		!NBT_IO::WriteFile("TestNbt.nbt", tData))
	{
		printf("write fail\n");
		return -1;
	}

	printf("write success\n");

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


	return 0;
}