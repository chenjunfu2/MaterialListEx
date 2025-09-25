#pragma once

#include <new>//std::bad_alloc
#include <bit>//std::bit_cast
#include <string>
#include <stdint.h>
#include <stdlib.h>//byte swap
#include <type_traits>
#include <stdarg.h>//va宏

#include "NBT_Node.hpp"

//请尽可能使用basic_string_view来减少开销
//因为string的[]运算符访问有坑，每次都会判断是否是优化模式导致问题，所以存储data指针直接访问以加速
//否则汇编里会出现一大堆
//cmp qword ptr [...],10h
//jb ...
//这种判断string是否是小于16的优化模式，来决定怎么访问string
template <typename T = std::basic_string_view<uint8_t>>
class MyInputStream
{
private:
	const T &tData = {};
	size_t szIndex = 0;
public:
	
	MyInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
	{}
	~MyInputStream(void) = default;//默认析构

	MyInputStream(const MyInputStream &) = delete;
	MyInputStream(MyInputStream &&) = delete;

	MyInputStream &operator=(const MyInputStream &) = delete;
	MyInputStream &operator=(MyInputStream &&) = delete;

	const typename T::value_type &operator[](size_t szIndex) const noexcept
	{
		return tData[szIndex];
	}

	typename T::value_type GetNext() noexcept
	{
		return tData[szIndex++];
	}

	void GetRange(void *pDest, size_t szSize) noexcept
	{
		std::memcpy(pDest, &tData[szIndex], szSize);
		szIndex += szSize;
	}

	void UnGet() noexcept
	{
		if (szIndex != 0)
		{
			--szIndex;
		}
	}

	const typename T::value_type *CurData() const noexcept
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

	const typename T::value_type *BaseData() const noexcept
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

template <typename DataType = std::basic_string_view<uint8_t>>
class NBT_Reader//请尽可能使用basic_string_view
{
	NBT_Reader(void) = delete;
	~NBT_Reader(void) = delete;

	using InputStream = MyInputStream<DataType>;//流类型
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
	template <typename T>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> _cdecl Error(const T &code, const InputStream &tData, _Printf_format_string_ const char *const cpExtraInfo = NULL, ...) noexcept//gcc使用__attribute__((format))，msvc使用_Printf_format_string_
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

		//如果可以，预览szCurrent前后n个字符，否则裁切到边界
#define VIEW_PRE (4 * 8 + 3)//向前
#define VIEW_SUF (4 * 8 + 5)//向后
		size_t rangeBeg = (tData.Index() > VIEW_PRE) ? (tData.Index() - VIEW_PRE) : (0);//上边界裁切
		size_t rangeEnd = ((tData.Index() + VIEW_SUF) < tData.Size()) ? (tData.Index() + VIEW_SUF) : (tData.Size());//下边界裁切
#undef VIEW_SUF
#undef VIEW_PRE
		//输出信息
		printf
		(
			"Data Review:\n"\
			"Current: 0x%02llX(%zu)\n"\
			"Data Size: 0x%02llX(%zu)\n"\
			"Data[0x%02llX(%zu),0x%02llX(%zu)):\n",

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

		//输出提示信息
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			printf("\nSkip err data and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			printf("\nSkip warn data and continue...\n\n");
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

	//读取大端序数值，bNoCheck为true则不进行任何检查
	template<bool bNoCheck = false, typename T>
	requires std::integral<T>
	static inline std::conditional_t<bNoCheck, void, ErrCode> ReadBigEndian(InputStream &tData, T &tVal) noexcept
	{
		if constexpr (!bNoCheck)
		{
			if (!tData.HasAvailData(sizeof(T)))
			{
				ErrCode eRet = Error(OutOfRangeError, tData, "tData size [%zu], current index [%zu], remaining data size [%zu], but try to read [%zu]",
					tData.Size(), tData.Index(), tData.Size() - tData.Index(), sizeof(T));
				STACK_TRACEBACK("HasAvailData Test");
				return eRet;
			}
		}

		if constexpr (sizeof(T) == sizeof(uint8_t))
		{
			tVal = (T)(uint8_t)tData.GetNext();
		}
		else if constexpr (sizeof(T) == sizeof(uint16_t))
		{
			uint16_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint16_t)_byteswap_ushort(tmp);
		}
		else if constexpr (sizeof(T) == sizeof(uint32_t))
		{
			uint32_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint32_t)_byteswap_ulong(tmp);
		}
		else if constexpr (sizeof(T) == sizeof(uint64_t))
		{
			uint64_t tmp{};
			tData.GetRange((uint8_t *)&tmp, sizeof(tmp));
			tVal = (T)(uint64_t)_byteswap_uint64(tmp);
		}
		else//other
		{
			//统一到无符号类型
			using UT = typename std::make_unsigned<T>::type;
			static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

			//缓冲区，读取
			uint8_t u8BigEndianBuffer[sizeof(T)] = { 0 };
			tData.GetRange(u8BigEndianBuffer, sizeof(u8BigEndianBuffer));

			//静态展开（替代for）
			[&] <size_t... Is>(std::index_sequence<Is...>) -> void
			{
				UT tmpVal = 0;//临时量缓存，避免编译器一直生成引用访问造成开销
				((tmpVal |= ((UT)u8BigEndianBuffer[Is]) << (8 * (sizeof(T) - Is - 1))), ...);//模板展开位操作

				//读取完成赋值给tVal
				tVal = (T)tmpVal;
			}(std::make_index_sequence<sizeof(T)>{});

			//与上面操作等价但性能较低的写法，保留以用于增加可读性
			//for (size_t i = sizeof(T); i > 0; --i)
			//{
			//	tVal |= ((UT)(uint8_t)tData.GetNext()) << (8 * (i - 1));//每次移动刚才提取的地位到高位，然后继续提取
			//}
		}

		if constexpr (!bNoCheck)
		{
			return AllOk;
		}
	}

