#pragma once

#include "NBT_Reader.hpp"//友元
#include "NBT_Visitor.hpp"//鸭子类
#include "NBT_Print.hpp"//打印输出
#include "NBT_IO.hpp"//IO流

class NBT_Scanner
{
	/// @brief 禁止构造
	NBT_Scanner(void) = delete;
	/// @brief 禁止析构
	~NBT_Scanner(void) = delete;

private:
///@cond
#define _RP___FUNCTION__ __FUNCTION__//用于编译过程二次替换达到函数内部

#define _RP___LINE__ _RP_STRLING(__LINE__)
#define _RP_STRLING(l) STRLING(l)
#define STRLING(l) #l

#define STACK_TRACEBACK(fmt, ...) funcInfo(NBT_Print_Level::Err, "In [{}] Line:[" _RP___LINE__ "]: \n" fmt "\n\n", _RP___FUNCTION__ __VA_OPT__(,) __VA_ARGS__);
#define CHECK_STACK_DEPTH(Depth) \
if((Depth) <= 0)\
{\
	eRet = Error(StackDepthExceeded, tData, funcInfo, "{}: NBT nesting depth exceeded maximum call stack limit", _RP___FUNCTION__);\
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
	ErrCode eRet = Error(OutOfMemoryError, tData, funcInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::bad_alloc)");\
	return eRet;\
}\
catch(const std::exception &e)\
{\
	ErrCode eRet = Error(StdException, tData, funcInfo, "{}: Info:[{}]", _RP___FUNCTION__, e.what());\
	STACK_TRACEBACK("catch(std::exception)");\
	return eRet;\
}\
catch(...)\
{\
	ErrCode eRet =  Error(UnknownError, tData, funcInfo, "{}: Info:[Unknown Exception]", _RP___FUNCTION__);\
	STACK_TRACEBACK("catch(...)");\
	return eRet;\
}
///@endcond

public:
	using ErrCode = NBT_Reader::ErrCode;
	using WarnCode = NBT_Reader::WarnCode;

	template <typename T, typename InputStream, typename InfoFunc, typename... Args>
	requires(std::is_same_v<T, ErrCode> || std::is_same_v<T, WarnCode>)
	static std::conditional_t<std::is_same_v<T, ErrCode>, ErrCode, void> Error
	(
		const T &code,
		const InputStream &tData,
		InfoFunc &funcInfo,
		const std::format_string<Args...> fmt,
		Args&&... args
	) noexcept
	{
		return NBT_Reader::Error(code, tData, funcInfo, std::move(fmt), std::forward<Args>(args)...);
	}


	//TODO
	







public:
	//返回值表示因为错误退出的，还是用正常退出的
	//true:正常退出   false:错误退出

	template<typename InputStream, typename Visitor, typename InfoFunc = NBT_Print>
	requires(IsLookLike_NBT_Visitor<Visitor>)
	static bool Scan(InputStream &IptStream, Visitor &tVisitor, size_t szStackDepth = 512, InfoFunc funcInfo = NBT_Print{})
	{
		tVisitor.VisitBegin();
		//TODO
		tVisitor.VisitEnd();
	}
	
	template<typename DataType = std::vector<uint8_t>, typename Visitor, typename InfoFunc = NBT_Print>
	requires(IsLookLike_NBT_Visitor<Visitor>)
	static bool Scan(const DataType &tDataInput, size_t szStartIdx, Visitor &tVisitor, size_t szStackDepth = 512, InfoFunc funcInfo = NBT_Print{})
	{
		NBT_IO::DefaultInputStream<DataType> IptStream(tDataInput, szStartIdx);

		tVisitor.VisitBegin();
		//TODO
		tVisitor.VisitEnd();
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
