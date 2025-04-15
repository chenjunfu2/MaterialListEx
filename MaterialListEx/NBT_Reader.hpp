#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"

#include <new>//std::bad_alloc

template <typename T>
class MyInputStream
{
private:
	const T &tData;
	size_t szIndex;
public:
	MyInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
	{}
	~MyInputStream() = default;//默认析构，走tData的析构即可

	typename T::value_type GetNext() noexcept
	{
		return tData[szIndex++];
	}

	void UnGet() noexcept
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	T::const_iterator Current() const noexcept
	{
		return tData.begin() + szIndex;
	}

	T::const_iterator Next(size_t szSize) const noexcept
	{
		return tData.begin() + (szIndex + szSize);
	}

	size_t AddIndex(size_t szSize) noexcept
	{
		return szIndex += szSize;
	}

	size_t SubIndex(size_t szSize) noexcept
	{
		return szIndex -= szSize;
	}

	bool IsEnd() const noexcept
	{
		return szIndex >= tData.size();
	}

	size_t Size() const noexcept
	{
		return tData.size();
	}

	bool HasAvailData(size_t szSize) const noexcept
	{
		return (tData.size() - szIndex) >= szSize;
	}

	void Reset() noexcept
	{
		szIndex = 0;
	}

	const T &Data() const noexcept
	{
		return tData;
	}

	size_t Index() const noexcept
	{
		return szIndex;
	}

	size_t &Index() noexcept
	{
		return szIndex;
	}
};

template <typename DataType = std::string>
class NBT_Reader
{
	using InputStream = MyInputStream<DataType>;//流类型
private:
	enum ErrCode : int
	{
		ERRCODE_END = -9,//结束标记，统计负数部分大小

		UnknownError,//其他错误
		StdException,//标准异常
		OutOfMemoryError,//内存不足错误（NBT文件问题）
		ListElementTypeError,//列表元素类型错误（NBT文件问题）
		StackDepthExceeded,//调用栈深度过深（NBT文件or代码设置问题）
		NbtTypeTagError,//NBT标签类型错误（NBT文件问题）
		OutOfRangeError,//（NBT内部长度错误溢出）（NBT文件问题）
		InternalTypeError,//变体NBT节点类型错误（代码问题）
		AllOk,//没有问题
		Compound_End,//集合结束
	};

	//确保[非错误码]为零，防止出现非法的[非错误码]导致判断失效数组溢出
	static_assert(AllOk == 0, "AllOk != 0");