	static ErrCode GetName(InputStream &tData, NBT_Type::String &tName) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		//读取2字节的无符号名称长度
		NBT_Type::StringLength wStringLength = 0;//w->word=2*byte
		eRet = ReadBigEndian(tData, wStringLength);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("wStringLength Read");
			return eRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(wStringLength))
		{
			ErrCode eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + wStringLength[%zu])[%zu] > DataSize[%zu]",
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

	template<typename T>
	static ErrCode GetBuiltInType(InputStream &tData, T &tBuiltIn) noexcept
	{
		ErrCode eRet = AllOk;

		//读取数据
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//类型映射

		//临时存储，因为可能存在跨类型转换
		RAW_DATA_T tTmpRawData = 0;
		eRet = ReadBigEndian(tData, tTmpRawData);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Read");
			return eRet;
		}

		//转换并返回
		tBuiltIn = std::move(std::bit_cast<T>(tTmpRawData));
		return eRet;
	}

	template<typename T>
	static ErrCode GetArrayType(InputStream &tData, T &tArray) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//获取4字节有符号数，代表数组元素个数
		NBT_Type::ArrayLength iElementCount = 0;//4byte
		eRet = ReadBigEndian(tData, iElementCount);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iElementCount Read");
			return eRet;
		}

		//判断长度是否超过
		if (!tData.HasAvailData(iElementCount * sizeof(T::value_type)))//保证下方调用安全
		{
			eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": (Index[%zu] + iElementCount[%zu] * sizeof(T::value_type)[%zu])[%zu] > DataSize[%zu]", 
				tData.Index(), (size_t)iElementCount, sizeof(T::value_type), tData.Index() + (size_t)iElementCount * sizeof(T::value_type), tData.Size());
			STACK_TRACEBACK("HasAvailData Test");
			return eRet;
		}
		
		//数组保存
		tArray.reserve(iElementCount);//提前扩容
		//读取dElementCount个元素
		for (NBT_Type::ArrayLength i = 0; i < iElementCount; ++i)
		{
			typename T::value_type tTmpData{};
			ReadBigEndian<true>(tData, tTmpData);//调用需要确保范围安全
			tArray.emplace_back(std::move(tTmpData));//读取一个插入一个
		}

		return eRet;
	MYCATCH;
	}

	//如果是非根部，有额外检测
	template<bool bRoot>
	static ErrCode GetCompoundType(InputStream &tData, NBT_Type::Compound &tCompound, size_t szStackDepth) noexcept
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
					eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": Index[%zu] >= DataSize()[%zu]", tData.Index(), tData.Size());
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
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch default: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
				STACK_TRACEBACK("tagNbt Test");
				return eRet;//超出范围立刻返回
			}

			//然后读取名称
			NBT_Type::String sName{};
			eRet = GetName(tData, sName);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetName Fail, Type: [NBT_Type::%s]", NBT_Type::GetTypeName(tagNbt));
				return eRet;//名称读取失败立刻返回
			}

			//然后根据类型，调用对应的类型读取并返回到tmpNode
			NBT_Node tmpNode{};
			eRet = GetSwitch(tData, tmpNode, tagNbt, szStackDepth - 1);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("GetSwitch Fail, Name: \"%s\", Type: [NBT_Type::%s]", sName.ToCharTypeUTF8().c_str(), NBT_Type::GetTypeName(tagNbt));
				//return eRet;//注意此处不返回，进行插入，以便分析错误之前的正确数据
			}

			//sName:tmpNode，插入当前调用栈深度的根节点
			//根据实际mc java代码得出，如果插入一个已经存在的键，会导致原先的值被替换并丢弃
			//那么在失败后，手动从迭代器替换当前值，注意，此处必须是try_emplace，因为try_emplace失败后原先的值
			//tmpNode不会被移动导致丢失，所以也无需拷贝插入以防止移动丢失问题
			auto [it,bSuccess] = tCompound.try_emplace(std::move(sName), std::move(tmpNode));
			if (!bSuccess)
			{
				//使用当前值替换掉阻止插入的原始值
				it->second = std::move(tmpNode);

				//发出警告，注意警告不用eRet接返回值
				Error(ElementExistsWarn, tData, __FUNCSIG__ ": Name: \"%s\", Type: [NBT_Type::%s] data already exist!",
					sName.ToCharTypeUTF8().c_str(), NBT_Type::GetTypeName(tagNbt));
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

	static ErrCode GetStringType(InputStream &tData, NBT_Type::String &tString) noexcept
	{
		ErrCode eRet = AllOk;

		//读取字符串
		eRet = GetName(tData, tString);//因为string与name读取原理一致，直接借用实现
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("GetString");//因为是借用实现，所以这里小小的改个名，防止报错Name误导人
			return eRet;
		}

		return eRet;
	}

	static ErrCode GetListType(InputStream &tData, NBT_Type::List &tList, size_t szStackDepth) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//读取1字节的列表元素类型
		NBT_TAG_RAW_TYPE enListElementTag = 0;//b=byte
		eRet = ReadBigEndian(tData, enListElementTag);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("enListElementTag Read");
			return eRet;
		}

		//错误的列表元素类型
		if (enListElementTag >= NBT_TAG::ENUM_END)
		{
			eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": List NBT Type:Unknown Type Tag[0x%02X(%d)]",
				(NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag);
			STACK_TRACEBACK("enListElementTag Test");
			return eRet;
		}

		//读取4字节的有符号列表长度
		NBT_Type::ListLength iListLength = 0;//4byte
		eRet = ReadBigEndian(tData, iListLength);
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("iListLength Read");
			return eRet;
		}

		//检查有符号数大小范围
		if (iListLength < 0)
		{
			eRet = Error(OutOfRangeError, tData, __FUNCSIG__ ": iListLength[%d] < 0", iListLength);
			STACK_TRACEBACK("iListLength Test");
			return eRet;
		}

		//防止重复N个结束标签，带有结束标签的必须是空列表
		if (enListElementTag == NBT_TAG::End && iListLength != 0)
		{
			eRet = Error(ListElementTypeError, tData, __FUNCSIG__ ": The list with TAG_End[0x00] tag must be empty, but [%d] elements were found",
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
			eRet = GetSwitch(tData, tmpNode, (NBT_TAG)enListElementTag, szStackDepth - 1);
			if (eRet != AllOk)//错误处理
			{
				STACK_TRACEBACK("GetSwitch Error, Size: [%d] Index: [%d]", iListLength, i);
				return eRet;
			}

			//每读取一个往后插入一个
			tList.emplace_back(std::move(tmpNode));
		}
		
		return eRet;
	MYCATCH;
	}

	//这个函数拦截所有内部调用产生的异常并处理返回，所以此函数绝对不抛出异常，由此调用此函数的函数也可无需catch异常
	static ErrCode GetSwitch(InputStream &tData, NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth) noexcept//选择函数不检查递归层，由函数调用的函数检查
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				nodeNbt.emplace<CurType>();
				eRet = GetBuiltInType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::String:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				nodeNbt.emplace<CurType>();
				eRet = GetStringType(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::List://需要递归调用，列表开头给出标签ID和长度，后续都为一系列同类型标签的有效负载（无标签 ID 或名称）
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				nodeNbt.emplace<CurType>();
				eRet = GetListType(tData, nodeNbt.GetData<CurType>(), szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_TAG::Compound://需要递归调用
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				nodeNbt.emplace<CurType>();
				eRet = GetCompoundType<false>(tData, nodeNbt.GetData<CurType>(), szStackDepth);//选择函数不减少递归层
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				nodeNbt.emplace<CurType>();
				eRet = GetArrayType<CurType>(tData, nodeNbt.GetData<CurType>());
			}
			break;
		case NBT_TAG::End://不应该在任何时候遇到此标签，Compound会读取到并消耗掉，不会传入，List遇到此标签不会调用读取，所以遇到即为错误
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]");
			}
			break;
		default://其它未知标签，如NBT内标数据签错误
			{
				eRet = Error(NbtTypeTagError, tData, __FUNCSIG__ ": NBT Tag switch error: Unknown Type Tag[0x%02X(%d)]",
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}
		
		if (eRet != AllOk)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x%02X(%d)] read error!",
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
	static bool ReadNBT(NBT_Type::Compound &tCompound, const DataType &tData, size_t szDataStartIndex = 0, size_t szStackDepth = 512) noexcept//从data中读取nbt
	{
	MYTRY;
		//初始化数据流对象
		InputStream IptStream(tData, szDataStartIndex);

		//输出最大栈深度
		//printf("Max Stack Depth [%zu]\n", szStackDepth);

		//开始递归读取
		return GetCompoundType<true>(IptStream, tCompound, szStackDepth) == AllOk;//从data中获取nbt数据到nRoot中，只有此调用为根部调用（模板true），用于处理特殊情况
	MYCATCH;
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