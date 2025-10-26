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
#include <algorithm>//std::sort

#include "NBT_Print.hpp"//打印输出
#include "NBT_Node.hpp"//nbt类型
#include "NBT_Endian.hpp"//字节序

class NBT_Writer
{
	NBT_Writer(void) = delete;
	~NBT_Writer(void) = delete;

public:
	template <typename T = std::vector<uint8_t>>
	class MyOutputStream
	{
	private:
		T &tData;

	public:
		using StreamType = T;
		using ValueType = typename T::value_type;
		static_assert(sizeof(ValueType) == 1, "Error ValueType Size");
		static_assert(std::is_trivially_copyable_v<ValueType>, "ValueType Must Be Trivially Copyable");

		//引用天生无法使用临时值构造，无需担心临时值构造导致的悬空引用
		MyOutputStream(T &_tData, size_t szStartIdx = 0) :tData(_tData)
		{
			tData.resize(szStartIdx);
		}
		~MyOutputStream(void) = default;

		const ValueType &operator[](size_t szIndex) const noexcept
		{
			return tData[szIndex];
		}

		template<typename V>
		requires(std::is_constructible_v<ValueType, V &&>)
			void PutOnce(V &&c)
		{
			tData.push_back(std::forward<V>(c));
		}

		void PutRange(const ValueType *pData, size_t szSize)
		{
			//tData.insert(tData.end(), &pData[0], &pData[szSize]);

			//使用更高效的实现，而不是迭代器写入
			size_t szCurSize = tData.size();
			tData.resize(szCurSize + szSize);
			memcpy(&tData.data()[szCurSize], &pData[0], szSize);
		}

		void AddReserve(size_t szAddSize)
		{
			tData.reserve(tData.size() + szAddSize);
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
	template <typename T, typename OutputStream, typename ErrInfoFunc, typename... Args>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> Error
		(
			const T &code,
			const OutputStream &tData,
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

		//如果可以，预览szCurrent前n个字符，否则裁切到边界
#define VIEW_PRE (8 * 8 + 8)//向前
		size_t rangeBeg = (tData.Size() > VIEW_PRE) ? (tData.Size() - VIEW_PRE) : (0);//上边界裁切
		size_t rangeEnd = tData.Size();//下边界裁切
#undef VIEW_PRE
		//输出信息
		funcErrInfo
		(
			"Data Review:\n"\
			"Data Size: 0x{:02X}({})\n"\
			"Data Range: [0x{:02X}({}),0x{:02X}({})):\n",

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

			funcErrInfo(" {:02X} ", (uint8_t)tData[i]);
		}

		//输出提示信息
		if constexpr (std::is_same_v<T, ErrCode>)
		{
			funcErrInfo("\nSkip err and return...\n\n");
		}
		else if constexpr (std::is_same_v<T, WarnCode>)
		{
			funcErrInfo("\nSkip warn and continue...\n\n");
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

	template<typename OutputStream, typename ErrInfoFunc>
	static inline ErrCode CheckReserve(OutputStream &tData, size_t szAddSize, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		tData.AddReserve(szAddSize);
		return AllOk;
	MYCATCH;
	}

	//写出大端序值
	template<typename T, typename OutputStream, typename ErrInfoFunc>
	requires std::integral<T>
	static inline ErrCode WriteBigEndian(OutputStream &tData, const T &tVal, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		auto BigEndianVal = NBT_Endian::NativeToBigAny(tVal);
		tData.PutRange((const uint8_t *)&BigEndianVal, sizeof(BigEndianVal));
		return AllOk;
	MYCATCH;
	}

	template<typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutName(OutputStream &tData, const NBT_Type::String &sName, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;

		//获取string长度
		size_t szStringLength = sName.size();

		//检查大小是否符合上限
		if (szStringLength > (size_t)NBT_Type::StringLength_Max)
		{
			eRet = Error(StringTooLongError, tData, funcErrInfo, "{}:\nszStringLength[{}] > StringLength_Max[{}]", __FUNCTION__,
				szStringLength, (size_t)NBT_Type::StringLength_Max);
			STACK_TRACEBACK("szStringLength Test");
			return eRet;
		}

		//输出名称长度
		NBT_Type::StringLength wNameLength = (uint16_t)szStringLength;
		eRet = WriteBigEndian(tData, wNameLength, funcErrInfo);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("wNameLength Write");
			return eRet;
		}

		//输出名称
		eRet = CheckReserve(tData, szStringLength * sizeof(sName[0]), funcErrInfo);//提前分配
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("CheckReserve Fail, Check Size: [{}]", szStringLength * sizeof(sName[0]));
			return eRet;
		}
		//范围写入
		tData.PutRange((const typename OutputStream::ValueType *)sName.data(), szStringLength);

		return eRet;
	MYCATCH;
	}

	template<typename T, typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutbuiltInType(OutputStream &tData, const T &tBuiltIn, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		//获取原始类型，然后转换到raw类型准备写出
		using RAW_DATA_T = NBT_Type::BuiltinRawType_T<T>;//原始类型映射
		RAW_DATA_T tTmpRawData = std::bit_cast<RAW_DATA_T>(tBuiltIn);

		eRet = WriteBigEndian(tData, tTmpRawData, funcErrInfo);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("tTmpRawData Write");
			return eRet;
		}

		return eRet;
	}