	static inline const char *const errReason[] =//反向数组运算方式：(-ERRCODE_END - 1) + ErrCode
	{
		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"NbtTypeTagError",
		"OutOfRangeError",
		"InternalTypeError",

		"AllOk",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

	enum WarnCode : int
	{
		NoWarn = 0,
		ElementExistsWarn,

		WARNCODE_END,
	};

	//确保[非警告码]为零，防止出现非法的[非警告码]导致判断失效数组溢出
	static_assert(NoWarn == 0, "NoWarn != 0");

	static inline const char *const warnReason[] =//正常数组，直接用WarnCode访问
	{
		"NoWarn",
		"ElementExistsWarn",
	};

	//记得同步数组！
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//使用变参形参表+vprintf代理复杂输出，给出更多扩展信息
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(const T &code, const InputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gcc使用__attribute__((format))
	{
		if constexpr (std::is_same<T, ErrCode>::value)
		{
			if (code >= AllOk)
			{
				return (int)code;
			}
			//上方if保证errc为负，此处访问保证无问题（除非代码传入异常错误码），通过(-ERRCODE_END - 1) + code得到反向数组下标
			printf("Read Err[%d]: \"%s\"\n", (int)code, errReason[(-ERRCODE_END - 1) + code]);
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			if (code <= NoWarn)
			{
				return (int)code;
			}
			//输出warn警告
			printf("Read Warn[%d]: \"%s\"\n", (int)code, warnReason[(int)code]);
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
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
		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : 0;//上边界裁切
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : tData.Size();//下边界裁切
#undef VIEW_SUF
#undef VIEW_PRE
		printf("Data Review:\nCurrent: 0x%02llX(%zu)\nData Size: 0x%02llX(%zu)\nData[0x%02llX(%zu)] ~ Data[0x%02llX(%zu)]:\n",
			(uint64_t)tData.Index(), tData.Index(), (uint64_t)tData.Size(), tData.Size(), (uint64_t)rangeBeg, rangeBeg, (uint64_t)rangeEnd - 1, rangeEnd - 1);
		
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

			if (i != tData.Index())
			{
				printf(" %02X ", (uint8_t)tData[i]);
			}
			else//如果是当前出错字节，加方括号框起
			{
				printf("[%02X]", (uint8_t)tData[i]);
			}
		}

		if constexpr (std::is_same<T, ErrCode>::value)
		{
			printf("\nSkip err data and return...\n\n");
		}
		else if constexpr (std::is_same<T, WarnCode>::value)
		{
			printf("\nSkip warn data and continue...\n\n");
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

		return (int)code;
	}

#define _RP___FUNCSIG__ __FUNCSIG__//用于编译过程二次替换达到函数内部

#define _RP___LINE__ _RP_STRLING(__LINE__)
#define _RP_STRLING(l) STRLING(l)
#define STRLING(l) #l

#define STACK_TRACEBACK(fmt, ...) printf("In [" _RP___FUNCSIG__ "] Line:[" _RP___LINE__ "]: \n"##fmt "\n\n", ##__VA_ARGS__);
#define CHECK_STACK_DEPTH(Depth) \
{\
	if((Depth) <= 0)\
	{\
		iRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
		STACK_TRACEBACK("(Depth) <= 0");\
		return iRet;\
	}\
}

#define MYTRY \
try\
{

#define MYCATCH_BADALLOC \
}\
catch(const std::bad_alloc &e)\
{\
	int iRet = Error(OutOfMemoryError, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return iRet;\
}

#define MYCATCH_OTHER \
}\
catch(const std::exception &e)\
{\
	int iRet = Error(StdException, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return iRet;\
}\
catch(...)\
{\
	int iRet =  Error(UnknownError, tData, _RP___FUNCSIG__ ": Info:[Unknown Exception]");\
	STACK_TRACEBACK("catch(...)");\
	return iRet;\
}


	// 读取大端序数值，bNoCheck为true则不进行任何检查
	template<bool bNoCheck = false, typename T>
	static std::conditional_t<bNoCheck, void, int> ReadBigEndian(InputStream &tData, T &tVal) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				int iRet = Error(OutOfRangeError, tData, "tData size [%zu], current index [%zu], remaining data size [%zu], but try to read [%zu]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData");
				return iRet;
			}
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)tData.GetNext();
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp <<= 8;
				tTmp |= (T)(uint8_t)tData.GetNext();//因为只会左移，不存在有符号导致的算术位移bug，不用转换为无符号类型
			}
			tVal = tTmp;
		}

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	static int GetName(InputStream &tData, NBT_Node::NBT_String &sName)
	{
		int iRet = AllOk;
		//读取2字节的无符号名称长度
		uint16_t wNameLength = 0;//w->word=2*byte
		iRet = ReadBigEndian(tData, wNameLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Read");
			return iRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(wNameLength))
		{
			int iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wNameLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wNameLength, tData.Index() + (size_t)wNameLength, tData.Size());
			STACK_TRACEBACK("HasAvailData");
			return iRet;
		}

