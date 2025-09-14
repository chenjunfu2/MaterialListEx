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
		ERRCODE_END = -9,//结束标记，统计负数部分大小

		UnknownError,//其他错误
		StdException,//标准异常
		OutOfMemoryError,//内存不足错误
		ListElementTypeError,//列表元素类型错误（代码问题）
		StackDepthExceeded,//调用栈深度过深（代码问题）
		StringTooLongError,//字符串过长错误
		ArrayTooLongError,//数组过长错误
		ListTooLongError,//列表过长错误

		AllOk,//没有问题
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
		"StringTooLongError",
		"ArrayTooLongError",
		"ListTooLongError",

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
	/*
		主动检查引发的错误，主动调用iRet = Error报告，然后触发STACK_TRACEBACK，最后返回iRet到上一级
		上一级返回的错误通过if (iRet < AllOk)判断的，直接触发STACK_TRACEBACK后返回iRet到上一级
	*/
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

		//获取string长度
		size_t szStringLength = sName.size();

		//检查大小是否符合上限
		if (szStringLength > (size_t)NBT_Type::StringLength_Max)
		{
			iRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
			return iRet;
		}

		//输出名称长度
		NBT_Type::StringLength wNameLength = (uint16_t)szStringLength;
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

		//获取原始类型，然后转换到raw类型准备写出
		const T &tBuiltIn = nRoot.GetData<T>();

		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//原始类型映射
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(tBuiltIn);

		iRet = WriteBigEndian(tData, tTmpRawData);
		if (iRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpRawData Write");
			return iRet;
		}

		return iRet;
	}

	template<typename T>
	static int PutArrayType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		const T &tArray = nRoot.GetData<T>();

		//获取数组大小判断是否超过要求上限
		//也就是4字节有符号整数上限
		size_t szArrayLength = tArray.size();
		if (szArrayLength > (size_t)NBT_Type::ArrayLength_Max)
		{
			//error
			//stack
			return iRet;
		}

		//获取实际写出大小
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		iRet = WriteBigEndian(tData, iArrayLength);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//写出元素
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			iRet = WriteBigEndian(tData, tTmpData);
			if (iRet < AllOk)
			{
				//stack
				return iRet;
			}
		}

		return iRet;
	}

	static int PutCompoundType(OutputStream &tData, const NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		//集合中如果存在nbt end类型的元素，删除而不输出



		return iRet;
	}

	static int PutStringType(OutputStream &tData, const NBT_Node &nRoot)
	{
		int iRet = AllOk;

		const NBT_Type::String &tString = nRoot.GetData<NBT_Type::String>();
		iRet = PutName(tData, tString);//借用PutName实现，因为string走的name相同操作
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}


		return iRet;
	}

	static int PutListType(OutputStream &tData, const NBT_Node &nRoot, size_t szStackDepth)
	{
		int iRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		const NBT_Type::List tList = nRoot.GetData<NBT_Type::List>();

		//检查
		size_t szListLength = tList.size();
		if (szListLength > (size_t)NBT_Type::ListLength_Max)//大于的情况下强制赋值会导致严重问题，只能返回错误
		{
			//error
			//stack
			return iRet;
		}

		//转换为写入大小
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//判断：长度不为0但是拥有空标签
		NBT_TAG enListValueTag = tList.enElementTag;
		if (iListLength != 0 && enListValueTag == NBT_TAG::End)
		{
			//error
			//stack
			return iRet;
		}

		//获取列表标签，如果列表长度为0，则强制改为空标签
		NBT_TAG bListElementType = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//写出标签
		iRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)bListElementType);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//写出长度
		iRet = WriteBigEndian(tData, iListLength);
		if (iRet < AllOk)
		{
			//stack
			return iRet;
		}

		//写出列表（递归）
		//写出时判断元素标签与bListElementType不一致的错误
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			//这里暂时不实现，需要调用下面SwitchNBT递归


		}

		return iRet;
	}

	//static int SwitchNBT(OutputStream &tData, const NBT_Node &nRoot)

	//static int PutNBT(OutputStream &tData, const NBT_Node &nRoot)

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot)
	{


		return true;
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