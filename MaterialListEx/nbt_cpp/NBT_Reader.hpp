#pragma once

#include <new>//std::bad_alloc
#include <bit>//std::bit_cast
#include <vector>//字节流
#include <stdint.h>//类型定义
#include <stddef.h>//size_t
#include <stdlib.h>//byte swap
#include <string.h>//memcpy
#include <utility>//std::move
#include <type_traits>//类型约束

#include "NBT_Print.hpp"//打印输出
#include "NBT_Node.hpp"//nbt类型
#include "NBT_Endian.hpp"//字节序

class NBT_Reader
{
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

public:
	template <typename T = std::vector<uint8_t>>
	class MyInputStream
	{
	private:
		const T &tData = {};
		size_t szIndex = 0;

	public:
		using StreamType = T;
		using ValueType = typename T::value_type;
		static_assert(sizeof(ValueType) == 1, "Error ValueType Size");
		static_assert(std::is_trivially_copyable_v<ValueType>, "ValueType Must Be Trivially Copyable");

		//禁止用户使用临时值构造
		MyInputStream(const T &&_tData, size_t szStartIdx = 0) = delete;
		MyInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
		{}
		~MyInputStream(void) = default;//默认析构

		MyInputStream(const MyInputStream &) = delete;
		MyInputStream(MyInputStream &&) = delete;

		MyInputStream &operator=(const MyInputStream &) = delete;
		MyInputStream &operator=(MyInputStream &&) = delete;

		const ValueType &operator[](size_t szIndex) const noexcept
		{
			return tData[szIndex];
		}

		const ValueType &GetNext() noexcept
		{
			return tData[szIndex++];
		}

		void GetRange(void *pDest, size_t szSize) noexcept
		{
			memcpy(pDest, &tData[szIndex], szSize);
			szIndex += szSize;
		}

		void UnGet() noexcept
		{
			if (szIndex != 0)
			{
				--szIndex;
			}
		}

		const ValueType *CurData() const noexcept
		{
			return &(tData[szIndex]);
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

		const ValueType *BaseData() const noexcept
		{
			return tData.data();
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

private:
	enum ErrCode : uint8_t
	{
		AllOk = 0,//没有问题

		UnknownError,//其他错误
		StdException,//标准异常
		OutOfMemoryError,//内存不足错误（NBT文件问题）
		ListElementTypeError,//列表元素类型错误（NBT文件问题）
		StackDepthExceeded,//调用栈深度过深（NBT文件or代码设置问题）
		NbtTypeTagError,//NBT标签类型错误（NBT文件问题）
		OutOfRangeError,//（NBT内部长度错误溢出）（NBT文件问题）
		InternalTypeError,//变体NBT节点类型错误（代码问题）

		ERRCODE_END,//结束标记，统计负数部分大小
	};

	constexpr static inline const char *const errReason[] =
	{
		"AllOk",

		"UnknownError",
		"StdException",
		"OutOfMemoryError",
		"ListElementTypeError",
		"StackDepthExceeded",
		"NbtTypeTagError",
		"OutOfRangeError",
		"InternalTypeError",
	};

	//记得同步数组！
	static_assert(sizeof(errReason) / sizeof(errReason[0]) == ERRCODE_END, "errReason array out sync");

	enum WarnCode : uint8_t
	{
		NoWarn = 0,

		ElementExistsWarn,

		WARNCODE_END,
	};

	constexpr static inline const char *const warnReason[] =//正常数组，直接用WarnCode访问
	{
		"NoWarn",

		"ElementExistsWarn",
	};

	//记得同步数组！
	static_assert(sizeof(warnReason) / sizeof(warnReason[0]) == WARNCODE_END, "warnReason array out sync");

	//error处理
	//使用变参形参表+vprintf代理复杂输出，给出更多扩展信息
	//主动检查引发的错误，主动调用eRet = Error报告，然后触发STACK_TRACEBACK，最后返回eRet到上一级
	//上一级返回的错误通过if (eRet != AllOk)判断的，直接触发STACK_TRACEBACK后返回eRet到上一级
	//如果是警告值，则不返回值
	template <typename T, typename InputStream, typename ErrInfoFunc, typename... Args>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> Error
		(
			const T &code,
			const InputStream &tData,
			ErrInfoFunc &funcErrInfo,
			const std::FMT_STR<Args...> fmt,
			Args&&... args
		) noexcept
	{
		//打印错误原因
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			if (code >= ERRCODE_END)
			{
				return code;
			}
			//上方if保证code不会溢出
			funcErrInfo("Read Err[{}]: {}\n", (uint8_t)code, errReason[code]);
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			if (code >= WARNCODE_END)
			{
				return;
			}
			//上方if保证code不会溢出
			funcErrInfo("Read Warn[{}]: {}\n", (uint8_t)code, warnReason[code]);
		}
		else
		{
			static_assert(false, "Unknown [T code] Type!");
		}

		//打印扩展信息
		funcErrInfo("Extra Info: \"");
		funcErrInfo(std::move(fmt), std::forward<Args>(args)...);
		funcErrInfo("\"\n\n");

		//如果可以，预览szCurrent前后n个字符，否则裁切到边界
#define VIEW_PRE (4 * 8 + 3)//向前
#define VIEW_SUF (4 * 8 + 5)//向后
		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : (0);//上边界裁切
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : (tData.Size());//下边界裁切
#undef VIEW_SUF
#undef VIEW_PRE
		//输出信息
		funcErrInfo
		(
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
					funcErrInfo("\n");
				}
				funcErrInfo("0x{:02X}: ", (uint64_t)i);
			}

			if (i != tData.Index())
			{
				funcErrInfo(" {:02X} ", (uint8_t)tData[i]);
			}
			else//如果是当前出错字节，加方括号框起
			{
				funcErrInfo("[{:02X}]", (uint8_t)tData[i]);
			}
		}