		MYTRY
		//解析出名称
		sName.reserve(wNameLength);//提前分配
		sName.assign(tData.Current(), tData.Next(wNameLength));//如果长度为0则构造0长字符串，合法行为
		MYCATCH_BADALLOC
		tData.AddIndex(wNameLength);//移动下标
		return AllOk;
	}

	template<typename T>
	struct BUILTIN_TYPE
	{
		using Type = T;
		static_assert(std::is_integral<T>::value, "Not a legal type!");//抛出编译错误
	};

	template<>
	struct BUILTIN_TYPE<NBT_Node::NBT_Float>//浮点数映射
	{
		using Type = uint32_t;
		static_assert(sizeof(Type) == sizeof(NBT_Node::NBT_Float), "Type size does not match!");
	};

	template<>
	struct BUILTIN_TYPE<NBT_Node::NBT_Double>//浮点数映射
	{
		using Type = uint64_t;
		static_assert(sizeof(Type) == sizeof(NBT_Node::NBT_Double), "Type size does not match!");
	};

	template<typename T, bool bHasName = true>
	static int GetbuiltInType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//获取NBT的N（名称）
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//读取数据
		using DATA_T = BUILTIN_TYPE<T>::Type;//类型映射

		DATA_T tTmpData = 0;
		iRet = ReadBigEndian(tData, tTmpData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpData Read", sName.c_str());
			return iRet;
		}

		if constexpr (bHasName)
		{
			MYTRY
			//名称-内含数据的节点插入当前调用栈深度的根节点
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move((*((T *)&tTmpData))));//无损数据类型转换
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(T).name());
			}
			MYCATCH_BADALLOC
		}
		else
		{
			//无名，为列表元素，直接修改nRoot
			nRoot.emplace<T>(std::move((*((T *)&tTmpData))));
		}

		return iRet;
	}

	template<typename T, bool bHasName = true>
	static int GetArrayType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//获取NBT的N（名称）
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//获取4字节有符号数，代表数组元素个数
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		iRet = ReadBigEndian(tData, dwElementCount);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" dwElementCount Read", sName.c_str());
			return iRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(dwElementCount * sizeof(T::value_type)))//保证下方调用安全
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + dwElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)dwElementCount, sizeof(T::value_type), tData.Index() + (size_t)dwElementCount * sizeof(T::value_type), tData.Size());
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}
		
		//数组保存
		T tArray{};
		MYTRY
		tArray.reserve(dwElementCount);//提前扩容
		//读取dElementCount个元素
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			ReadBigEndian<true>(tData, tTmpData);//调用需要确保范围安全
			tArray.emplace_back(std::move(tTmpData));//读取一个插入一个
		}
		MYCATCH_BADALLOC
		
		if constexpr (bHasName)
		{
			MYTRY
			//完成后插入
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tArray));
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tArray).name());
			}
			MYCATCH_BADALLOC
		}
		else//无名称，为列表元素
		{
			nRoot.emplace<T>(std::move(tArray));
		}

		return iRet;
	}

	template<bool bHasName = true>
	static int GetCompoundType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)//此递归函数需要保证传递返回值
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//获取NBT的N（名称）
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//开始递归
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		//此处无需捕获异常，因为GetNBT内部调用的switchNBT不会抛出异常
		iRet = GetNBT(tData, nodeTemp, szStackDepth);//此处直接传递，因为GetNBT内部会进行递减
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" GetNBT", sName.c_str());
			//return iRet;//不跳过，进行插入，以便分析错误之前的正确数据
		}

		if constexpr (bHasName)
		{
			MYTRY
			//递归完成，所有子节点已到位
			//取出NBT_Compound挂到自己根部（移动）
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(nodeTemp));
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_Compound).name());
			}
			MYCATCH_BADALLOC
		}
		else//无名称，为列表元素
		{
			nRoot = std::move(nodeTemp);
		}

		return iRet >= AllOk ? AllOk : iRet;//>=AllOk强制返回AllOk以防止Compound_End
	}

	template<bool bHasName = true>
	static int GetStringType(InputStream &tData, NBT_Node &nRoot)
	{
		int iRet = AllOk;
		//获取NBT的N（名称）
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//读取2字节的无符号名称长度
		uint16_t wStrLength = 0;//w->word=2*byte
		iRet = ReadBigEndian(tData, wStrLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" wStrLength Read", sName.c_str());
			return iRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(wStrLength))
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStrLength[%zu])[%zu] > DataSize[%zu]",
				tData.Index(), (size_t)wStrLength, tData.Index() + (size_t)wStrLength, tData.Size());
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//读取字符串
		NBT_Node::NBT_String sString{};
		MYTRY
		sString.reserve(wStrLength);//预分配
		sString.assign(tData.Current(), tData.Next(wStrLength));//范围赋值
		MYCATCH_BADALLOC
		tData.AddIndex(wStrLength);//移动下标

		if constexpr (bHasName)
		{
			MYTRY
			//原位构造
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(sString));
			if (!ret.second)//插入失败，元素已存在
			{
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(NBT_Node::NBT_String).name());
			}
			MYCATCH_BADALLOC
		}
		else//列表元素直接赋值
		{
			nRoot.emplace<NBT_Node::NBT_String>(std::move(sString));
		}
		
		return iRet;
	}

	template<bool bHasName = true>
	static int GetListType(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//获取NBT的N（名称）
		NBT_Node::NBT_String sName{};
		if constexpr (bHasName)//如果无名称则string默认为空
		{
			iRet = GetName(tData, sName);
			if (iRet < AllOk)
			{
				STACK_TRACEBACK("GetName");
				return iRet;
			}
		}

		//读取1字节的列表元素类型
		uint8_t bListElementType = 0;//b=byte
		iRet = ReadBigEndian(tData, bListElementType);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" bListElementType Read", sName.c_str());
			return iRet;
		}

		//错误的列表元素类型
		if (bListElementType >= NBT_Node::NBT_TAG::ENUM_END)
		{
			iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[%02X(%d)]", bListElementType, bListElementType);
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//读取4字节的有符号列表长度
		int32_t dwListLength = 0;//dw=double-world=4*byte
		iRet = ReadBigEndian(tData, dwListLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" dwListLength Read", sName.c_str());
			return iRet;
		}

		//检查有符号数大小范围
		if (dwListLength < 0)
		{
			iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": dwListLength[%d] < 0", dwListLength);
			STACK_TRACEBACK("Name: \"%s\"", sName.c_str());
			return iRet;
		}

		//根据元素类型，读取n次列表
		NBT_Node::NBT_List tmpList{};
		MYTRY
		tmpList.reserve(dwListLength);//已知大小提前分配减少开销

		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//列表元素会直接赋值修改
			iRet = SwitchNBT<false>(tData, tmpNode, (NBT_Node::NBT_TAG)bListElementType, szStackDepth - 1);
			if (iRet < AllOk)//错误处理
			{
				STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d]", sName.c_str(), dwListLength, i);
				break;//跳出循环以保留错误数据之前的正确数据
			}

			if (bListElementType == NBT_Node::NBT_TAG::TAG_End)//空元素特判
			{
				//读取1字节的bCompoundEndTag
				uint8_t bCompoundEndTag = 0;//b=byte
				iRet = ReadBigEndian(tData, bCompoundEndTag);
				if (iRet < AllOk)
				{
					STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d] bCompoundEndTag Read", sName.c_str(), dwListLength, i);
					break;//跳出循环以保留错误数据之前的正确数据
				}

				//判断tag是否符合
				if (bCompoundEndTag != NBT_Node::TAG_End)
				{
					iRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": require Compound End Tag [0x00], but read Unknown Tag [0x%02X]!", bCompoundEndTag);
					STACK_TRACEBACK("Name: \"%s\" Size: [%d] Index: [%d]", sName.c_str(), dwListLength, i);
					break;//跳出循环以保留错误数据之前的正确数据
				}
			}

			//每读取一个往后插入一个
			tmpList.emplace_back(std::move(tmpNode));
		}
		MYCATCH_BADALLOC

		//列表可嵌套，所以处理本身嵌套无名情况
		if constexpr (bHasName)
		{
			MYTRY
			auto ret = nRoot.GetData<NBT_Node::NBT_Compound>().try_emplace(std::move(sName), std::move(tmpList));
			if (!ret.second)//插入失败，元素已存在
			{
				//此处实为警告而非错误，无需return
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": the \"%s\"[%s] tData already exist!", ANSISTR(U16STR(sName)).c_str(), typeid(tmpList).name());
			}
			MYCATCH_BADALLOC
		}
		else//列表中的列表，直接赋值，而不进行插入
		{
			nRoot.emplace<NBT_Node::NBT_List>(std::move(tmpList));
		}

		//如果错误，则传递，否则返回Ok
		return iRet >= AllOk ? AllOk : iRet;//列表同时作为元素，成功应该返回Ok，而不是传递返回值
	}

	//这个函数拦截所有内部调用产生的异常并处理返回，所以此函数绝对不抛出异常，由此调用此函数的函数也可无需catch异常
	template<bool bHasName = true>
	static int SwitchNBT(InputStream &tData, NBT_Node &nRoot, NBT_Node::NBT_TAG tag, size_t szStackDepth) noexcept//选择函数不检查递归层，由函数调用的函数检查
	{
		int iRet = AllOk;

		MYTRY
		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				iRet = Compound_End;//返回结尾
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_ByteArray, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{//最复杂
				iRet = GetListType<bHasName>(tData, nRoot, szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_Node::TAG_Compound://需要递归调用
			{
				iRet = GetCompoundType<bHasName>(tData, nRoot, szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_IntArray, bHasName>(tData, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_LongArray, bHasName>(tData, nRoot);
			}
			break;
		default://NBT内标数据签错误
			{
				iRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[%02X(%d)]", tag, tag);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}
		MYCATCH_OTHER//这里捕获所有其它未捕获的异常

		if (iRet < AllOk)
		{
			STACK_TRACEBACK("iRet < AllOk");
		}

		return iRet;//传递返回值
	}

	//不抛出异常
	template<bool bRoot = false>//根部特化版本，用于处理末尾
	static int GetNBT(InputStream &tData, NBT_Node &nRoot, size_t szStackDepth) noexcept//递归调用读取并添加节点
	{
		int iRet = AllOk;//默认值
		CHECK_STACK_DEPTH(szStackDepth);
		//节点类型检查：保证当前nRoot是NBT_Node::NBT_Compound类型，否则失败
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//类型错误
		{
			iRet = Error(InternalTypeError, tData, __FUNCSIG__ ": nRoot is not type: [%s]", typeid(NBT_Node::NBT_Compound).name());
			STACK_TRACEBACK("TypeHolds");
			return iRet;
		}
		
		do
		{
			if (!tData.HasAvailData(sizeof(uint8_t)))//处理末尾情况
			{
				if constexpr (!bRoot)//非根部情况遇到末尾，则报错
				{
					iRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", tData.Index(), tData.Size());
					break;
				}
				else
				{
					iRet = AllOk;
					break;//否则根部直接跳出
				}
			}

			//处理正常情况
			iRet = SwitchNBT(tData, nRoot, (NBT_Node::NBT_TAG)(uint8_t)tData.GetNext(), szStackDepth - 1);//已捕获所有异常
		} while (iRet == AllOk);//iRet<AllOk即为错误，跳出循环，>AllOk则为其它动作，跳出循环

		if constexpr (bRoot)//根部情况遇到Compound_End则转为AllOk返回
		{
			if (iRet >= AllOk)
			{
				iRet = AllOk;
			}
		}

		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Compound Size: [%zu]", nRoot.GetData<NBT_Node::NBT_Compound>().size());
		}
		
		return iRet;//传递返回值
	}
public:
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	//szStackDepth 控制栈深度，递归层检查仅由可嵌套的可能进行递归的函数进行，栈深度递减仅由对选择函数的调用进行
	static bool ReadNBT(NBT_Node &nRoot, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//从data中读取nbt
	{
		MYTRY
		//初始化NBT根对象
		nRoot.Clear();//清掉原来的数据（注意如果nbt较大的情况下，这是一个较深的递归清理过程，不排除栈空间不足导致清理失败），可能抛出异常之类的，需要try catch
		nRoot.emplace<NBT_Node::NBT_Compound>();//设置为compound

		//初始化数据流对象
		InputStream IptStream{ tData,szDataStartIndex };

		//输出最大栈深度
		printf("Max Stack Depth [%zu]\n", szStackDepth);

		//开始递归读取
		return GetNBT<true>(IptStream, nRoot, szStackDepth) == AllOk;//从data中获取nbt数据到nRoot中，只有此调用为根部调用（模板true），用于处理特殊情况
		MYCATCH_OTHER//捕获其他异常
	}


#undef MYTRY
#undef MYCATCH
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCSIG__
};