#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

class InputStream
{
private:
	const std::string &sString;
	size_t szIndex;
public:
	InputStream(const std::string &_sString, size_t szStartIdx = 0) :sString(_sString), szIndex(szStartIdx)
	{}
	~InputStream() = default;

	char GetChar()
	{
		return sString[szIndex++];
	}

	void UnGet()
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const char &operator[](size_t szIndex) const
	{
		return sString[szIndex];
	}

	std::string::const_iterator Current() const
	{
		return sString.begin() + szIndex;
	}

	std::string::const_iterator Next(size_t szSize) const
	{
		return sString.begin() + (szIndex + szSize);
	}

	size_t AddIndex(size_t szSize)
	{
		return szIndex += szSize;
	}

	size_t SubIndex(size_t szSize)
	{
		return szIndex -= szSize;
	}

	bool IsEnd() const
	{
		return szIndex >= sString.size();
	}

	size_t Size() const
	{
		return sString.size();
	}

	bool HasAvailData(size_t szSize) const
	{
		return (sString.size() - szIndex) >= szSize;
	}

	void Reset()
	{
		szIndex = 0;
	}

	const std::string &Data() const
	{
		return sString;
	}

	size_t Index() const
	{
		return szIndex;
	}

	size_t &Index()
	{
		return szIndex;
	}
};


