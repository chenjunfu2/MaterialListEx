#pragma once

#include <format>
#include <stdio.h>

/// @file
/// @brief 用于处理NBT信息打印的默认实现

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
	FILE *pfOutput = NULL;

public:
	/// @brief 通过c文件对象构造
	/// @param _pfOutput C文件对象
	/// @note 类只引用文件对象，而非持有，类不会释放文件对象，
	/// 且文件对象的生命周期必须大于此类，否则行为未定义
	NBT_Print(FILE *_pfOutput) :pfOutput(_pfOutput)
	{}
	
	/// @brief 默认析构
	~NBT_Print(void) = default;

	/// @brief 函数调用运算符重载，用于将类作为仿函数调用
	/// @tparam ...Args 变参模板
	/// @param fmt 接受std::format_string的format
	/// @param ...args 变参，与string的format对应
	/// @note 函数不能也不应该抛出任何异常，因为函数可能用于异常信息打印，不能出现二次异常，
	/// 本实现中如果std::format出现任何二次异常，则放弃打印自定义信息，从c标准io的printf输出新抛出的异常信息，
	/// 如果再次失败，则不做处理，因为异常可能已经致命，导致无法执行任何代码。
	template<typename... Args>
	void operator()(const std::FMT_STR<Args...> fmt, Args&&... args) noexcept
	{
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
};