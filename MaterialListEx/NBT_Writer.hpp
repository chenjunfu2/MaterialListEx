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
	enum ErrCode : uint8_t
	{
		AllOk = 0,//没有问题

		UnknownError,//其他错误
		StdException,//标准异常
		OutOfMemoryError,//内存不足错误
		ListElementTypeError,//列表元素类型错误（代码问题）
		StackDepthExceeded,//调用栈深度过深（代码问题）
		StringTooLongError,//字符串过长错误
		ArrayTooLongError,//数组过长错误
		ListTooLongError,//列表过长错误

		ERRCODE_END,//结束标记
	};

	constexpr static inline const char *const errReason[] =
	{
		"AllOk",

		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"StringTooLongError",
		"ArrayTooLongError",
		"ListTooLongError",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (-ERRCODE_END), "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//正常数组，直接用WarnCode访问
	{
		"NoWarn",
	};

	//记得同步数组！
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//error处理
	//使用变参形参表+vprintf代理复杂输出，给出更多扩展信息
	//主动检查引发的错误，主动调用eRet = Error报告，然后触发STACK_TRACEBACK，最后返回eRet到上一级
	//上一级返回的错误通过if (eRet != AllOk)判断的，直接触发STACK_TRACEBACK后返回eRet到上一级
	//如果是警告值，则不返回值
	template <typename T>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> _cdecl Error(const T &code, const OutputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gcc使用__attribute__((format))，msvc使用_Printf_format_string_
	{

		//警告不返回值
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			return code;
		}
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
		eRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
		STACK_TRACEBACK("(Depth) <= 0");\
		return eRet;\
	}\
}

#define MYTRY \
try\
{

#define MYCATCH \
}\
catch(const std::bad_alloc &e)\
{\
	ErrCode eRet = Error(OutOfMemoryError, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return eRet;\
}\
catch(const std::exception &e)\
{\
	ErrCode eRet = Error(StdException, tData, _RP___FUNCSIG__ ": Info:[%s]", e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return eRet;\
}\
catch(...)\
{\
	ErrCode eRet =  Error(UnknownError, tData, _RP___FUNCSIG__ ": Info:[Unknown Exception]");\
	STACK_TRACEBACK("catch(...)");\
	return eRet;\
}

	//大小端转换
	template<typename T>
	static ErrCode WriteBigEndian(OutputStream &tData, const T &tVal) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		if constexpr (sizeof(T) == 1)
		{
			tData.PutOnce((uint8_t)tVal);
		}
		else
		{
			//统一到无符号类型，防止有符号右移错误
			using UT = typename std::make_unsigned<T>::type;
			UT tTmp = (UT)tVal;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tData.PutOnce((uint8_t)tTmp);
				tTmp >>= 8;
			}
		}

		return eRet;
	MYCATCH;
	}

	static ErrCode PutName(OutputStream &tData, const NBT_Type::String &sName) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//获取string长度
		size_t szStringLength = sName.size();

		//检查大小是否符合上限
		if (szStringLength > (size_t)NBT_Type::StringLength_Max)
		{
			eRet = Error(StringTooLongError, tData, __FUNCSIG__ ": sName.length()[%zu] > UINT16_MAX[%zu]", sName.length(), UINT16_MAX);
			//STACK_TRACEBACK
			return eRet;
		}

		//输出名称长度
		NBT_Type::StringLength wNameLength = (uint16_t)szStringLength;
		eRet = WriteBigEndian(tData, wNameLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Write");
			return eRet;
		}

		//输出名称
		tData.PutRange(sName.begin(), sName.end());

		return eRet;
	MYCATCH;
	}

	template<typename T>
	static ErrCode PutbuiltInType(OutputStream &tData, const T &tBuiltIn) noexcept
	{
		ErrCode eRet = AllOk;

		//获取原始类型，然后转换到raw类型准备写出
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//原始类型映射
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(tBuiltIn);

		eRet = WriteBigEndian(tData, tTmpRawData);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("Name: \"%s\" tTmpRawData Write");
			return eRet;
		}

		return eRet;
	}

	template<typename T>
	static ErrCode PutArrayType(OutputStream &tData, const T &tArray) noexcept
	{
		ErrCode eRet = AllOk;

		//获取数组大小判断是否超过要求上限
		//也就是4字节有符号整数上限
		size_t szArrayLength = tArray.size();
		if (szArrayLength > (size_t)NBT_Type::ArrayLength_Max)
		{
			//error
			//stack
			return eRet;
		}

		//获取实际写出大小
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		eRet = WriteBigEndian(tData, iArrayLength);
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		//写出元素
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			eRet = WriteBigEndian(tData, tTmpData);
			if (eRet < AllOk)
			{
				//stack
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutCompoundType(OutputStream &tData, const NBT_Type::Compound &tCompound, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		
		//注意compound是为数不多的没有元素数量限制的结构
		//此处无需检查大小，且无需写出大小
		for (const auto &it : tCompound)
		{
			NBT_TAG curTag = it.second.GetTag();

			//集合中如果存在nbt end类型的元素，删除而不输出
			if (curTag == NBT_TAG::End)
			{
				continue;
			}

			//先写出tag
			eRet = WriteBigEndian((NBT_TAG_RAW_TYPE)curTag);
			if (eRet != AllOk)
			{
				//stack
				break;
			}

			//然后写出name
			eRet = PutName(tData, it.first);
			if (eRet != AllOk)
			{
				//stack
				break;
			}

			//最后根据tag类型写出数据
			eRet = PutSwitch(tData, it.second, curTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				//stack
				break;
			}
		}

		return eRet;
	}

	static ErrCode PutStringType(OutputStream &tData, const NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		eRet = PutName(tData, tString);//借用PutName实现，因为string走的name相同操作
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		return eRet;
	}

	static ErrCode PutListType(OutputStream &tData, const NBT_Type::List &tList, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//检查
		size_t szListLength = tList.size();
		if (szListLength > (size_t)NBT_Type::ListLength_Max)//大于的情况下强制赋值会导致严重问题，只能返回错误
		{
			//error
			//stack
			return eRet;
		}

		//转换为写入大小
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//判断：长度不为0但是拥有空标签
		NBT_TAG enListValueTag = tList.enElementTag;
		if (iListLength != 0 && enListValueTag == NBT_TAG::End)
		{
			//error
			//stack
			return eRet;
		}

		//获取列表标签，如果列表长度为0，则强制改为空标签
		NBT_TAG enListElementTag = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//写出标签
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)enListElementTag);
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		//写出长度
		eRet = WriteBigEndian(tData, iListLength);
		if (eRet < AllOk)
		{
			//stack
			return eRet;
		}

		//写出列表（递归）
		//写出时判断元素标签与enListElementTag不一致的错误
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			//获取元素与类型
			const NBT_Node &tmpNode = tList[i];
			NBT_TAG curTag = tmpNode.GetTag();

			//对于每个元素，检查类型是否与列表存储一致
			if (curTag != enListElementTag)
			{
				//error
				//stack
				return eRet;
			}

			//一致，很好，那么输出
			//列表无名字，无需重复tag，只需输出数据
			eRet = PutSwitch(tData, tmpNode, enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				//stack
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutSwitch(OutputStream &tData, const NBT_Node &nRoot, NBT_TAG tagNbt, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;





		return eRet;
	}

public:
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

	static bool WriteNBT(DataType &tData, const NBT_Node &nRoot, size_t szStackDepth = 512)
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