		//输出提示信息
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			funcErrInfo("\nSkip err data and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			funcErrInfo("\nSkip warn data and continue...\n\n");
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

#define _RP___FUNCTION__ __FUNCTION__//用于编译过程二次替换达到函数内部

#define _RP___LINE__ _RP_STRLING(__LINE__)
#define _RP_STRLING(l) STRLING(l)
#define STRLING(l) #l

#define STACK_TRACEBACK(fmt, ...) funcErrInfo("In [{}] Line:[" _RP___LINE__ "]: \n" fmt "\n\n", _RP___FUNCTION__ __VA_OPT__(,) __VA_ARGS__);
#define CHECK_STACK_DEPTH(Depth) \
if((Depth) <= 0)\
{\
	eRet = Error(StackDepthExceeded, tData, funcErrInfo, "{}: NBT nesting depth exceeded maximum call stack limit", _RP___FUNCTION__);\
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
	ErrCode eRet = Error(OutOfMemoryError, tData, funcErrInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return eRet;\
}\
catch(const std::exception &e)\
{\
	ErrCode eRet = Error(StdException, tData, funcErrInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return eRet;\
}\
catch(...)\
{\
	ErrCode eRet =  Error(UnknownError, tData, funcErrInfo, "{}: Info:[Unknown Exception]", _RP___FUNCTION__);\
	STACK_TRACEBACK("catch(...)");\
	return eRet;\
}

	//读取大端序数值，bNoCheck为true则不进行任何检查
	template<bool bNoCheck = false, typename T, typename InputStream, typename ErrInfoFunc>
	requires std::integral<T>
	static inline std::conditional_t<bNoCheck, void, ErrCode> ReadBigEndian(InputStream &tData, T &tVal, ErrInfoFunc &funcErrInfo) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				ErrCode eRet = Error(OutOfRangeError, tData, funcErrInfo, "tData size [{}], current index [{}], remaining data size [{}], but try to read [{}]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData Test");
				return eRet;
			}
		}

		T BigEndianVal{};
		tData.GetRange((uint8_t *)&BigEndianVal, sizeof(BigEndianVal));
		tVal = NBT_Endian::BigToNativeAny(BigEndianVal);

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	template<typename InputStream, typename ErrInfoFunc>
	static ErrCode GetName(InputStream &tData, NBT_Type::String &tName, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		//读取2字节的无符号名称长度
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		eRet = ReadBigEndian(tData, wStringLength, funcErrInfo);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("wStringLength Read");
			return eRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(wStringLength))
		{
			ErrCode eRet = Error(OutOfRangeError, tData, funcErrInfo, "{}:\n(Index[{}] + wStringLength[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), (size_t)wStringLength, tData.Index() + (size_t)wStringLength, tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return eRet;
		}

		
		//解析出名称
		tName.reserve(wStringLength);//提前分配
		tName.assign((const NBT_Type::String::value_type *)tData.CurData(), wStringLength);//构造string（如果长度为0则构造0长字符串，合法行为）
		tData.AddIndex(wStringLength);//移动下标

		return eRet;
	MYCATCH;
	}

	template<typename T, typename InputStream, typename ErrInfoFunc>
	static ErrCode GetBuiltInType(InputStream &tData, T &tBuiltIn, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		//读取数据
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//类型映射

		//临时存储，因为可能存在跨类型转换
		RAW_DATA_T tTmpRawData = 0;
		eRet = ReadBigEndian(tData, tTmpRawData, funcErrInfo);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Read");
			return eRet;
		}

		//转换并返回
		tBuiltIn = std::move(std::bit_cast<T>(tTmpRawData));
		return eRet;
	}

