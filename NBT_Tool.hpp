#pragma once

#include "NBT_Node.hpp"

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
		Compound_End = 1,//结束
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
		 szCurrent += wNameLength;//移动下标
		 return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//读取数据
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//浮点数特判
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//无损数据类型转换
			}
			else
			{
				//无名，为列表元素，直接修改nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) };
			}
			
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//浮点数特判
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//无损数据类型转换
			}
			else
			{
				//无名，为列表元素，直接修改nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) };
			}
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ tTmpData });
			}
			else
			{
				nRoot = NBT_Node{ tTmpData };
			}
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

	template<typename T, bool bHasName = true>
	static int GetArrayType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
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
			tArray.emplace_back(tTmpData);//读取一个插入一个
		}
		
		if constexpr (bHasName)
		{
			//完成后插入
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::move(tArray) });
		}
		else//无名称，为列表元素
		{
			nRoot = NBT_Node{ std::move(tArray) };
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//开始递归
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(data, szCurrent, nodeTemp);
		if (iRet < AllOk)
		{
			return iRet;
		}

		if constexpr (bHasName)
		{
			//递归完成，所有子节点已到位
			//取出NBT_Compound挂到自己根部（移动）
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) });
		}
		else//无名称，为列表元素
		{
			nRoot = NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static int GetStringType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
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

		if constexpr (bHasName)
		{
			//原位构造
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} });
		}
		else//列表元素直接赋值
		{
			nRoot = NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} };
		}
		szCurrent += wStrLength;//移动下标
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
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
		NBT_Node::NBT_List tmpList;
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//列表元素会直接赋值修改
			int iRet = SwitchNBT<false>(data, szCurrent, tmpNode, (NBT_Node::NBT_TAG)bListElementType);

			if (iRet != AllOk)
			{
				return iRet;
			}

			//每读取一个往后插入一个
			tmpList.emplace_back(std::move(tmpNode));
		}

		//列表可嵌套，所以处理本身嵌套无名情况
		if constexpr (bHasName)
		{
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, NBT_Node{ std::move(tmpList) });
		}
		else//列表中的列表，直接赋值，而不进行插入
		{
			nRoot = NBT_Node{ std::move(tmpList) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static inline int SwitchNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot, NBT_Node::NBT_TAG tag)
	{
		if (szCurrent >= data.size() && tag != NBT_Node::TAG_End)
		{
			return OutOfRange;
		}

		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				iRet = Compound_End;
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{//最复杂
				iRet = GetListType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Compound://需要递归调用
			{
				iRet = GetCompoundType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(data, szCurrent, nRoot);
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
			iRet = SwitchNBT(data, szCurrent, nRoot, (NBT_Node::NBT_TAG)(uint8_t)data[szCurrent++]);
		} while (iRet == AllOk);
		
		return iRet;
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
		return GetNBT(data, szCurrent, nRoot) == Compound_End;
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}

	void Print(void) const
	{
		PrintSwitch(nRoot, 0);
	}

private:
	void PrintSwitch(const NBT_Node &nRoot, int iLevel) const
	{
		switch (nRoot.GetTag())
		{
		case NBT_Node::TAG_End:
			{
				printf("[Compound End]");
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				printf("%db", nRoot.GetData<NBT_Node::NBT_Byte>());
			}
			break;
		case NBT_Node::TAG_Short:
			{
				printf("%ds", nRoot.GetData<NBT_Node::NBT_Short>());
			}
			break;
		case NBT_Node::TAG_Int:
			{
				printf("%d", nRoot.GetData<NBT_Node::NBT_Int>());
			}
			break;
		case NBT_Node::TAG_Long:
			{
				printf("%lldl", nRoot.GetData<NBT_Node::NBT_Long>());
			}
			break;
		case NBT_Node::TAG_Float:
			{
				printf("%ff", nRoot.GetData<NBT_Node::NBT_Float>());
			}
			break;
		case NBT_Node::TAG_Double:
			{
				printf("%lff", nRoot.GetData<NBT_Node::NBT_Double>());
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Byte_Array>();
				printf("[B;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}
				if (arr.size() != 0)
				{
					printf("\b");
				}

				printf("]");
			}
			break;
		case NBT_Node::TAG_String:
			{
				printf("\"%s\"", nRoot.GetData<NBT_Node::NBT_String>().c_str());
			}
			break;
		case NBT_Node::TAG_List:
			{
				auto &list = nRoot.GetData<NBT_Node::NBT_List>();
				printf("[");
				for (auto &it : list)
				{
					PrintSwitch(it, ++iLevel);
					printf(",");
				}

				if (list.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound:
			{
				auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
				printf("{");

				for (auto &it : cpd)
				{
					printf("\"%s\":", it.first.c_str());
					PrintSwitch(it.second, ++iLevel);
					printf(",");
				}

				if (cpd.size() != 0)
				{
					printf("\b");
				}
				printf("}");
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Int_Array>();
				printf("[I;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Long_Array>();
				printf("[L;");
				for (auto &it : arr)
				{
					printf("%lld,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		default:
			{
				printf("[Unknow Type]");
			}
			break;
		}
	}
};