	template<typename T, typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutArrayType(OutputStream &tData, const T &tArray, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		//获取数组大小判断是否超过要求上限
		//也就是4字节有符号整数上限
		size_t szArrayLength = tArray.size();
		if (szArrayLength > (size_t)NBT_Type::ArrayLength_Max)
		{
			eRet = Error(ArrayTooLongError, tData, funcErrInfo, "{}:\nszArrayLength[{}] > ArrayLength_Max[{}]", __FUNCTION__,
				szArrayLength, (size_t)NBT_Type::ArrayLength_Max);
			STACK_TRACEBACK("szArrayLength Test");
			return eRet;
		}

		//获取实际写出大小
		NBT_Type::ArrayLength iArrayLength = (NBT_Type::ArrayLength)szArrayLength;
		eRet = WriteBigEndian(tData, iArrayLength, funcErrInfo);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("iArrayLength Write");
			return eRet;
		}

		//写出元素
		eRet = CheckReserve(tData, iArrayLength * sizeof(tArray[0]), funcErrInfo);//提前分配
		if (eRet != AllOk)
		{
			STACK_TRACEBACK("CheckReserve Fail, Check Size: [{}]", iArrayLength * sizeof(tArray[0]));
			return eRet;
		}

		for (NBT_Type::ArrayLength i = 0; i < iArrayLength; ++i)
		{
			eRet = WriteBigEndian(tData, tArray[i], funcErrInfo);
			if (eRet < AllOk)
			{
				STACK_TRACEBACK("tTmpData Write");
				return eRet;
			}
		}

		return eRet;
	}

	//如果是非根部，不会输出额外的Compound_End
	template<bool bRoot, bool bSortCompound, typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutCompoundType(OutputStream &tData, const NBT_Type::Compound &tCompound, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept
	{
	MYTRY;
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);
		
		using IterableRangeType = typename std::conditional_t<bSortCompound, std::vector<NBT_Type::Compound::const_iterator>, const NBT_Type::Compound &>;

		//通过模板bSortCompound指定是否执行排序输出（nbt中仅compound是无序结构）
		IterableRangeType tmpIterableRange =//注意此处如果内部抛出异常，返回空vector的情况下还有可能二次异常，所以外部还需另一个try catch
		[&](void) noexcept -> IterableRangeType
		{
			if constexpr (bSortCompound)
			{
				try//筛掉标准库异常
				{
					IterableRangeType vSort{};
					vSort.reserve(tCompound.size());//提前扩容

					//插入迭代器
					for (auto it = tCompound.cbegin(), end = tCompound.cend(); it != end; ++it)
					{
						vSort.push_back(it);
					}

					//进行排序
					std::sort(vSort.begin(), vSort.end(),
						[](const auto &l, const auto &r) -> bool
						{
							return l->first < r->first;
						}
					);
					return vSort;
				}
				catch (const std::bad_alloc &e)
				{
					eRet = Error(OutOfMemoryError, tData, funcErrInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());
					STACK_TRACEBACK("catch(std::bad_alloc)");
					return {};
				}
				catch (const std::exception &e)
				{
					eRet = Error(StdException, tData, funcErrInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());
					STACK_TRACEBACK("catch(std::exception)");
					return {};
				}
				catch (...)
				{
					eRet = Error(UnknownError, tData, funcErrInfo, "{}: Info:[Unknown Exception]", _RP___FUNCTION__);
					STACK_TRACEBACK("catch(...)");
					return {};
				}
			}
			else
			{
				return tCompound;
			}
		}();

		//判断错误码是否被设置
		if constexpr (bSortCompound)
		{
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("Lambda: GetIterableRange Error!");
				return eRet;
			}
		}

