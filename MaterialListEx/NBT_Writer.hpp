#pragma once

#include <new>//bad alloc
#include <string>
#include <stdint.h>
#include <type_traits>

#include "NBT_Node.hpp"

template <typename T>
class MyOutputStream
{
private:
	T &tData;
public:
	MyOutputStream(T &_tData, size_t szStartIdx = 0) :tData(_tData)
	{
		tData.resize(szStartIdx);
	}
	~MyOutputStream(void) = default;

	template<typename V>
	requires(std::is_constructible_v<T::value_type, V &&>)
	void PutOnce(V &&c)
	{
		tData.push_back(std::forward<V>(c));
	}

	void PutRange(typename T::const_iterator itBeg, typename T::const_iterator itEnd)
	{
		tData.append(itBeg, itEnd);
	}

	void UnPut(void) noexcept
	{
		tData.pop_back();
	}

	size_t GetSize(void) const noexcept
	{
		return tData.size();
	}

	void Reset(void) noexcept
	{
		tData.clear();
	}

	const T &Data(void) const noexcept
	{
		return tData;
	}

	T &Data(void) noexcept
	{
		return tData;
	}
};


template <typename DataType = std::basic_string<uint8_t>>
class NBT_Writer
{
	using OutputStream = MyOutputStream<DataType>;//流类型
private:
	enum ErrCode : int
	{
		ERRCODE_END = -5,//结束标记，统计负数部分大小

		UnknownError,//其他错误
		StdException,//标准异常
		OutOfMemoryError,//内存不足错误
		StringTooLongError,
		AllOk,
	};

	//确保[非错误码]为零，防止出现非法的[非错误码]导致判断失效数组溢出
	static_assert(AllOk == 0, "AllOk != 0");

	static inline const char *const errReason[] =//反向数组运算方式：(-ERRCODE_END - 1) + ErrCode
	{
		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"StringTooLongError",

		"AllOk",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");


	enum WarnCode : int
	{
		NoWarn = 0,

		WARNCODE_END,
	};

	//确保[非警告码]为零，防止出现非法的[非警告码]导致判断失效数组溢出
	static_assert(NoWarn == 0, "NoWarn != 0");

	static inline const char *const warnReason[] =//正常数组，直接用WarnCode访问
	{
		"NoWarn",
	};

	//记得同步数组！
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//error处理
	//使用变参形参表+vprintf代理复杂输出，给出更多扩展信息
	template <typename T, typename std::enable_if<std::is_same<T, ErrCode>::value || std::is_same<T, WarnCode>::value, int>::type = 0>
	static int _cdecl Error(const T &code, const OutputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...)
	{
		/*考虑添加栈回溯，输出详细错误发生的nbt嵌套路径*/



		return code;
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

	//大小端转换
	template<typename T>
	static inline int WriteBigEndian(OutputStream &tData, const T &tVal)
	{
		int iRet = AllOk;
		if constexpr (sizeof(T) == 1)
		{
			MYTRY
			tData.PutOnce((uint8_t)tVal);
			MYCATCH_BADALLOC
		}
		else
		{
			//统一到无符号类型，防止有符号右移错误
			using UT = typename std::make_unsigned<T>::type;
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				MYTRY
				tData.PutOnce((uint8_t)tTmp);
				MYCATCH_BADALLOC
				tTmp >>= 8;
			}
		}

		return iRet;
	}

	//PutName
	static int PutName(OutputStream &tData, const NBT_Type::String &sName)
	{
		int iRet = AllOk;
		//检查大小是否符合上限
		if (sName.length() > UINT16_MAX)
		{
			iRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
			return iRet;
		}

		//输出名称长度
		uint16_t wNameLength = (uint16_t)sName.length();
		iRet = WriteBigEndian(tData, wNameLength);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Write");
			return iRet;
		}

		//输出名称
		MYTRY
		tData.PutRange(sName.begin(), sName.end());
		MYCATCH_BADALLOC

		return AllOk;
	}

	template<typename T>
	static int PutbuiltInType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//原始类型映射

		//获取原始类型，然后转换到raw类型准备写出
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(nRoot.GetData<T>());

		iRet = WriteBigEndian(tData, tTmpRawData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpRawData Write");
			return iRet;
		}

		return iRet;
	}

	//PutArrayType

	//PutCompoundType

	//PutStringType

	//PutListType

	//SwitchNBT

	//PutNBT

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot)
	{

	}


#undef MYTRY
#undef MYCATCH_BADALLOC
#undef MYCATCH_OTHER
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCSIG__
};