#pragma once

#include <new>//bad alloc
#include <string>
#include <stdint.h>
#include <type_traits>

#include "NBT_Node.hpp"

template <typename T = std::basic_string<uint8_t>>
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

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	template<typename V>
	requires(std::is_constructible_v<typename T::value_type, V &&>)
	void PutOnce(V &&c)
	{
		tData.push_back(std::forward<V>(c));
	}

	void PutRange(const typename T::value_type *pData, size_t szSize)
	{
		tData.append(pData, szSize);
	}

	void UnPut(void) noexcept
	{
		tData.pop_back();
	}

	size_t Size(void) const noexcept
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
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

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
		NbtTypeTagError,//NBT标签类型错误

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
		"NbtTypeTagError",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == (ERRCODE_END), "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		EndElementIgnoreWarn,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//正常数组，直接用WarnCode访问
	{
		"NoWarn",

		"EndElementIgnoreWarn",
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
		//打印错误原因
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			if (code >= ERRCODE_END)
			{
				return code;
			}
			//上方if保证code不会溢出
			printf("Read Err[%d]: \"%s\"\n", code, errReason[code]);
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			if (code >= WARNCODE_END)
			{
				return;
			}
			//上方if保证code不会溢出
			printf("Read Warn[%d]: \"%s\"\n", code, warnReason[code]);
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

		//打印扩展信息
		if (cpExtraInfo != NULL)
		{
			printf("Extra Info:\"");
			va_list args;//变长形参
			va_start(args, cpExtraInfo);
			vprintf(cpExtraInfo, args);
			va_end(args);
			printf("\"\n");
		}

		//如果可以，预览szCurrent前n个字符，否则裁切到边界
#define VIEW_PRE (8 * 8 + 8)//向前
		size_t rangeBeg = (tData.Size() > VIEW_PRE) ? (tData.Size() - VIEW_PRE) : (0);//上边界裁切
		size_t rangeEnd = tData.Size();//下边界裁切
#undef VIEW_PRE
		//输出信息
		printf
		(
			"Data Review:\n"\
			"Data Size: 0x%02llX(%zu)\n"\
			"Data[0x%02llX(%zu),0x%02llX(%zu)):\n",

			(uint64_t)tData.Size(), tData.Size(),
			(uint64_t)rangeBeg, rangeBeg,
			(uint64_t)rangeEnd, rangeEnd
		);

		//打数据
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

			printf(" %02X ", (uint8_t)tData[i]);
		}

		//输出提示信息
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			printf("\nSkip err and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			printf("\nSkip warn and continue...\n\n");
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

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
if((Depth) <= 0)\
{\
	eRet = Error(StackDepthExceeded, tData, _RP___FUNCSIG__ ": NBT nesting depth exceeded maximum call stack limit");\
	STACK_TRACEBACK("(Depth) <= 0");\
	return eRet;\
}\

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

	//写出大端序值
	template<typename T>
	static inline ErrCode WriteBigEndian(OutputStream &tData, const T &tVal) noexcept
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
			static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

			for (size_t i = sizeof(T); i > 0; --i)
			{
				tData.PutOnce((uint8_t)(((UT)tVal) >> (8 * (i - 1))));//依次提取
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
			eRet = Error(StringTooLongError, tData, __FUNCSIG__ ": szStringLength[%zu] > StringLength_Max[%zu]",
				szStringLength, (size_t)NBT_Type::StringLength_Max);
			STACK_TRACEBACK("szStringLength Test");
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
		tData.PutRange((const typename DataType::value_type *)sName.data(), sName.size());

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
			STACK_TRACEBACK("tTmpRawData Write");
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
			eRet = Error(ArrayTooLongError, tData, __FUNCSIG__ ": szArrayLength[%zu] > ArrayLength_Max[%zu]",
				szArrayLength, (size_t)NBT_Type::ArrayLength_Max);
			STACK_TRACEBACK("szArrayLength Test");
			return eRet;
		}

		//获取实际写出大小
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		eRet = WriteBigEndian(tData, iArrayLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("iArrayLength Write");
			return eRet;
		}

		//写出元素
		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			typename T::value_type tTmpData = tArray[i];
			eRet = WriteBigEndian(tData, tTmpData);
			if (eRet < AllOk)
			{
				STACK_TRACEBACK("tTmpData Write");
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
		for (const auto &[sName, nodeNbt] : tCompound)
		{
			NBT_TAG curTag = nodeNbt.GetTag();

			//集合中如果存在nbt end类型的元素，删除而不输出
			if (curTag == NBT_TAG::End)
			{
				//End元素被忽略警告（警告不返回错误码）
				Error(EndElementIgnoreWarn, tData, __FUNCSIG__ ": Name: \"%s\", type is [NBT_Type::End], ignored!",
					U16ANSI(U16STR(sName)).c_str());
				continue;
			}

			//先写出tag
			eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)curTag);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("curTag Write");
				return eRet;
			}

			//然后写出name
			eRet = PutName(tData, sName);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutName Fail, Type: [NBT_Type::%s]", NBT_Type::GetTypeName(curTag));
				return eRet;
			}

			//最后根据tag类型写出数据
			eRet = PutSwitch(tData, nodeNbt, curTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Fail, Name: \"%s\", Type: [NBT_Type::%s]",
					U16ANSI(U16STR(sName)).c_str(), NBT_Type::GetTypeName(curTag));
				return eRet;
			}
		}

		//注意Compound类型有一个NBT_TAG::End结尾，如果写出错误则在前面返回，不放置结尾
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)NBT_TAG::End);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("NBT_TAG::End[0x00(0)] Write");
			return eRet;
		}

		return eRet;
	}

	static ErrCode PutStringType(OutputStream &tData, const NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		eRet = PutName(tData, tString);//借用PutName实现，因为string走的name相同操作
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("PutString");//因为是借用实现，所以这里小小的改个名，防止报错Name误导人
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
			eRet = Error(ListTooLongError, tData, __FUNCSIG__ ": szListLength[%zu] > ListLength_Max[%zu]",
				szListLength, (size_t)NBT_Type::ListLength_Max);
			STACK_TRACEBACK("szListLength Test");
			return eRet;
		}

		//转换为写入大小
		NBT_Type::ListLength iListLength = (NBT_Type::ListLength)szListLength;

		//判断：长度不为0但是拥有空标签
		NBT_TAG enListValueTag = tList.enElementTag;
		if (enListValueTag == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found",
				iListLength);
			STACK_TRACEBACK("iListLength And enListValueTag Test");
			return eRet;
		}

		//获取列表标签，如果列表长度为0，则强制改为空标签
		NBT_TAG enListElementTag = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//写出标签
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)enListElementTag);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("enListElementTag Write");
			return eRet;
		}

		//写出长度
		eRet = WriteBigEndian(tData, iListLength);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("iListLength Write");
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
				eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": Expected type [NBT_Type::%s][0x%02X(%d)] in list, but found type [NBT_Type::%s][0x%02X(%d)]",
				NBT_Type::GetTypeName(enListElementTag), (NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag,
				NBT_Type::GetTypeName(curTag), (NBT_TAG_RAW_TYPE)curTag, (NBT_TAG_RAW_TYPE)curTag);
				STACK_TRACEBACK("curTag Test");
				return eRet;
			}

			//一致，很好，那么输出
			//列表无名字，无需重复tag，只需输出数据
			eRet = PutSwitch(tData, tmpNode, enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Error, Size: [%d] Index: [%d]", iListLength, i);
				return eRet;
			}
		}

		return eRet;
	}

	static ErrCode PutSwitch(OutputStream &tData, const NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth) noexcept
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::String://类型唯一，非模板函数
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				eRet = PutStringType(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::List://可能递归，需要处理szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				eRet = PutListType(tData, nodeNbt.GetData<CurType>(), szStackDepth);
			}
			break;
		case NBT_TAG::Compound://可能递归，需要处理szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				eRet = PutCompoundType(tData, nodeNbt.GetData<CurType>(), szStackDepth);
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::End://注意end标签绝对不可以进来
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]");
			}
			break;
		default://数据出错
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}

		if (eRet != AllOk)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x%02X(%d)] write error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return eRet;
	}

public:
	//输出到tData中，部分功能和原理参照ReadNBT处的注释，szDataStartIndex在此处可以对一个tData通过不同的tCompound和szDataStartIndex = tData.size()
	//来调用以达到把多个不同的nbt输出到同一个tData内的功能
	static bool WriteNBT(DataType &tData, const NBT_Type::Compound &tCompound, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept
	{
	MYTRY;
		//初始化数据流对象
		OutputStream OptStream(tData, szDataStartIndex);

		//输出最大栈深度
		//printf("Max Stack Depth [%zu]\n", szStackDepth);

		//开始递归输出
		return PutCompoundType(OptStream, tCompound, szStackDepth) == AllOk;
	MYCATCH;//以防万一还是需要捕获一下
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