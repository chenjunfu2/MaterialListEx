#pragma once

#include "NBT_Node.hpp"//nbt类型
#include "NBT_Visitor.hpp"//鸭子类与部分实现
#include "NBT_Endian.hpp"//字节序
#include "NBT_IO.hpp"//IO流对象

#include <stdint.h>

/// @file
/// @brief NBT类型二进制流扫描工具

/// @brief NBT 数据流式扫描器，通过访问器回调处理 NBT 结构，不构建完整内存树
/// @note 该类提供静态方法 ScanNBT，以流式方式解析 NBT 数据。解析过程中通过访问器（Visitor）
/// 回调通知各个节点（数值、数组、字符串、列表、复合标签等），用户可通过自定义访
/// 问器控制解析流程（进入、跳过、停止）。处理NBT时无需一次性加载整个数据树到内存。
class NBT_Scanner
{
	/// @brief 禁止构造
	NBT_Scanner(void) = delete;
	/// @brief 禁止析构
	~NBT_Scanner(void) = delete;

protected:
	///@cond
	enum class Control : uint8_t
	{
		Continue,	///< 继续处理（继续迭代）
		Break,		///< 跳过剩余值（离开当前结构层级回到父层级）
		Stop,		///< 停止处理（终止解析）
		Error,		///< 出现错误（终止解析）
	};

	static Control ResultControlToControl(NBT_Visitor::ResultControl enResultControl)
	{
		switch (enResultControl)
		{
		case NBT_Visitor::ResultControl::Continue:	return Control::Continue;	break;
		case NBT_Visitor::ResultControl::Break:		return Control::Break;		break;
		case NBT_Visitor::ResultControl::Stop:		return Control::Stop;		break;
		default:									return Control::Error;		break;
		}
	}

protected:
	enum ErrCode : uint8_t
	{
		AllOk = 0,//没有问题

		UnknownError,//其他错误（代码问题）
		StdException,//标准异常（代码问题）
		UnknownControlCode,//未知的控制码（代码问题）
		ListElementTypeError,//列表元素类型错误（NBT文件问题）
		OutOfMemoryError,//内存不足错误（NBT文件问题）
		StackDepthExceeded,//调用栈深度过深（NBT文件or代码设置问题）
		NbtTypeTagError,//NBT标签类型错误（NBT文件问题）
		OutOfRangeError,//（NBT内部长度错误溢出）（NBT文件问题）

		ERRCODE_END,//结束标记，统计负数部分大小
	};

	constexpr static inline const char *const errReason[] =
	{
		"AllOk",
		
		"UnknownError",
		"StdException",
		"UnknownControlCode",
		"ListElementTypeError",
		"OutOfMemoryError",
		"StackDepthExceeded",
		"NbtTypeTagError",
		"OutOfRangeError",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == ERRCODE_END, "errReason array out sync");

	template <typename InputStream, typename Visitor, typename... Args>
	static ErrCode Error
	(
		const ErrCode errCode,
		const InputStream &tData,
		Visitor &tVisitor,
		const std::format_string<Args...> fmt,
		Args&&... args
	) noexcept
	{
		if (errCode >= ERRCODE_END)//保证errCode不会溢出
		{
			return errCode;
		}
		
		//打印错误原因
		NBT_Print_Level lvl = NBT_Print_Level::Err;
		tVisitor.VisitError(lvl, "Scan Err[{}]: {}\n", (uint8_t)errCode, errReason[errCode]);

		//打印扩展信息
		tVisitor.VisitError(lvl, "Extra Info: \"");
		tVisitor.VisitError(lvl, std::move(fmt), std::forward<Args>(args)...);
		tVisitor.VisitError(lvl, "\"\n\n");

		//如果可以，预览szCurrent前后n个字符，否则裁切到边界
#define VIEW_PRE (4 * 8 + 3)//向前
#define VIEW_SUF (4 * 8 + 5)//向后
		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : (0);//上边界裁切
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : (tData.Size());//下边界裁切
#undef VIEW_SUF
#undef VIEW_PRE
		//输出信息
		tVisitor.VisitError
		(
			lvl,
			"Data Review:\n"\
			"Current: 0x{:02X}({})\n"\
			"Data Size: 0x{:02X}({})\n"\
			"Data Range: [0x{:02X}({}),0x{:02X}({})):\n",

			(uint64_t)tData.Index(), tData.Index(),
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
					tVisitor.VisitError(lvl, "\n");
				}
				tVisitor.VisitError(lvl, "0x{:02X}: ", (uint64_t)i);
			}