		//注意compound是为数不多的没有元素数量限制的结构
		//此处无需检查大小，且无需写出大小
		for (const auto &it: tmpIterableRange)
		{
			const auto &[sName, nodeNbt] = [&](void) -> const auto &
			{
				if constexpr (bSortCompound)
				{
					return *it;
				}
				else
				{
					return it;
				}
			}();

			NBT_TAG curTag = nodeNbt.GetTag();

			//集合中如果存在nbt end类型的元素，删除而不输出
			if (curTag == NBT_TAG::End)
			{
				//End元素被忽略警告（警告不返回错误码）
				Error(EndElementIgnoreWarn, tData, funcErrInfo, "{}:\nName: \"{}\", type is [NBT_Type::End], ignored!", __FUNCTION__,
					sName.ToCharTypeUTF8());
				continue;
			}

			//先写出tag
			eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)curTag, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("curTag Write");
				return eRet;
			}

			//然后写出name
			eRet = PutName(tData, sName, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutName Fail, Type: [NBT_Type::{}]", NBT_Type::GetTypeName(curTag));
				return eRet;
			}

			//最后根据tag类型写出数据
			eRet = PutSwitch<bSortCompound>(tData, nodeNbt, curTag, szStackDepth - 1, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Fail, Name: \"{}\", Type: [NBT_Type::{}]",
					sName.ToCharTypeUTF8(), NBT_Type::GetTypeName(curTag));
				return eRet;
			}
		}

		if constexpr (!bRoot)
		{
			//注意Compound类型有一个NBT_TAG::End结尾，如果写出错误则在前面返回，不放置结尾
			eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)NBT_TAG::End, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("NBT_TAG::End[0x00(0)] Write");
				return eRet;
			}
		}

		return eRet;
	MYCATCH;
	}

	template<typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutStringType(OutputStream &tData, const NBT_Type::String &tString, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		eRet = PutName(tData, tString, funcErrInfo);//借用PutName实现，因为string走的name相同操作
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("PutString");//因为是借用实现，所以这里小小的改个名，防止报错Name误导人
			return eRet;
		}

		return eRet;
	}

	template<bool bSortCompound, typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutListType(OutputStream &tData, const NBT_Type::List &tList, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;
		CHECK_STACK_DEPTH(szStackDepth);

		//检查
		size_t szListLength = tList.size();
		if (szListLength > (size_t)NBT_Type::ListLength_Max)//大于的情况下强制赋值会导致严重问题，只能返回错误
		{
			eRet = Error(ListTooLongError, tData, funcErrInfo, "{}:\nszListLength[{}] > ListLength_Max[{}]", __FUNCTION__,
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
			eRet = Error(ListElementTypeError, tData, funcErrInfo, "{}:\nThe list with TAG_End[0x00] tag must be empty, but [{}] elements were found", __FUNCTION__,
				iListLength);
			STACK_TRACEBACK("iListLength And enListValueTag Test");
			return eRet;
		}

		//获取列表标签，如果列表长度为0，则强制改为空标签
		NBT_TAG enListElementTag = iListLength == 0 ? NBT_TAG::End : enListValueTag;

		//写出标签
		eRet = WriteBigEndian(tData, (NBT_TAG_RAW_TYPE)enListElementTag, funcErrInfo);
		if (eRet < AllOk)
		{
			STACK_TRACEBACK("enListElementTag Write");
			return eRet;
		}

		//写出长度
		eRet = WriteBigEndian(tData, iListLength, funcErrInfo);
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
				eRet = Error(ListElementTypeError, tData, funcErrInfo, "{}:\nExpected type [NBT_Type::{}][0x{:02X}({})] in list, but found type [NBT_Type::{}][0x{:02X}({})]", __FUNCTION__,
				NBT_Type::GetTypeName(enListElementTag), (NBT_TAG_RAW_TYPE)enListElementTag, (NBT_TAG_RAW_TYPE)enListElementTag,
				NBT_Type::GetTypeName(curTag), (NBT_TAG_RAW_TYPE)curTag, (NBT_TAG_RAW_TYPE)curTag);
				STACK_TRACEBACK("curTag Test");
				return eRet;
			}

			//一致，很好，那么输出
			//列表无名字，无需重复tag，只需输出数据
			eRet = PutSwitch<bSortCompound>(tData, tmpNode, enListElementTag, szStackDepth - 1, funcErrInfo);
			if (eRet != AllOk)
			{
				STACK_TRACEBACK("PutSwitch Error, Size: [{}] Index: [{}]", iListLength, i);
				return eRet;
			}
		}

		return eRet;
	}

	template<bool bSortCompound, typename OutputStream, typename ErrInfoFunc>
	static ErrCode PutSwitch(OutputStream &tData, const NBT_Node &nodeNbt, NBT_TAG tagNbt, size_t szStackDepth, ErrInfoFunc &funcErrInfo) noexcept
	{
		ErrCode eRet = AllOk;

		switch (tagNbt)
		{
		case NBT_TAG::Byte:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Byte>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Short:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Short>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Int:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Int>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Long:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Long>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Float:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Float>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::Double:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Double>;
				eRet = PutbuiltInType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::ByteArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::ByteArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::String://类型唯一，非模板函数
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::String>;
				eRet = PutStringType(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::List://可能递归，需要处理szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::List>;
				eRet = PutListType<bSortCompound>(tData, nodeNbt.GetData<CurType>(), szStackDepth, funcErrInfo);
			}
			break;
		case NBT_TAG::Compound://可能递归，需要处理szStackDepth
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::Compound>;
				eRet = PutCompoundType<false, bSortCompound>(tData, nodeNbt.GetData<CurType>(), szStackDepth, funcErrInfo);
			}
			break;
		case NBT_TAG::IntArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::IntArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::LongArray:
			{
				using CurType = NBT_Type::TagToType_T<NBT_TAG::LongArray>;
				eRet = PutArrayType<CurType>(tData, nodeNbt.GetData<CurType>(), funcErrInfo);
			}
			break;
		case NBT_TAG::End://注意end标签绝对不可以进来
			{
				eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nNBT Tag switch error: Unexpected Type Tag NBT_TAG::End[0x00(0)]", __FUNCTION__);
			}
			break;
		default://数据出错
			{
				eRet = Error(NbtTypeTagError, tData, funcErrInfo, "{}:\nNBT Tag switch error: Unknown Type Tag[0x{:02X}({})]", __FUNCTION__,
					(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);//此处不进行提前返回，往后默认返回处理
			}
			break;
		}

		if (eRet != AllOk)//如果出错，打一下栈回溯
		{
			STACK_TRACEBACK("Tag[0x{:02X}({})] write error!",
				(NBT_TAG_RAW_TYPE)tagNbt, (NBT_TAG_RAW_TYPE)tagNbt);
		}

		return eRet;
	}

public:
	//输出到tData中，部分功能和原理参照ReadNBT处的注释，szDataStartIndex在此处可以对一个tData通过不同的tCompound和szDataStartIndex = tData.size()
	//来调用以达到把多个不同的nbt输出到同一个tData内的功能
	template<bool bSortCompound = true, typename OutputStream, typename ErrInfoFunc = NBT_Print>
	static bool WriteNBT(OutputStream OptStream, const NBT_Type::Compound &tCompound, size_t szStackDepth = 512, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept
	{
		//输出最大栈深度
		//printf("Max Stack Depth [%zu]\n", szStackDepth);

		//开始递归输出
		return PutCompoundType<true, bSortCompound>(OptStream, tCompound, szStackDepth, funcErrInfo) == AllOk;
	}

	template<bool bSortCompound = true, typename DataType = std::vector<uint8_t>, typename ErrInfoFunc = NBT_Print>
	static bool WriteNBT(DataType &tDataOutput, size_t szStartIdx, const NBT_Type::Compound &tCompound, size_t szStackDepth = 512, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept
	{
		return WriteNBT<bSortCompound>(MyOutputStream<DataType>(tDataOutput, szStartIdx), tCompound, szStackDepth, std::move(funcErrInfo));
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