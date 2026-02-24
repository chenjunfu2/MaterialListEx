#pragma once

#include <format>
#include <stdio.h>

/// @file
/// @brief 用于处理NBT信息打印的默认实现

/// @brief 用于指示信息打印等级的枚举
enum class NBT_Print_Level
{
	Info,		///< 普通信息
	Warn,		///< 警告信息
	Err,		///< 错误信息
};


/// @brief 一个用于打印信息到指定的C文件对象的工具类，作为库内大部分存在信息输出接口的默认实现。
/// 实际可被使用此类为默认值参数的函数的调用方，以类似此类的仿函数参数重写的其它类型替换，
/// 比如调用方实现了一个My_Print，只要重载了仿函数调用运算符，且参数与此类的仿函数调用运算符一致，
/// 则可以直接替换此类并传递给目标参数。
/// @note 输出格式里的fmt为std::format格式
class NBT_Print
{
/// @brief 用于MSVC低版本的std::format_string支持。
/// @details 在版本过低的时候切换为msvc内部实现。
/// 其它编译器与版本使用标准库实现。
#if defined(_MSC_VER) && _MSC_VER < 1935 //旧版本MSVC 1935-不支持
#define FMT_STR _Fmt_string //使用MSVC库内部类型
#else
#define FMT_STR format_string //MSVC 19.35+、GCC、Clang 等使用标准库版本
#endif

private:
	FILE *pfOutputInfo = NULL;
	FILE *pfOutputWarn = NULL;
	FILE *pfOutputErr = NULL;

public:
	/// @brief 通过c文件对象构造
	/// @param _pfOutputInfo 普通信息输出的C文件对象（通常为stdout，可以为NULL）
	/// @param _pfOutputWarn 警告信息输出的C文件对象（通常为stderr，可以为NULL）
	/// @param _pfOutputErr 错误信息输出的C文件对象（通常为stderr，可以为NULL）
	/// @note 类只引用文件对象，而非持有，类不会释放文件对象，
	/// 且文件对象的生命周期必须大于此类，否则行为未定义
	/// @note 如果任意构造参数为NULL，则与其对应等级的输出会被取消
	NBT_Print(FILE *_pfOutputInfo = stdout, FILE *_pfOutputWarn = stderr, FILE *_pfOutputErr = stderr)
		: pfOutputInfo(_pfOutputInfo)
		, pfOutputWarn(_pfOutputWarn)
		, pfOutputErr(_pfOutputErr)
	{}
	
	/// @brief 默认析构
	~NBT_Print(void) = default;

	/// @brief 函数调用运算符重载，用于将类作为仿函数调用，通过使用指定等级输出信息
	/// @tparam ...Args 变参模板
	/// @param lvl 用于指示信息打印等级
	/// @param fmt 接受std::format_string的format
	/// @param ...args 变参，与string的format对应
	/// @note 函数不能也不应该抛出任何异常，因为函数可能用于异常信息打印，不能出现二次异常，
	/// 本实现中如果std::format出现任何二次异常，则放弃打印自定义信息，从c标准io的printf输出新抛出的异常信息，
	/// 如果再次失败，则不做处理，因为异常可能已经致命，导致无法执行任何代码。
	template<typename... Args>
	void operator()(NBT_Print_Level lvl, const std::FMT_STR<Args...> fmt, Args&&... args) noexcept
	{
		FILE *pfOutput = NULL;
		switch (lvl)
		{
		case NBT_Print_Level::Info:
			pfOutput = pfOutputInfo;
			break;
		case NBT_Print_Level::Warn:
			pfOutput = pfOutputWarn;
			break;
		case NBT_Print_Level::Err:
			pfOutput = pfOutputErr;
			break;
		default:
			//错误的等级不进行赋值，保持pfOutput为NULL，在后续跳过输出
			break;
		}

		if (pfOutput == NULL)//为NULL跳过
		{
			return;
		}

		try
		{
			auto tmp = std::format(std::move(fmt), std::forward<Args>(args)...);
			fwrite(tmp.data(), sizeof(tmp.data()[0]), tmp.size(), pfOutput);
		}
		catch (const std::exception &e)
		{
			fprintf(stderr, "ErrInfo Exception: \"%s\"\n", e.what());
		}
		catch (...)
		{
			fprintf(stderr, "ErrInfo Exception: \"Unknown Error\"\n");
		}
	}

	/// @brief 函数调用运算符重载，用于将类作为仿函数调用，默认使用Info等级输出信息
	/// @tparam ...Args 变参模板
	/// @param fmt 接受std::format_string的format
	/// @param ...args 变参，与string的format对应
	/// @note 函数不能也不应该抛出任何异常，因为函数可能用于异常信息打印，不能出现二次异常，
	/// 本实现中如果std::format出现任何二次异常，则放弃打印自定义信息，从c标准io的printf输出新抛出的异常信息，
	/// 如果再次失败，则不做处理，因为异常可能已经致命，导致无法执行任何代码。
	template<typename... Args>
	void operator()(const std::FMT_STR<Args...> fmt, Args&&... args) noexcept
	{
		return operator()(NBT_Print_Level::Info, std::move(fmt), std::forward<Args>(args)...);
	}
};