			if (i != tData.Index())
			{
				tVisitor.VisitError(lvl, " {:02X} ", (uint8_t)tData[i]);
			}
			else//如果是当前出错字节，加方括号框起
			{
				tVisitor.VisitError(lvl, "[{:02X}]", (uint8_t)tData[i]);
			}
		}

		//输出提示信息
		tVisitor.VisitError(lvl, "\nSkip err data and return...\n\n");

		return errCode;
	}

#define _RP___FUNCTION__ __FUNCTION__//用于编译过程二次替换达到函数内部

#define _RP___LINE__ _RP_STRLING(__LINE__)
#define _RP_STRLING(l) STRLING(l)
#define STRLING(l) #l

#define STACK_TRACEBACK(fmt, ...) tVisitor.VisitError(NBT_Print_Level::Err, "In [{}] Line:[" _RP___LINE__ "]: \n" fmt "\n\n", _RP___FUNCTION__ __VA_OPT__(,) __VA_ARGS__);
#define CHECK_STACK_DEPTH(depth, ret) \
if((depth) == 0)\
{\
	Error(StackDepthExceeded, tData, tVisitor, "{}: NBT nesting depth exceeded maximum call stack limit", _RP___FUNCTION__);\
	STACK_TRACEBACK(#depth " == 0");\
	return ret;\
}

#define UNKNOWN_CONTROL_CODE(func, ret) \
{\
	Error(UnknownControlCode, tData, tVisitor, "Function [" #func "] return unknown control code");\
	STACK_TRACEBACK("ControlCode Test");\
	return ret;\
}

#define CALL_FUNC_RET_CONTROL(name, val) \
do\
{\
	Control controlRet = ResultControlToControl(val);\
	if (controlRet == Control::Error)\
	{\
		Error(UnknownControlCode, tData, tVisitor, "Function [" #name "] return unknown control code");\
		STACK_TRACEBACK("ControlCode Test");\
	}\
	return controlRet;\
}while(false)

#define MYTRY \
try\
{

#define MYCATCH(ret) \
}\
catch(const std::bad_alloc &e)\
{\
	Error(OutOfMemoryError, tData, tVisitor, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return ret;\
}\
catch(const std::exception &e)\
{\
	Error(StdException, tData, tVisitor, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return ret;\
}\
catch(...)\
{\
	Error(UnknownError, tData, tVisitor, "{}: Info:[Unknown Exception]", _RP___FUNCTION__);\
	STACK_TRACEBACK("catch(...)");\
	return ret;\
}

	template<bool bNoCheck = false, typename T, typename InputStream, typename Visitor>
	requires std::integral<T>
	static inline std::conditional_t<bNoCheck, void, bool> ReadBigEndian(InputStream &tData, T &tVal, Visitor &tVisitor) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				Error(OutOfRangeError, tData, tVisitor, "tData size [{}], current index [{}], remaining data size [{}], but try to read [{}]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData Test");
				return false;
			}
		}

		T BigEndianVal{};
		tData.GetRange((uint8_t *)&BigEndianVal, sizeof(BigEndianVal));
		tVal = NBT_Endian::BigToNativeAny(BigEndianVal);

		if constexpr (!bNoCheck)
		{
			return true;
		}
	}

	template<typename InputStream, typename Visitor>
	static bool GetName(InputStream &tData, NBT_Type::String &tName, Visitor &tVisitor) noexcept
	{
	MYTRY;
		//读取长度
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		if (!ReadBigEndian(tData, wStringLength, tVisitor))
		{
			STACK_TRACEBACK("wStringLength Read");
			return false;
		}

		using ValueType = NBT_Type::String::value_type;
		size_t szStringLength = (size_t)wStringLength;
		size_t szStringSize = szStringLength * sizeof(ValueType);

		//检查长度
		if (!tData.HasAvailData(szStringSize))
		{
			Error(OutOfRangeError, tData, tVisitor, "{}:\n(Index[{}] + szStringLength[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), szStringLength, tData.Index() + szStringLength, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return false;
		}

		//解析数据
		tName.reserve(szStringLength);//提前分配
		tName.assign((const ValueType *)tData.CurData(), szStringLength);//构造string（如果长度为0则构造0长字符串，合法行为）
		
		tData.AddIndex(szStringSize);//移动下标

		return true;
	MYCATCH(false);
	}

	template<typename InputStream, typename Visitor>
	static bool SkipName(InputStream &tData, Visitor &tVisitor) noexcept
	{
		//读取长度
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		if (!ReadBigEndian(tData, wStringLength, tVisitor))
		{
			STACK_TRACEBACK("wStringLength Read");
			return false;
		}

		size_t szSkipSize = (size_t)wStringLength * sizeof(NBT_Type::String::value_type);

		//检查长度
		if (!tData.HasAvailData(szSkipSize))
		{
			Error(OutOfRangeError, tData, tVisitor, "{}:\n(Index[{}] + szSkipSize[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), szSkipSize, tData.Index() + szSkipSize, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return false;
		}

		//跳过数据
		tData.AddIndex(szSkipSize);
		return true;
	}

	template<typename InputStream, typename Visitor>
	static Control ScanEndType(InputStream &tData, Visitor &tVisitor) noexcept
	{
	MYTRY;
		return ResultControlToControl(tVisitor.VisitListEnd());
	MYCATCH(Control::Error);
	}

	template<typename InputStream, typename Visitor>
	static bool SkipEndType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		//EndType无数据负载
		return true;
	}

	template<typename T, typename InputStream, typename Visitor>
	static Control ScanBuiltInType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//类型映射
		RAW_DATA_T tTmpRawData = 0;
		if (!ReadBigEndian(tData, tTmpRawData, tVisitor))
		{
			STACK_TRACEBACK("tTmpRawData Read");
			return Control::Error;
		}

	MYTRY;
		CALL_FUNC_RET_CONTROL(tVisitor.VisitNumericResult<T>, tVisitor.template VisitNumericResult<T>(std::bit_cast<T>(tTmpRawData)));
	MYCATCH(Control::Error);
	}

	template<typename T, typename InputStream, typename Visitor>
	static bool SkipBuiltInType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//类型映射
		size_t szSkipSize = sizeof(RAW_DATA_T);

		if (!tData.HasAvailData(szSkipSize))
		{
			Error(OutOfRangeError, tData, tVisitor, "{}:\n(Index[{}] + szSkipSize[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), szSkipSize, tData.Index() + szSkipSize, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return false;
		}

		//跳过数据
		tData.AddIndex(szSkipSize);
		return true;
	}

	template<typename T, typename InputStream, typename Visitor>
	static Control ScanArrayType(InputStream &tData, Visitor &tVisitor) noexcept
	{
	MYTRY;
		//获取4字节有符号数，代表数组元素个数
		NBT_Type::ArrayLength iArrayLength = 0;//4byte
		if (!ReadBigEndian(tData, iArrayLength, tVisitor))
		{
			STACK_TRACEBACK("iArrayLength Read");
			return Control::Error;
		}

		//检查有符号数大小范围
		if (iArrayLength < 0)
		{
			Error(OutOfRangeError, tData, tVisitor, ":\niArrayLength[{}] < 0", __FUNCTION__, iArrayLength);
			STACK_TRACEBACK("iArrayLength Test");
			return Control::Error;
		}

		//验证完成，类型转换
		using ValueType = typename T::value_type;
		size_t szArrayLength = (size_t)iArrayLength;
		size_t szArraySize = szArrayLength * sizeof(ValueType);

		//先进行合法性检查
		if (!tData.HasAvailData(szArraySize))
		{
			Error(OutOfRangeError, tData, tVisitor, "{}:\n(Index[{}] + szArraySize[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), szArrayLength, tData.Index() + szArraySize, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return Control::Error;
		}

		//数组保存
		T tArray{};
		tArray.reserve(szArrayLength);

		//读取dElementCount个元素
		for (size_t i = 0; i < szArrayLength; ++i)
		{
			ValueType tTmpData{};
			ReadBigEndian<true>(tData, tTmpData, tVisitor);//调用需要确保范围安全（已在前面检查）
			tArray.emplace_back(std::move(tTmpData));//读取一个插入一个
		}

		CALL_FUNC_RET_CONTROL(tVisitor.VisitArrayResult<T>, tVisitor.template VisitArrayResult<T>(std::move(tArray)));
	MYCATCH(Control::Error);
	}

	template<typename T, typename InputStream, typename Visitor>
	static bool SkipArrayType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		//获取4字节有符号数，代表数组元素个数
		NBT_Type::ArrayLength iArrayLength = 0;//4byte
		if (!ReadBigEndian(tData, iArrayLength, tVisitor))
		{
			STACK_TRACEBACK("iArrayLength Read");
			return false;
		}

		//检查有符号数大小范围
		if (iArrayLength < 0)
		{
			Error(OutOfRangeError, tData, tVisitor, ":\niArrayLength[{}] < 0", __FUNCTION__, iArrayLength);
			STACK_TRACEBACK("iArrayLength Test");
			return false;
		}

		size_t szSkipSize = (size_t)iArrayLength * sizeof(typename T::value_type);

		if (!tData.HasAvailData(szSkipSize))
		{
			Error(OutOfRangeError, tData, tVisitor, "{}:\n(Index[{}] + szSkipSize[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), szSkipSize, tData.Index() + szSkipSize, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return false;
		}

		tData.AddIndex(szSkipSize);//直接递增索引跳过
		return true;
	}

	template<typename InputStream, typename Visitor>
	static Control ScanStringType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		NBT_Type::String tString;
		if (!GetName(tData, tString, tVisitor))//转发调用
		{
			STACK_TRACEBACK("GetString");
			return Control::Error;
		}

	MYTRY;
		CALL_FUNC_RET_CONTROL(tVisitor.VisitStringResult, tVisitor.VisitStringResult(std::move(tString)));
	MYCATCH(Control::Error);
	}

	template<typename InputStream, typename Visitor>
	static bool SkipStringType(InputStream &tData, Visitor &tVisitor) noexcept
	{
		if (!SkipName(tData, tVisitor))//转发调用
		{
			STACK_TRACEBACK("SkipString");
			return false;
		}

		return true;
	}

	template<typename InputStream, typename Visitor>
	static Control ScanListType(InputStream &tData, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
	MYTRY;
		//栈深度检测
		CHECK_STACK_DEPTH(szStackDepth, Control::Error);

		//读取列表标签
		NBT_TAG_RAW_TYPE u8ListElementTag = 0;//b=byte
		if (!ReadBigEndian(tData, u8ListElementTag, tVisitor))
		{
			STACK_TRACEBACK("u8ListElementTag Read");
			return Control::Error;
		}

		//标签验证
		if (u8ListElementTag >= NBT_TAG::ENUM_END)
		{
			Error(NbtTypeTagError, tData, tVisitor, "{}:\nList NBT Type:Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
				(NBT_TAG_RAW_TYPE)u8ListElementTag, (NBT_TAG_RAW_TYPE)u8ListElementTag);
			STACK_TRACEBACK("u8ListElementTag Test");
			return Control::Error;
		}

		//类型转换
		NBT_TAG enListElementTag = (NBT_TAG)u8ListElementTag;

		//读取列表长度
		NBT_Type::ListLength iListLength = 0;//4byte
		if (!ReadBigEndian(tData, iListLength, tVisitor))
		{
			STACK_TRACEBACK("iListLength Read");
			return Control::Error;
		}

		//验证
		if (iListLength < 0)
		{
			Error(OutOfRangeError, tData, tVisitor, ":\niListLength[{}] < 0", __FUNCTION__, iListLength);
			STACK_TRACEBACK("iListLength Test");
			return Control::Error;
		}

		//类型转换
		size_t szListLength = (size_t)iListLength;

		//大小&类型判断
		if (enListElementTag == NBT_TAG::End && szListLength != 0)
		{
			Error(ListElementTypeError, tData, tVisitor, "{}:\nThe list with TAG_End[0x00] tag must be empty, but [{}] elements were found", __FUNCTION__,
				szListLength);
			STACK_TRACEBACK("enListElementTag And szListLength Test");
			return Control::Error;
		}

		//类型设置
		if (szListLength == 0 && enListElementTag != NBT_TAG::End)
		{
			enListElementTag = NBT_TAG::End;
		}

		//遍历索引
		size_t i = 0;//声明在外面，用于跳过剩余的起始位置

		//访问器开始回调
		switch (tVisitor.VisitListBegin(enListElementTag, szListLength))
		{
		case NBT_Visitor::ResultControl::Continue:	/*继续（什么也不做）*/	break;
		case NBT_Visitor::ResultControl::Break:		goto skip_any;			break;//这时候索引从0开始，跳过所有
		case NBT_Visitor::ResultControl::Stop:		return Control::Stop;	break;
		default:
			UNKNOWN_CONTROL_CODE(tVisitor.VisitListBegin, Control::Error);
			break;
		}

		//遍历读取
		for (; i < szListLength; ++i)
		{
			//访问器元素回调
			switch (tVisitor.VisitListElementBegin(enListElementTag, i))
			{
			case NBT_Visitor::NestingControl::Enter:	/*进入值（什么也不做）*/	break;
			case NBT_Visitor::NestingControl::Skip:		//跳过当前元素
				{
					if (!SkipSwitch(tData, enListElementTag, tVisitor, szStackDepth - 1))
					{
						STACK_TRACEBACK("SkipSwitch Error, Size: [{}] Index: [{}]", szListLength, i);
						return Control::Error;
					}
					continue;//跳过当前并继续
				}
				break;
			case NBT_Visitor::NestingControl::Break:	goto skip_any;			break;//跳过剩余所有列表元素，注意当前元素还未读取，所以下面依旧从i开始跳
			case NBT_Visitor::NestingControl::Stop:		return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitListElementBegin, Control::Error);
				break;
			}

			//元素递归访问
			switch (ScanSwitch(tData, enListElementTag, tVisitor, szStackDepth - 1))
			{
			case Control::Continue:	/*继续（什么也不做）*/	break;
			case Control::Break:	/*跳过（什么也不做）*/	break;//（从内部跳过之后传出的标签不具有传递性）
			case Control::Stop:		return Control::Stop;	break;
			case Control::Error:
				STACK_TRACEBACK("ScanSwitch Error, Size: [{}] Index: [{}]", szListLength, i);
				return Control::Error;
				break;
			default:
				UNKNOWN_CONTROL_CODE(ScanSwitch, Control::Error);
				break;
			}

			//元素结束回调
			switch (tVisitor.VisitListElementEnd(enListElementTag, i))
			{
			case NBT_Visitor::ResultControl::Continue:	/*继续（什么也不做）*/	break;
			case NBT_Visitor::ResultControl::Break:		goto skip_any;			break;//跳过剩余
			case NBT_Visitor::ResultControl::Stop:		return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitListElementEnd, Control::Error);
				break;
			}
		}

	skip_any://跳过剩余，如果没有则不跳过
		for (size_t j = i; j < szListLength; ++j)//从当前i开始跳到结束
		{
			if (!SkipSwitch(tData, enListElementTag, tVisitor, szStackDepth - 1))
			{
				STACK_TRACEBACK("SkipSwitch Error, Size: [{}] Index: [{}]", szListLength, j);
				return Control::Error;
			}
		}

		//返回控制标签
		CALL_FUNC_RET_CONTROL(tVisitor.VisitListEnd, tVisitor.VisitListEnd());
	MYCATCH(Control::Error);
	}

	template<typename InputStream, typename Visitor>
	static bool SkipListType(InputStream &tData, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
		//栈深度检测
		CHECK_STACK_DEPTH(szStackDepth, false);

		NBT_TAG_RAW_TYPE u8ListElementTag = 0;//b=byte
		if (!ReadBigEndian(tData, u8ListElementTag, tVisitor))
		{
			STACK_TRACEBACK("u8ListElementTag Read");
			return false;
		}

		if (u8ListElementTag >= NBT_TAG::ENUM_END)
		{
			Error(NbtTypeTagError, tData, tVisitor, "{}:\nList NBT Type:Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
				(NBT_TAG_RAW_TYPE)u8ListElementTag, (NBT_TAG_RAW_TYPE)u8ListElementTag);
			STACK_TRACEBACK("u8ListElementTag Test");
			return false;
		}

		NBT_TAG enListElementTag = (NBT_TAG)u8ListElementTag;

		NBT_Type::ListLength iListLength = 0;//4byte
		if (!ReadBigEndian(tData, iListLength, tVisitor))
		{
			STACK_TRACEBACK("iListLength Read");
			return false;
		}

		if (iListLength < 0)
		{
			Error(OutOfRangeError, tData, tVisitor, ":\niListLength[{}] < 0", __FUNCTION__, iListLength);
			STACK_TRACEBACK("iListLength Test");
			return false;
		}

		size_t szListLength = (size_t)iListLength;

		if (enListElementTag == NBT_TAG::End && szListLength != 0)
		{
			Error(ListElementTypeError, tData, tVisitor, "{}:\nThe list with TAG_End[0x00] tag must be empty, but [{}] elements were found", __FUNCTION__,
				szListLength);
			STACK_TRACEBACK("enListElementTag And szListLength Test");
			return false;
		}

		if (szListLength == 0 && enListElementTag != NBT_TAG::End)
		{
			enListElementTag = NBT_TAG::End;
		}

		size_t szSkipLength = szListLength;

		for (size_t i = 0; i < szSkipLength; ++i)
		{
			if (!SkipSwitch(tData, enListElementTag, tVisitor, szStackDepth - 1))
			{
				STACK_TRACEBACK("SkipSwitch Error, Size: [{}] Index: [{}]", szListLength, i);
				return false;
			}
		}

		return true;
	}

	template<bool bRoot, typename InputStream, typename Visitor>
	static Control ScanCompoundType(InputStream &tData, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
	MYTRY;
		//栈深度检测
		CHECK_STACK_DEPTH(szStackDepth, Control::Error);

		if constexpr (!bRoot)//非根部才进行compound调用
		{
			switch (tVisitor.VisitCompoundBegin())
			{
			case NBT_Visitor::ResultControl::Continue:	/*继续（什么也不做）*/	break;
			case NBT_Visitor::ResultControl::Break:		goto skip_any;			break;//跳过所有
			case NBT_Visitor::ResultControl::Stop:		return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitCompoundBegin, Control::Error);
				break;
			}
		}
		else//根部调用访问器起始回调
		{
			tVisitor.VisitBegin();
		}

		//循环直到内部退出
		while (true)
		{
			//处理末尾情况
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))
			{
				if constexpr (!bRoot)//非根部情况遇到末尾，则报错
				{
					Error(OutOfRangeError, tData, tVisitor, "{}:\nIndex[{}] >= DataSize()[{}]", __FUNCTION__,
						tData.Index(), tData.Size());
					STACK_TRACEBACK("HasAvailData Test");
					return Control::Error;
				}
				else//根部遇到末尾，正常结束
				{
					return Control::Stop;
				}
			}

			//先读取一下类型
			NBT_TAG_RAW_TYPE u8CompoundEntryTag = (NBT_TAG_RAW_TYPE)tData.GetNext();
			if (u8CompoundEntryTag == NBT_TAG::End)//处理End情况
			{
				goto end_return;//遇到end tag，正常离开
			}

			//范围验证
			if (u8CompoundEntryTag >= NBT_TAG::ENUM_END)
			{
				Error(NbtTypeTagError, tData, tVisitor, "{}:\nNBT Tag switch default: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					u8CompoundEntryTag, u8CompoundEntryTag);
				STACK_TRACEBACK("u8CompoundEntryTag Test");
				return Control::Error;
			}

			//验证完成，类型转换
			NBT_TAG enCompoundEntryTag = (NBT_TAG)u8CompoundEntryTag;

			//访问器条目回调（仅类型）
			switch (tVisitor.VisitCompoundNextEntryType(enCompoundEntryTag))
			{
			case NBT_Visitor::NestingControl::Enter:	/*进入（什么也不做）*/	break;
			case NBT_Visitor::NestingControl::Skip:		//跳过一个
				{
					//类型已被读取
					//跳过名称
					if (!SkipName(tData, tVisitor))
					{
						STACK_TRACEBACK("SkipName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}

					//跳过数据
					if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
					{
						STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}
				}
				break;
			case NBT_Visitor::NestingControl::Break:	//跳过所有（离开）
				{
					//类型已被读取
					//跳过名称
					if (!SkipName(tData, tVisitor))
					{
						STACK_TRACEBACK("SkipName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}

					//跳过数据
					if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
					{
						STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}

					//前面跳过当前剩余未完成的部分，然后到尾部跳过所有
					goto skip_any;
				}
				break;
			case NBT_Visitor::NestingControl::Stop:	return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitCompoundNextEntryType, Control::Error);
				break;
			}

			//读取名称
			NBT_Type::String sName{};
			if (!GetName(tData, sName, tVisitor))
			{
				STACK_TRACEBACK("GetName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return Control::Error;
			}

			//调用访问器（类型名称）
			switch (tVisitor.VisitCompoundEntryBegin(enCompoundEntryTag, std::move(sName)))
			{
			case NBT_Visitor::NestingControl::Enter:	/*进入（什么也不做）*/	break;
			case NBT_Visitor::NestingControl::Skip:		//跳过一个
				{
					//类型、名称已被读取
					//跳过数据
					if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
					{
						STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}
					continue;//继续上面的循环
				}
				break;
			case NBT_Visitor::NestingControl::Break://跳过剩余
				{
					//类型、名称已被读取
					//跳过数据
					if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))//先跳过当前
					{
						STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
						return Control::Error;
					}

					//前面跳过当前剩余未完成的部分，然后到尾部跳过所有
					goto skip_any;
				}
				break;
			case NBT_Visitor::NestingControl::Stop:	return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitCompoundEntryBegin, Control::Error);
				break;
			}

			//元素递归访问
			switch (ScanSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
			{
			case Control::Continue:	/*继续（什么也不做）*/	break;
			case Control::Break:	/*跳过（什么也不做）*/	break;//（从内部跳过之后传出的标签不具有传递性）
			case Control::Stop:		return Control::Stop;	break;
			case Control::Error:
				STACK_TRACEBACK("ScanSwitch Error, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return Control::Error;
				break;
			default:
				UNKNOWN_CONTROL_CODE(ScanSwitch, Control::Error);
				break;
			}

			//元素访问结束回调
			switch (tVisitor.VisitCompoundEntryEnd(enCompoundEntryTag, std::move(sName)))
			{
			case NBT_Visitor::ResultControl::Continue:	/*继续（什么也不做）*/	break;
			case NBT_Visitor::ResultControl::Break:		goto skip_any;			break;//跳过剩余所有
			case NBT_Visitor::ResultControl::Stop:		return Control::Stop;	break;
			default:
				UNKNOWN_CONTROL_CODE(tVisitor.VisitCompoundEntryEnd, Control::Error);
				break;
			}
		}

	skip_any://跳过剩下所有数据
		while (true)
		{
			//跳过剩余的compound值
			//处理末尾情况
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))
			{
				if constexpr (!bRoot)//非根部情况遇到末尾，则报错
				{
					Error(OutOfRangeError, tData, tVisitor, "{}:\nIndex[{}] >= DataSize()[{}]", __FUNCTION__,
						tData.Index(), tData.Size());
					STACK_TRACEBACK("HasAvailData Test");
					return Control::Error;
				}
				else
				{
					return Control::Stop;//结束
				}
			}

			NBT_TAG_RAW_TYPE u8CompoundEntryTag = (NBT_TAG_RAW_TYPE)tData.GetNext();

			if (u8CompoundEntryTag == NBT_TAG::End)
			{
				goto end_return;//遇到end tag，正常离开
			}

			if (u8CompoundEntryTag >= NBT_TAG::ENUM_END)
			{
				Error(NbtTypeTagError, tData, tVisitor, "{}:\nNBT Tag switch default: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					u8CompoundEntryTag, u8CompoundEntryTag);
				STACK_TRACEBACK("u8CompoundEntryTag Test");
				return Control::Error;
			}

			NBT_TAG enCompoundEntryTag = (NBT_TAG)u8CompoundEntryTag;

			if (!SkipName(tData, tVisitor))
			{
				STACK_TRACEBACK("SkipName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return Control::Error;
			}

			if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
			{
				STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return Control::Error;
			}
		}

	end_return://结束返回
		if constexpr (!bRoot)
		{
			CALL_FUNC_RET_CONTROL(tVisitor.VisitCompoundEnd, tVisitor.VisitCompoundEnd());//返回
		}
		else
		{
			tVisitor.VisitEnd();//根部调用结束
			return Control::Stop;//根部直接返回
		}
	MYCATCH(Control::Error);
	}

	template<typename InputStream, typename Visitor>
	static bool SkipCompoundType(InputStream &tData, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
		//栈深度检测
		CHECK_STACK_DEPTH(szStackDepth, false);

		while (true)
		{
			//跳过compound值
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))//处理末尾情况
			{
				Error(OutOfRangeError, tData, tVisitor, "{}:\nIndex[{}] >= DataSize()[{}]", __FUNCTION__,
					tData.Index(), tData.Size());
				STACK_TRACEBACK("HasAvailData Test");
				return false;//跳过必然不可能是根部调用
			}

			NBT_TAG_RAW_TYPE u8CompoundEntryTag = (NBT_TAG_RAW_TYPE)tData.GetNext();

			if (u8CompoundEntryTag == NBT_TAG::End)
			{
				return true;//正常结束
			}

			if (u8CompoundEntryTag >= NBT_TAG::ENUM_END)
			{
				Error(NbtTypeTagError, tData, tVisitor, "{}:\nNBT Tag switch default: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					u8CompoundEntryTag, u8CompoundEntryTag);
				STACK_TRACEBACK("u8CompoundEntryTag Test");
				return false;
			}

			NBT_TAG enCompoundEntryTag = (NBT_TAG)u8CompoundEntryTag;

			if (!SkipName(tData, tVisitor))
			{
				STACK_TRACEBACK("SkipName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return false;
			}

			if (!SkipSwitch(tData, enCompoundEntryTag, tVisitor, szStackDepth - 1))
			{
				STACK_TRACEBACK("SkipSwitch Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(enCompoundEntryTag));
				return false;
			}
		}

		return true;
	}

	template<typename InputStream, typename Visitor>
	static Control ScanSwitch(InputStream &tData, NBT_TAG tagNbt, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
		Control retControl;
		switch (tagNbt)
		{
		case NBT_TAG::End:
			{
				retControl = ScanEndType(tData, tVisitor);
			}
			break;
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				retControl = ScanBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				retControl = ScanArrayType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::String:
			{
				retControl = ScanStringType(tData, tVisitor);
			}
			break;
		case NBT_TAG::List:
			{
				retControl = ScanListType(tData, tVisitor, szStackDepth);
			}
			break;
		case NBT_TAG::Compound:
			{
				retControl = ScanCompoundType<false>(tData, tVisitor, szStackDepth);
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				retControl = ScanArrayType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				retControl = ScanArrayType<CurType>(tData, tVisitor);
			}
			break;
		default:
			{
				Error(NbtTypeTagError, tData, tVisitor, "{}:\nNBT Tag switch error: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
				retControl = Control::Error;
			}
			break;
		}

		if (retControl == Control::Error)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x{:02X}({})] read error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return retControl;
	}

	template<typename InputStream, typename Visitor>
	static bool SkipSwitch(InputStream &tData, NBT_TAG tagNbt, Visitor &tVisitor, size_t szStackDepth) noexcept
	{
		bool bRet = false;
		switch (tagNbt)
		{
		case NBT_TAG::End:
			{
				bRet = SkipEndType(tData, tVisitor);
			}
			break;
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				bRet = SkipBuiltInType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				bRet = SkipArrayType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::String:
			{
				bRet = SkipStringType(tData, tVisitor);
			}
			break;
		case NBT_TAG::List:
			{
				bRet = SkipListType(tData, tVisitor, szStackDepth);
			}
			break;
		case NBT_TAG::Compound:
			{
				bRet = SkipCompoundType(tData, tVisitor, szStackDepth);
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				bRet = SkipArrayType<CurType>(tData, tVisitor);
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				bRet = SkipArrayType<CurType>(tData, tVisitor);
			}
			break;
		default:
			{
				Error(NbtTypeTagError, tData, tVisitor, "{}:\nNBT Tag switch error: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
				bRet = false;
			}
			break;
		}

		if (!bRet)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x{:02X}({})] read error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return bRet;
	}

///@endcond
public:
	/// @brief 从输入流中扫描NBT数据，并通过访问器回调处理每个节点
	/// @tparam InputStream 输入流类型，必须符合DefaultInputStream类型的接口
	/// @tparam Visitor 访问器类型，必须符合IsLookLike_NBT_Visitor概念
	/// @param IptStream 输入流对象
	/// @param tVisitor 访问器对象，用于处理扫描过程中遇到的NBT数据节点
	/// @param szStackDepth 递归最大深度，防止栈溢出
	/// @return 扫描成功返回true，失败返回false
	/// @note 函数通过访问器回调的方式遍历整个NBT结构，不会构建完整的内存树，适合处理大型NBT数据。
	/// 若遇到格式错误或超过深度限制，函数将返回false并停止扫描。
	template<typename InputStream, typename Visitor>
	requires(IsLookLike_NBT_Visitor<Visitor>)
	static bool ScanNBT(InputStream &IptStream, Visitor &tVisitor, size_t szStackDepth = 512) noexcept
	{
		return ScanCompoundType<true>(IptStream, tVisitor, szStackDepth) != Control::Error;
	}
	
	/// @brief 从数据容器中扫描NBT数据，并通过访问器回调处理每个节点
	/// @tparam DataType 数据容器类型，默认为std::vector<uint8_t>
	/// @tparam Visitor 访问器类型，必须符合IsLookLike_NBT_Visitor概念
	/// @param tDataInput 输入数据容器
	/// @param szStartIdx 数据起始索引，会忽略容器中前szStartIdx字节的数据
	/// @param tVisitor 访问器对象，用于处理扫描过程中遇到的NBT数据节点
	/// @param szStackDepth 递归最大深度，防止栈溢出
	/// @return 扫描成功返回true，失败返回false
	/// @note 此函数是ScanNBT(InputStream)版本的数据容器适配版本，其它行为请参考ScanNBT(InputStream)版本的说明。
	template<typename DataType = std::vector<uint8_t>, typename Visitor>
	requires(IsLookLike_NBT_Visitor<Visitor>)
	static bool ScanNBT(const DataType &tDataInput, size_t szStartIdx, Visitor &tVisitor, size_t szStackDepth = 512) noexcept
	{
		NBT_IO::DefaultInputStream<DataType> IptStream(tDataInput, szStartIdx);
		return ScanCompoundType<true>(IptStream, tVisitor, szStackDepth) != Control::Error;
	}

#undef MYTRY
#undef MYCATCH
#undef CALL_FUNC_RET_CONTROL
#undef UNKNOWN_CONTROL_CODE
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCTION__
};