	template<typename T, typename InputStream, typename ErrInfoFunc>
	static ErrCode GetArrayType(InputStream &tData, T &tArray, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//获取4字节有符号数，代表数组元素个数
		NBT_Type::ArrayLength iElementCount = 0;//4byte
		eRet = ReadBigEndian(tData, iElementCount, funcErrInfo);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iElementCount Read");
			return eRet;
		}

		using ValueType = typename T::value_type;

		//判断长度是否超过
		if (!tData.HasAvailData(iElementCount * sizeof(ValueType)))//保证下方调用安全
		{
			eRet = Error(OutOfRangeError, tData, funcErrInfo, "{}:\n(Index[{}] + iElementCount[{}] * sizeof(T::value_type)[{}])[{}] > DataSize[{}]", __FUNCTION__,
				tData.Index(), (size_t)iElementCount, sizeof(ValueType), tData.Index() + (size_t)iElementCount * sizeof(typename T::value_type), tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return eRet;
		}
		
		//数组保存
		tArray.reserve(iElementCount);//提前扩容
		//读取dElementCount个元素
		for (NBT_Type::ArrayLength i = 0; i < iElementCount; ++i)
		{
			ValueType tTmpData{};
			ReadBigEndian<true>(tData, tTmpData, funcErrInfo);//调用需要确保范围安全
			tArray.emplace_back(std::move(tTmpData));//读取一个插入一个
		}

		return eRet;
	MYCATCH;
	}

	//如果是非根部，有额外检测
	template<bool bRoot, typename InputStream, typename ErrInfoFunc>
	static ErrCode GetCompoundType(InputStream &tData, NBT_Type::Compound &tCompound, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//读取
		while (true)
		{
			//处理末尾情况
			if (!tData.HasAvailData(sizeof(NBT_TAG_RAW_TYPE)))
			{
				if constexpr (!bRoot)//非根部情况遇到末尾，则报错
				{
					eRet = Error(OutOfRangeError, tData, funcErrInfo, "{}:\nIndex[{}] >= DataSize()[{}]", __FUNCTION__,
						tData.Index(), tData.Size());
				}

				return eRet;//否则直接返回（默认值AllOk）
			}

			//先读取一下类型
			NBT_TAG tagNbt = (NBT_TAG)(NBT_TAG_RAW_TYPE)tData.GetNext();
			if (tagNbt == NBT_TAG::End)//处理End情况
			{
				return eRet;//直接返回（默认值AllOk）
			}

			if (tagNbt >= NBT_TAG::ENUM_END)//确认在范围内
			{
				eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nNBT Tag switch default: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
				STACK_TRACEBACK("tagNbt Test");
				return eRet;//超出范围立刻返回
			}

			//然后读取名称
			NBT_Type::String sName{};
			eRet = GetName(tData, sName, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(tagNbt));
				return eRet;//名称读取失败立刻返回
			}

			//然后根据类型，调用对应的类型读取并返回到tmpNode
			NBT_Node tmpNode{};
			eRet = GetSwitch(tData, tmpNode, tagNbt, szStackDepth - 1, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetSwitch Fail, Name: \"{}\", Type: [NBT_Type::{}]", sName.ToCharTypeUTF8(), NBT_Type::GetTypeName(tagNbt));
				//return eRet;//注意此处不返回，进行插入，以便分析错误之前的正确数据
			}

