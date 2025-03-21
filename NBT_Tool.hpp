#pragma once
#include <stdint.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <variant>

#include <type_traits>

class NBT_Tool;//前向声明

class NBT_Node
{
public:
	enum NBT_TAG :uint8_t
	{
		TAG_End = 0,	//结束项
		TAG_Byte,		//int8_t
		TAG_Short,		//int16_t
		TAG_Int,		//int32_t
		TAG_Long,		//int64_t
		TAG_Float,		//float 4byte
		TAG_Double,		//double 8byte
		TAG_Byte_Array,	//std::vector<int8_t>
		TAG_String,		//std::string->有长度数据，且为非0终止字符串!!
		TAG_List,		//std::list<NBT_Node>
		TAG_Compound,	//std::map<std::string, NBT_Node>->字符串为NBT项名称
		TAG_Int_Array,	//std::vector<int32_t>
		TAG_Long_Array,	//std::vector<int64_t>
	};

	using NBT_End			= std::monostate;//无状态
	using NBT_Byte			= int8_t;
	using NBT_Short			= int16_t;
	using NBT_Int			= int32_t;
	using NBT_Long			= int64_t;
	using NBT_Float			= std::conditional_t<(sizeof(float) == sizeof(uint32_t)), float, uint32_t>;//通过编译期确认类型大小来选择正确的类型，优先浮点类型，如果失败则替换为对应的可用类型
	using NBT_Double		= std::conditional_t<(sizeof(double) == sizeof(uint64_t)), double, uint64_t>;
	using NBT_Byte_Array	= std::vector<int8_t>;
	using NBT_Int_Array		= std::vector<int32_t>;
	using NBT_Long_Array	= std::vector<int64_t>;
	using NBT_String		= std::string;
	using NBT_List			= std::list<NBT_Node>;//存储一系列同类型标签的有效负载（无标签 ID 或名称）
	using NBT_Compound		= std::map<std::string, NBT_Node>;//挂在序列下的内容都通过map绑定名称

	template<typename... Ts> struct TypeList
	{};
	using NBT_TypeList = TypeList
	<
		NBT_End,
		NBT_Byte,
		NBT_Short,
		NBT_Int,
		NBT_Long,
		NBT_Float,
		NBT_Double,
		NBT_Byte_Array,
		NBT_String,
		NBT_List,
		NBT_Compound,
		NBT_Int_Array,
		NBT_Long_Array
	>;

	template <typename T>
	struct TypeListToVariant;

	template <typename... Ts>
	struct TypeListToVariant<TypeList<Ts...>>
	{
		using type = std::variant<Ts...>;
	};

	using VariantData = TypeListToVariant<NBT_TypeList>::type;
private:
	NBT_TAG tag;
	VariantData data;


	// 类型索引计算
	template <typename T, typename List>
	struct TypeIndex;

	template <typename T, typename... Ts>
	struct TypeIndex<T, TypeList<T, Ts...>>
	{
		static constexpr size_t value = 0;
	};

	template <typename T, typename U, typename... Ts>
	struct TypeIndex<T, TypeList<U, Ts...>>
	{
		static constexpr size_t value = 1 + TypeIndex<T, TypeList<Ts...>>::value;
	};

	// 类型存在检查
	template <typename T, typename List>
	struct IsValidType;

	template <typename T, typename... Ts>
	struct IsValidType<T, TypeList<Ts...>>
	{
		static constexpr bool value = (std::is_same_v<T, Ts> || ...);
	};


	// 自动推导标签类型
	template<typename T>
	static constexpr NBT_TAG deduce_tag()
	{
		static_assert(IsValidType<std::decay_t<T>, NBT_TypeList>::value, "Invalid type for NBT node");
		return static_cast<NBT_TAG>(TypeIndex<std::decay_t<T>, NBT_TypeList>::value);
	}
public:
	// 通用构造函数
	template<typename T>
	explicit NBT_Node(T &&value)
		: tag(deduce_tag<T>()),
		data(std::forward<T>(value))
	{
		static_assert(!std::is_same_v<std::decay_t<T>, NBT_Node>, "Cannot construct NBT_Node from another NBT_Node");
	}

	// 默认构造（TAG_End）
	NBT_Node() : tag(TAG_End), data(std::monostate{})
	{}

	// 自动析构由variant处理
	~NBT_Node() = default;
	//移动构造由variant处理
	NBT_Node(NBT_Node &&) = default;
	//拷贝构造由variant处理
	NBT_Node(const NBT_Node &) = default;

	//清除所有数据
	void Clear(void)
	{
		tag = TAG_Compound;
		data = NBT_Compound{};
	}

	// 获取标签类型
	NBT_TAG GetTag() const noexcept
	{
		return tag;
	}

	// 获取当前存储类型的index
	size_t VariantIndex() const noexcept
	{
		return data.index();
	}

	// 类型安全访问
	template<typename T>
	const T &GetData() const
	{
		return std::get<T>(data);
	}

	template<typename T>
	T &GetData()
	{
		return std::get<T>(data);
	}

	// 类型检查
	template<typename T>
	bool TypeHolds() const
	{
		return std::holds_alternative<T>(data);
	}
};


class NBT_Tool
{
private:
	NBT_Node nRoot{ NBT_Node::NBT_Compound{} };//默认空节点