class NBT_Reader
{
private:
	// 读取大端序数值，bNoCheck为true则不进行任何检查
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, bool> ReadBigEndian(InputStream &sData, T& tVal)
	{
		if constexpr (!bNoCheck)
		{
			if (!sData.HasAvailData(sizeof(T)))
			{
				return false;
			}
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)sData.GetChar();
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)sData.GetChar();//因为只会左移，不存在有符号导致的算术位移bug，不用转换为无符号类型
			}
			tVal = tTmp;
		}

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	enum ErrCode : int
	{
		Compound_End = 1,//结束
		AllOk = 0,//没有问题
		InternalTypeError = -1,//变体NBT节点类型错误（代码问题）
		OutOfRangeError = -2,//（NBT内部长度错误溢出）（NBT文件问题）
		NbtTypeTagError = -3,//NBT标签类型错误（NBT文件问题）
		StackDepthExceeded = -4,//调用栈深度过深（NBT文件or代码设置问题）
		ERRCODE_END = -5,//结束标记，统计负数部分大小
	};

	static inline const char *const errReason[] =
	{
		"AllOk",
		"InternalTypeError",
		"OutOfRangeError",
		"NbtTypeTagError",
		"StackDepthExceeded",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

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
	static int _cdecl Error(T code, const InputStream &sData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)//gcc使用__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//上方if保证errc为负，此处反转访问保证无问题（除非代码传入异常错误码）
			printf("Read Err[%d]: \"%s\"\n", (int)code, errReason[-(int)code]);
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			if (code >= NoWarn)
			{
				return (int)code;
			}
			//输出warn警告
			printf("Read Warn[%d]: \"%s\"\n", (int)code, warnReason[(int)code]);
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

		size_t rangeBeg = (sData.Index() > VIEW_PRE) ? (sData.Index() - VIEW_PRE) : 0;//上边界裁切
		size_t rangeEnd = ((sData.Index() + VIEW_SUF) < sData.Size()) ? (sData.Index() + VIEW_SUF) : sData.Size();//下边界裁切
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)sData.Index(), sData.Index(), (uint64_t)sData.Size(), sData.Size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd - 1, rangeEnd - 1);
		
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

			if (i != sData.Index())
			{
				printf(" %02X ", (uint8_t)sData[i]);
			}
			else//如果是当前出错字节，加方括号框起
			{
				printf("[%02X]", (uint8_t)sData[i]);
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

#define _RP___FUNCSIG__ __FUNCSIG__//用于编译过程二次替换达到函数内部

#define CHECK_STACK_DEPTH(Depth) \
{\
	if((Depth) <= 0)\
	{\
		return Error(StackDepthExceeded, sData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
	}\
}

	static int GetName(InputStream &sData, std::string &sName)
	{
		//读取2字节的无符号名称长度
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(sData, wNameLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": wNameLength Read");
		}

		//判断长度是否超过
		if (!sData.HasAvailData(wNameLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + wNameLength[%zu])[%zu] > DataSize[%zu]",
				sData.Index(), (size_t)wNameLength, sData.Index() + (size_t)wNameLength, sData.Size());
		}

		//解析出名称
		sName = { sData.Current(), sData.Next(wNameLength) };//如果长度为0则构造0长字符串，合法行为
		sData.AddIndex(wNameLength);//移动下标
		return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(InputStream &sData, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//读取数据
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//浮点数特判
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//无损数据类型转换
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Float).name());
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
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//无损数据类型转换
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Double).name());
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
			if (!ReadBigEndian(sData, tTmpData))
			{
				return Error(OutOfRangeError, sData, __FUNCSIG__ ": tTmpData Read");
			}

			if constexpr (bHasName)
			{
				//名称-内含数据的节点插入当前调用栈深度的根节点
				auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ tTmpData });
				if (!ret.second)//插入失败，元素已存在
				{
					Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tTmpData).name());
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
	static int GetArrayType(InputStream &sData, NBT_Node &nRoot)
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
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//获取4字节有符号数，代表数组元素个数
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(sData, dwElementCount))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": dwElementCount Read");
		}

		//判断长度是否超过
		if (!sData.HasAvailData(dwElementCount * sizeof(T::value_type)))//保证下方调用安全
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				sData.Index(), (size_t)dwElementCount, sizeof(T::value_type), sData.Index() + (size_t)dwElementCount * sizeof(T::value_type), sData.Size());
		}
		
		//数组保存
		T tArray;
		tArray.reserve(dwElementCount);//提前扩容

		//读取dElementCount个元素
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(sData, tTmpData);//调用需要确保范围安全
			tArray.emplace_back(tTmpData);//读取一个插入一个
		}
		
		if constexpr (bHasName)
		{
			//完成后插入
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::move(tArray) });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tArray).name());
			}
		}
		else//无名称，为列表元素
		{
			nRoot = NBT_Node{ std::move(tArray) };
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)//此递归函数需要保证传递返回值
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//开始递归
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(sData, nodeTemp, szStackDepth);//此处直接传递，因为GetNBT内部会进行递减
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
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Compound).name());
			}
		}
		else//无名称，为列表元素
		{
			nRoot = NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static int GetStringType(InputStream &sData, NBT_Node &nRoot)
	{
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//读取2字节的无符号名称长度
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(sData, wStrLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": wStrLength Read");
		}

		//判断长度是否超过
		if (!sData.HasAvailData(wStrLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": (Index[%zu] + wStrLength[%zu])[%zu] > DataSize[%zu]",
				sData.Index(), (size_t)wStrLength, sData.Index() + (size_t)wStrLength, sData.Size());
		}

		if constexpr (bHasName)
		{
			//原位构造
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), NBT_Node{ std::string{sData.Current(), sData.Next(wStrLength)} });
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(std::string).name());
			}
		}
		else//列表元素直接赋值
		{
			nRoot = NBT_Node{ std::string{sData.Current(), sData.Next(wStrLength)} };
		}
		sData.AddIndex(wStrLength);//移动下标
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//获取NBT的N（名称）
		std::string sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			int iRet = GetName(sData, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//读取1字节的列表元素类型
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(sData, bListElementType))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": bListElementType Read");
		}

		//错误的列表元素类型
		if (bListElementType >= NBT_Node::NBT_TAG::ENUM_END)
		{
			return Error(NbtTypeTagError, sData, __FUNCSIG__ ": List NBT Type:Unknow Type Tag[%02X(%d)]", bListElementType, bListElementType);
		}

		//读取4字节的有符号列表长度
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(sData, dwListLength))
		{
			return Error(OutOfRangeError, sData, __FUNCSIG__ ": dwListLength Read");
		}

		//根据元素类型，读取n次列表
		NBT_Node::NBT_List tmpList;
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//列表元素会直接赋值修改
			int iRet = SwitchNBT<false>(sData, tmpNode, (NBT_Node::NBT_TAG)bListElementType, szStackDepth - 1);

			if (iRet != AllOk)//如果iRet是Compound_End则列表包含n个NBT_Node空值，NBT_Node默认初始化即为TAG_End，符合逻辑
			{
				return iRet;//此处只在失败时传递返回值
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
				Error(ElementExistsWarn, sData, __FUNCSIG__ ": the \"%s\"[%s] sData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tmpList).name());
			}
		}
		else//列表中的列表，直接赋值，而不进行插入
		{
			nRoot = NBT_Node{ std::move(tmpList) };
		}

		return AllOk;//列表同时作为元素，成功应该返回Ok，而不是传递返回值
	}

	template<bool bHasName = true>
	static int SwitchNBT(InputStream &sData, NBT_Node &nRoot, NBT_Node::NBT_TAG tag, size_t szStackDepth)//选择函数不检查递归层，由函数调用的函数检查
	{
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
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{//最复杂
				iRet = GetListType<bHasName>(sData, nRoot, szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_Node::TAG_Compound://需要递归调用
			{
				iRet = GetCompoundType<bHasName>(sData, nRoot, szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(sData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(sData, nRoot);
			}
			break;
		default://NBT内标数据签错误
			{
				iRet = Error(NbtTypeTagError, sData, __FUNCSIG__ ": NBT Tag switch default: Unknow Type Tag[%02X(%d)]", tag, tag);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}

		return iRet;//传递返回值
	}

	static int GetNBT(InputStream &sData, NBT_Node &nRoot, size_t szStackDepth)//递归调用读取并添加节点
	{
		CHECK_STACK_DEPTH(szStackDepth);
		//节点类型检查：保证当前nRoot是NBT_Node::NBT_Compound类型，否则失败
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//类型错误
		{
			return Error(InternalTypeError, sData, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
		}

		int iRet;
		do
		{
			if (!sData.HasAvailData(sizeof(char)))//处理末尾情况
			{
				sData.UnGet();//尝试回退一个查看上一字节
				if (sData.HasAvailData(sizeof(char)) && sData.GetChar() == NBT_Node::NBT_TAG::TAG_End)//如果上一字节是TAG_End表明NBT已结束，则退出循环
				{
					iRet = Compound_End;
				}
				else
				{
					iRet = Error(OutOfRangeError, sData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", sData.Index(), sData.Size());
				}
				break;
			}

			//处理正常情况
			iRet = SwitchNBT(sData, nRoot, (NBT_Node::NBT_TAG)(uint8_t)sData.GetChar(), szStackDepth - 1);
		} while (iRet == AllOk);
		
		return iRet;//传递返回值
	}
public:
	NBT_Reader(void) = default;
	~NBT_Reader(void) = default;

	//szStackDepth 控制栈深度，递归层检查仅由可嵌套的可能进行递归的函数进行，栈深度递减仅由对选择函数的调用进行
	bool SetNBT(const std::string data, size_t szDataStartIndex = 0, size_t szStackDepth = 512)//设置nbt到类内
	{//对于用户来说是设置给类
		nRoot.Clear();//清掉原来的数据（注意如果nbt较大的情况下，这是一个较深的递归清理过程，不排除栈空间不足导致清理失败）
		size_t szCurrent{ 0 };
		InputStream sData{ data,szDataStartIndex };
		printf("Max Stack Depth [%zu]\n", szStackDepth);
		return GetNBT(sData, nRoot, szStackDepth) == Compound_End;//对于类来说是从用户给的data获得（get）nbt数据
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}
private:
	NBT_Node nRoot{ NBT_Node::NBT_Compound{} };//默认空节点
};