			//sName:tmpNode，插入当前调用栈深度的根节点
			//根据实际mc java代码得出，如果插入一个已经存在的键，会导致原先的值被替换并丢弃
			//那么在失败后，手动从迭代器替换当前值，注意，此处必须是try_emplace，因为try_emplace失败后原先的值
			//tmpNode不会被移动导致丢失，所以也无需拷贝插入以防止移动丢失问题
			auto [it, bSuccess] = tCompound.try_emplace(std::move(sName), std::move(tmpNode));
			if (!bSuccess)
			{
				//使用当前值替换掉阻止插入的原始值
				it->second = std::move(tmpNode);

				//发出警告，注意警告不用eRet接返回值
				Error(ElementExistsWarn, tData, funcErrInfo, "{}:\nName: \"{}\", Type: [NBT_Type::{}] data already exist!", __FUNCTION__,
					sName.ToCharTypeUTF8(), NBT_Type::GetTypeName(tagNbt));
			}

			//最后判断是否出错
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("While break with an error!");
				return eRet;//出错返回
			}
		}

		return eRet;//返回错误码
	MYCATCH;
	}

	template<typename InputStream, typename ErrInfoFunc>
	static ErrCode GetStringType(InputStream &tData, NBT_Type::String &tString, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		//读取字符串
		eRet = GetName(tData, tString, funcErrInfo);//因为string与name读取原理一致，直接借用实现
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("GetString");//因为是借用实现，所以这里小小的改个名，防止报错Name误导人
			return eRet;
		}

		return eRet;
	}

	template<typename InputStream, typename ErrInfoFunc>
	static ErrCode GetListType(InputStream &tData, NBT_Type::List &tList, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//读取1字节的列表元素类型
		NBT_TAG_RAW_TYPE enListElementTag = 0;//b=byte
		eRet = ReadBigEndian(tData, enListElementTag, funcErrInfo);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("enListElementTag Read");
			return eRet;
		}

		//错误的列表元素类型
		if (enListElementTag >= NBT_TAG::ENUM_END)
		{
			eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nList NBT Type:Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
				(NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag);
			STACK_TRACEBACK("enListElementTag Test");
			return eRet;
		}

		//读取4字节的有符号列表长度
		NBT_Type::ListLength iListLength = 0;//4byte
		eRet = ReadBigEndian(tData, iListLength, funcErrInfo);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iListLength Read");
			return eRet;
		}

		//检查有符号数大小范围
		if (iListLength < 0)
		{
			eRet = Error(OutOfRangeError, tData, funcErrInfo, ":\niListLength[{}] < 0", __FUNCTION__, iListLength);
			STACK_TRACEBACK("iListLength Test");
			return eRet;
		}

		//防止重复N个结束标签，带有结束标签的必须是空列表
		if (enListElementTag == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, funcErrInfo, "{}:\nThe list with TAG_End[0x00] tag must be empty, but [{}] elements were found", __FUNCTION__,
				iListLength);
			STACK_TRACEBACK("enListElementTag And iListLength Test");
			return eRet;
		}

		//确保如果长度为0的情况下，列表类型必为End
		if (iListLength == 0 && enListElementTag != NBT_TAG::End)
		{
			enListElementTag = (NBT_TAG_RAW_TYPE)NBT_TAG::End;
		}

		//设置类型并提前扩容
		tList.enElementTag = (NBT_TAG)enListElementTag;//先设置类型
		tList.reserve(iListLength);//已知大小提前分配减少开销

		//根据元素类型，读取n次列表
		for (NBT_Type::ListLength i = 0; i < iListLength; ++i)
		{
			NBT_Node tmpNode{};//列表元素会直接赋值修改
			eRet = GetSwitch(tData, tmpNode, (NBT_TAG)enListElementTag, szStackDepth - 1, funcErrInfo);
			if (eRet != AllOk)//错误处理
			{
				STACK_TRACEBACK("GetSwitch Error, Size: [{}] Index: [{}]", iListLength, i);
				return eRet;
			}

			//每读取一个往后插入一个
			tList.emplace_back(std::move(tmpNode));
		}
		
		return eRet;
	MYCATCH;
	}

	//这个函数拦截所有内部调用产生的异常并处理返回，所以此函数绝对不抛出异常，由此调用此函数的函数也可无需catch异常
	template<typename InputStream, typename ErrInfoFunc>
	static ErrCode GetSwitch(InputStream &tData, NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept//选择函数不检查递归层，由函数调用的函数检查
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::String:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				nodeNbt.emplace<CurType>();
				eRet = GetStringType(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				nodeNbt.emplace<CurType>();
				eRet = GetListType(tData, nodeNbt.GetData<CurType>(), szStackDepth, funcErrInfo);//选择函数不减少递归层
			}
			break;
		case NBT_TAG::Compound://需要递归调用
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				nodeNbt.emplace<CurType>();
				eRet = GetCompoundType<false>(tData, nodeNbt.GetData<CurType>(), szStackDepth, funcErrInfo);//选择函数不减少递归层
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::End://不应该在任何时候遇到此标签，Compound会读取到并消耗掉，不会传入，List遇到此标签不会调用读取，所以遇到即为错误
			{
				eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nNBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]", __FUNCTION__);
			}
			break;
		default://其它未知标签，如NBT内标数据签错误
			{
				eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nNBT Tag switch error: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}
		
		if (eRet != AllOk)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x{:02X}({})] read error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return eRet;//传递返回值
	}