	// 读取大端序数值
	template<typename T>
	static bool ReadBigEndian(const std::string &data, size_t &szCurrent, T& tVal)
	{
		if (szCurrent + sizeof(T) >= data.size())
		{
			return false;
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//因为只会左移，不存在有符号导致的算术位移bug，不用转换为无符号类型
			}
			tVal = tTmp;
		}

		return true;
	}

	// 读取大端序数值（快速版）（调用者需要确保data范围安全）
	template<typename T>
	static inline void FastReadBigEndian(const std::string &data, size_t &szCurrent, T &tVal)
	{
		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//因为只会左移，不存在有符号导致的算术位移bug，不用转换为无符号类型
			}
			tVal = tTmp;
		}
	}

	enum ErrCode
	{
		AllOk = 0,//没有问题
		OutOfRange = -1,
		TypeError = -2,
		WhatTheFuck = -1145,//几乎不可能的错误
	};


	static int GetName(const std::string &data, size_t &szCurrent, std::string &sName)
	{
		//读取2字节的无符号名称长度
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wNameLength))
		{
			return OutOfRange;
		}

		//判断长度是否超过
		if (szCurrent + wNameLength >= data.size())
		{
			return OutOfRange;
		}

		//解析出名称
		 sName = { data.begin() + szCurrent, data.begin() + (szCurrent + wNameLength) };//如果长度为0则构造0长字符串，合法行为
		 return AllOk;
	}


	template<typename T>
	static int GetbuiltInType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//读取数据
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//浮点数特判
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//名称-内含数据的节点插入当前调用栈深度的根节点
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float*)&tTmpData)) });//无损数据类型转换
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//浮点数特判
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//名称-内含数据的节点插入当前调用栈深度的根节点
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//无损数据类型转换
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			//名称-内含数据的节点插入当前调用栈深度的根节点
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ tTmpData });
		}
		else
		{
			static_assert(false, "Not a legal type call!");//抛出编译错误
		}

		return AllOk;
	}

	template<typename T>
	struct is_std_vector : std::false_type
	{};

	template<typename T, typename Alloc>
	struct is_std_vector<std::vector<T, Alloc>> : std::true_type
	{};

	template<typename T>
	static int GetArrayType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//获取4字节有符号数，代表数组元素个数
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(data, szCurrent, dwElementCount))
		{
			return OutOfRange;
		}

		//判断长度是否超过
		if (szCurrent + dwElementCount * sizeof(T) >= data.size())//保证下方调用安全
		{
			return OutOfRange;
		}

		//判断是不是vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//抛出编译错误
		}
		
		//数组保存
		T tArray;
		tArray.reserve(dwElementCount);//提前扩容

		//读取dElementCount个元素
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			FastReadBigEndian(data, szCurrent, tTmpData);//调用需要确保范围安全
			tArray.push_back(tTmpData);//读取一个插入一个
		}
		
		//完成后插入
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::move(tArray) });
		return AllOk;
	}

	static int GetCompoundType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//开始递归
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		iRet = GetNBT(data, szCurrent, nodeTemp);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//递归完成，所有子节点已到位
		//取出NBT_Compound挂到自己根部（移动）
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()));

		return AllOk;
	}

	static int GetStringType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//读取2字节的无符号名称长度
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wStrLength))
		{
			return OutOfRange;
		}

		//判断长度是否超过
		if (szCurrent + wStrLength >= data.size())
		{
			return OutOfRange;
		}

		//原位构造
		nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent,data.begin() + (szCurrent + wStrLength)} });
		return AllOk;
	}

	static int GetListType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		int iRet = GetName(data, szCurrent, sName);
		if (iRet < AllOk)
		{
			return iRet;
		}

		//读取1字节的列表元素类型
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(data, szCurrent, bListElementType))
		{
			return OutOfRange;
		}


		//读取4字节的有符号列表长度
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(data, szCurrent, dwListLength))
		{
			return OutOfRange;
		}

		//根据元素类型，读取n次列表
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			int iRet = SwitchNBT(data, szCurrent, nRoot, bListElementType);
			if(iRet < AllOk)
			{
				return iRet;
			}



		}
	}


	static inline int SwitchNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot, NBT_Node::NBT_TAG tag)
	{
		if (szCurrent >= data.size())
		{
			return OutOfRange;
		}

		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				return AllOk;
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{//最复杂

			}
			break;
		case NBT_Node::TAG_Compound://需要递归调用
			{
				iRet = GetCompoundType(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array>(data, szCurrent, nRoot);
			}
			break;
		default:
			{
				iRet = WhatTheFuck;//这错误怎么出现的？
			}
			break;
		}

		return iRet;
	}

	static int GetNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)//递归调用读取并添加节点
	{
		//节点类型检查：保证当前nRoot是NBT_Node::NBT_Compound类型，否则失败
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//类型错误
		{
			return TypeError;
		}

		int iRet;
		do
		{
			iRet = SwitchNBT(data, szCurrent, nRoot, data[szCurrent++]);
		} while (iRet == AllOk);
		
	}
public:
	NBT_Tool(void) = default;
	NBT_Tool(const std::string &data)
	{
		GetNBT(data);
	}
	~NBT_Tool(void) = default;

	bool GetNBT(const std::string &data)
	{
		nRoot.Clear();//清掉数据
		size_t szCurrent{ 0 };
		GetNBT(data, szCurrent, nRoot);
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}
};