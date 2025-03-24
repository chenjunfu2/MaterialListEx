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

	enum ErrCode : int
	{
		Compound_End = 1,//结束
		AllOk = 0,//没有问题
		InternalTypeError = -1,//变体NBT节点类型错误（代码问题）
		OutOfRangeError = -2,//（NBT内部长度错误溢出）（NBT文件问题）
		NbtTypeTagError = -3,//NBT标签类型错误（NBT文件问题）
	};

	static inline const char *const errReason[] =
	{
		"AllOk",
		"InternalTypeError",
		"OutOfRangeError",
		"NbtTypeTagError",
	};

	enum WarnCode : int
	{
		NoWarn = 0,
		ElementExistsWarn = 1,
	};

	static inline const char *const warnReason[] =
	{
		"NoWarn",
		"ElementExistsWarn",
	};

	//使用变参形参表+vprintf代理复杂输出，给出更多扩展信息
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(T code, const std::string &data, const size_t &szCurrent, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)//gcc使用__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//上方if保证errc为负，此处反转访问保证无问题（除非代码传入异常错误码）
			printf("Read Err[%d]: \"%s\"\n", code, errReason[-(int)code]);
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			if (code >= NoWarn)
			{
				return (int)code;
			}
			//输出warn错误
			printf("Read Warn[%d]: \"%s\"\n", code, warnReason[(int)code]);
		}
		else
		{
			static_assert(false, "Unknow [T code] Type!");
		}

		if (cpExtraInfo != NULL)
		{
			printf("Extra Info:\"");
			va_list args;//变长形参
			va_start(args, cpExtraInfo);
			vprintf(cpExtraInfo, args);
			va_end(args);
			printf("\"\n");
		}

		//如果可以，预览szCurrent前后n个字符，否则裁切到边界
#define VIEW_PRE 32//向前
#define VIEW_SUF (32 + 8)//向后

		size_t rangeBeg = (szCurrent > VIEW_PRE) ? (szCurrent - VIEW_PRE) : 0;//上边界裁切
		size_t rangeEnd = ((szCurrent + VIEW_SUF) < data.size()) ? (szCurrent + VIEW_SUF) : data.size();//下边界裁切
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)szCurrent, szCurrent, (uint64_t)data.size(), data.size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd, rangeEnd);
		
		for (size_t i = rangeBeg; i < rangeEnd; ++i)
		{
			if ((i - rangeBeg) % 8 == 0)//输出地址
			{
				if (i != rangeBeg)//除去第一个每8个换行
				{
					printf("\n");
				}
				printf("0x%02llX: ", (uint64_t)i);
			}

			if (i != szCurrent)
			{
				printf(" %02X ", (uint8_t)data[i]);
			}
			else//如果是当前出错字节，加方括号框起
			{
				printf("[%02X]", (uint8_t)data[i]);
			}
		}

		if constexpr (std::is_same<T, ErrCode>::value)
		{
			printf("\nSkip err data and clear!\n");
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			printf("\nSkip warn data and continue...\n");
		}
		else
		{
			static_assert(false, "Unknow [T code] Type!");
		}

		return (int)code;
	}


	static int GetName(const std::string &data, size_t &szCurrent, std::string &sName)
	{
		//读取2字节的无符号名称长度
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wNameLength))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": wNameLength Read");
		}

		//判断长度是否超过
		if (szCurrent + wNameLength >= data.size())
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + wNameLength[%zu] [%zu]>= data.size()[%zu]",
				szCurrent, wNameLength, szCurrent + (size_t)wNameLength, data.size());
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
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//无损数据类型转换
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Float).name());
				}
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
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//无损数据类型转换
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Double).name());
				}
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
				return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ tTmpData });
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tTmpData).name());
				}
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
		//判断是不是vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//抛出编译错误
		}

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
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": dwElementCount Read");
		}

		//判断长度是否超过
		if (szCurrent + dwElementCount * sizeof(T::value_type) >= data.size())//保证下方调用安全
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu] [%zu]>= data.size()[%zu]", 
				szCurrent, (size_t)dwElementCount, sizeof(T::value_type), szCurrent + (size_t)dwElementCount * sizeof(T::value_type), data.size());
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
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::move(tArray) });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tArray).name());
			}
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
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(sName, NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(NBT_Node::NBT_Compound).name());
			}
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
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": wStrLength Read");
		}

		//判断长度是否超过
		if (szCurrent + wStrLength >= data.size())
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] + wStrLength[%zu] [%zu]>= data.size()[%zu]",
				szCurrent, (size_t)wStrLength, szCurrent + (size_t)wStrLength, data.size());
		}

		if constexpr (bHasName)
		{
			//原位构造
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(std::string).name());
			}
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
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": bListElementType Read");
		}


		//读取4字节的有符号列表长度
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(data, szCurrent, dwListLength))
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": dwListLength Read");
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
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(sName, NBT_Node{ std::move(tmpList) });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, data, szCurrent, __FUNCSIG__ ": the \"%s\"[%s] data already exist!", sName, typeid(tmpList).name());
			}
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
		if (szCurrent >= data.size() && tag != NBT_Node::TAG_End)//如果tag当前就是结尾，则直接下去处理结尾返回
		{
			return Error(OutOfRangeError, data, szCurrent, __FUNCSIG__ ": szCurrent[%zu] >= data.size()[%zu]", szCurrent, data.size());
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
		default://NBT内标数据签错误
			{
				iRet = Error(NbtTypeTagError, data, szCurrent, __FUNCSIG__ ": NBT Tag switch default: Unknow Type Tag[%02X(%d)]", tag, tag);//此处不进行提前返回，往后默认返回处理
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
			return Error(InternalTypeError, data, szCurrent, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
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
		SetNBT(data);
	}
	~NBT_Tool(void) = default;

	bool SetNBT(const std::string &data)//设置nbt到类内
	{//对于用户来说是设置给类
		nRoot.Clear();//清掉原来的数据（注意如果nbt较大的情况下，这是一个较深的递归清理过程，不排除栈空间不足导致清理失败）
		size_t szCurrent{ 0 };
		return GetNBT(data, szCurrent, nRoot) == Compound_End;//对于类来说是从用户给的data获得（get）nbt数据
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
		auto tag = nRoot.GetTag();
		switch (tag)
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
				printf("[Unknow NBT Tag Type [%02X(%d)]]", tag, tag);
			}
			break;
		}
	}
};