public:
	/*
	备注：此函数读取nbt时，会创建一个默认根，然后把nbt内所有数据集合到此默认根上，
	也就是哪怕按照mojang的nbt标准，默认根是无名Compound，也会被挂接到返回值里的
	NBT_Type::Compound中。遍历函数返回的NBT_Type::Compound即可得到所有NBT数据，
	这么做的目的是为了方便读写例程且不用在某些地方部分实现mojang的无名compound的
	特殊处理，这种情况下可以在一定程度上甚至比mojang标准支持更多的NBT文件情况，
	比如文件内并不是Compound开始的，而是单纯的几个不同类型且带有名字的NBT，那么也能
	正常读取到并全部挂在NBT_Type::Compound中，就好像nbt文件本身就是一个大的无名
	NBT_Type::Compound一样，相对的，写出函数也能支持写出此种情况，所以写出函数
	WriteNBT在写出的时候，传入的值也是一个内含NBT_Type::Compound的NBT_Node，
	然后传入的NBT_Type::Compound本身不会被以任何形式写入NBT文件，而是内部数据，
	也就是挂接在下面的内容会被写入，这样既能保证兼容mojang的nbt文件，也能一定程度上
	扩展nbt文件内可以存储的内容（允许nbt文件直接存储多个键值对而不是必须先挂在一个
	无名称的Compound下）
	*/

	//szStackDepth 控制栈深度，递归层检查仅由可嵌套的可能进行递归的函数进行，栈深度递减仅由对选择函数的调用进行
	//注意此函数不会清空tCompound，所以可以对一个tCompound通过不同的tData多次调用来读取多个nbt片段并合并到一起
	//如果指定了szDataStartIndex则会忽略tData中长度为szDataStartIndex的数据
	template<typename InputStream, typename ErrInfoFunc = NBT_Print>
	static bool ReadNBT(InputStream IptStream, NBT_Type::Compound &tCompound, size_t szStackDepth = 512, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept//从data中读取nbt
	{
		return GetCompoundType<true>(IptStream, tCompound, szStackDepth, funcErrInfo) == AllOk;//从data中获取nbt数据到nRoot中，只有此调用为根部调用（模板true），用于处理特殊情况
	}

	template<typename DataType = std::vector<uint8_t>, typename ErrInfoFunc = NBT_Print>
	static bool ReadNBT(const DataType &tDataInput, size_t szStartIdx, NBT_Type::Compound &tCompound, size_t szStackDepth = 512, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept//从data中读取nbt
	{
		MyInputStream<DataType> IptStream(tDataInput, szStartIdx);
		return GetCompoundType<true>(IptStream, tCompound, szStackDepth, funcErrInfo) == AllOk;
	}


#undef MYTRY
#undef MYCATCH
#undef CHECK_STACK_DEPTH
#undef STACK_TRACEBACK
#undef STRLING
#undef _RP_STRLING
#undef _RP___LINE__
#undef _RP___FUNCTION__
};