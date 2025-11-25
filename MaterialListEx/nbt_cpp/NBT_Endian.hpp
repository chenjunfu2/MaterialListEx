#pragma once

#include <bit>//字节序
#include <stdint.h>//定义
#include <stddef.h>//size_t
#include <utility>//std::index_sequence
#include <type_traits>//std::make_unsigned_t

#include "Compiler_Define.h"//编译器类型判断

/// @file
/// @brief 端序工具集

/// @brief 用于处理大小端运算，根据实际平台字节序类型进行自动匹配
class NBT_Endian
{
	/// @brief 禁止构造
	NBT_Endian(void) = delete;
	/// @brief 禁止析构
	~NBT_Endian(void) = delete;
	
private:
	constexpr static bool IsLittleEndian(void) noexcept
	{
		return std::endian::native == std::endian::little;
	}

	constexpr static bool IsBigEndian(void) noexcept
	{
		return std::endian::native == std::endian::big;
	}

public:
	/// @brief 颠倒字节序，需要整数字节数为2的倍数或字节数为1
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 字节序的颠倒形式
	/// @note 保守实现，仅需平台支持位操作
	template<typename T>
	requires std::integral<T>
	constexpr static T ByteSwapAny(T data) noexcept
	{
		//必须是2的倍数才能正确执行byteswap
		static_assert(sizeof(T) % 2 == 0 || sizeof(T) == 1, "The size of T is not a multiple of 2 or equal to 1");

		//如果大小是1直接返回
		if constexpr (sizeof(T) == 1)
		{
			return data;
		}

		//统一到无符号类型
		using UT = std::make_unsigned_t<T>;
		static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

		//获取静态大小
		constexpr size_t szSize = sizeof(T);
		constexpr size_t szHalf = sizeof(T) / 2;

		//临时交换量
		UT tmp = 0;

		//(i < sizeof(T) / 2)前半，左移
		[&] <size_t... i>(std::index_sequence<i...>) -> void
		{
			((tmp |= ((UT)data & ((UT)0xFF << (8 * i))) << 8 * (szSize - (i * 2) - 1)), ...);
		}(std::make_index_sequence<szHalf>{});

		//(i + szHalf >= sizeof(T) / 2)后半，右移
		[&] <size_t... i>(std::index_sequence<i...>) -> void
		{
			((tmp |= ((UT)data & ((UT)0xFF << (8 * (i + szHalf)))) >> 8 * (i * 2 + 1)), ...);
		}(std::make_index_sequence<szHalf>{});

		//转换回原先的类型并返回
		return (T)tmp;
	}

	/// @brief 颠倒字节序16位特化版
	/// @param data uint16_t类型的值
	/// @return 字节序的颠倒形式
	/// @note 如果平台支持内建指令，则使用平台内建指令，否则落到保守实现ByteSwapAny
	static uint16_t ByteSwap16(uint16_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if CJF2_NBT_CPP_COMPILER_MSVC
		return _byteswap_ushort(data);
#elif CJF2_NBT_CPP_COMPILER_GCC || CJF2_NBT_CPP_COMPILER_CLANG
		return __builtin_bswap16(data);
#else
		return ByteSwapAny(data);
#endif
	}

	/// @brief 颠倒字节序32位特化版
	/// @param data uint32_t类型的值
	/// @return 字节序的颠倒形式
	/// @note 如果平台支持内建指令，则使用平台内建指令，否则落到保守实现ByteSwapAny
	static uint32_t ByteSwap32(uint32_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if CJF2_NBT_CPP_COMPILER_MSVC
		return _byteswap_ulong(data);
#elif CJF2_NBT_CPP_COMPILER_GCC || CJF2_NBT_CPP_COMPILER_CLANG
		return __builtin_bswap32(data);
#else
		return ByteSwapAny(data);
#endif
	}

	/// @brief 颠倒字节序32位特化版
	/// @param data uint32_t类型的值
	/// @return 字节序的颠倒形式
	/// @note 如果平台支持内建指令，则使用平台内建指令，否则落到保守实现ByteSwapAny
	static uint64_t ByteSwap64(uint64_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if CJF2_NBT_CPP_COMPILER_MSVC
		return _byteswap_uint64(data);
#elif CJF2_NBT_CPP_COMPILER_GCC || CJF2_NBT_CPP_COMPILER_CLANG
		return __builtin_bswap64(data);
#else
		return ByteSwapAny(data);
#endif
	}

	/// @brief 颠倒字节序，自动匹配位数
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 字节序的颠倒形式
	/// @note 如果没有T类型位数的特化版，则落到保守实现ByteSwapAny
	template<typename T>
	requires std::integral<T>
	constexpr static T AutoByteSwap(T data) noexcept
	{
		//如果是已知大小，优先走重载，因为重载更有可能是指令集支持的高效实现
		//否则走位操作实现，效率更低但是兼容性更好
		if constexpr (sizeof(T) == sizeof(uint8_t))
		{
			return data;
		}
		else if constexpr (sizeof(T) == sizeof(uint16_t))
		{
			return (T)ByteSwap16((uint16_t)data);
		}
		else if constexpr (sizeof(T) == sizeof(uint32_t))
		{
			return (T)ByteSwap32((uint32_t)data);
		}
		else if constexpr (sizeof(T) == sizeof(uint64_t))
		{
			return (T)ByteSwap64((uint64_t)data);
		}
		else
		{
			return ByteSwapAny(data);
		}
	}

	//------------------------------------------------------//

	/// @brief 从当前平台字节序转换到大端字节序，自动匹配位数
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 大端字节序的值
	/// @note 如果平台字节序与大端相同，则返回值不变
	template<typename T>
	requires std::integral<T>
	static T NativeToBigAny(T data) noexcept
	{
		if constexpr (IsBigEndian())//当前也是big
		{
			return data;
		}

		//当前是little，little转换到big
		return AutoByteSwap(data);
	}

	/// @brief 从当前平台字节序转换到小端字节序，自动匹配位数
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 小端字节序的值
	/// @note 如果平台字节序与小端相同，则返回值不变
	template<typename T>
	requires std::integral<T>
	static T NativeToLittleAny(T data) noexcept
	{
		if constexpr (IsLittleEndian())//当前也是little
		{
			return data;
		}

		//当前是big，big转换到little
		return AutoByteSwap(data);
	}

	/// @brief 从大端字节序转换到当前平台字节序，自动匹配位数
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 平台字节序的值
	/// @note 如果平台字节序与大端相同，则返回值不变
	template<typename T>
	requires std::integral<T>
	static T BigToNativeAny(T data) noexcept
	{
		if constexpr (IsBigEndian())//当前也是big
		{
			return data;
		}

		//当前是little，big转换到little
		return AutoByteSwap(data);
	}

	/// @brief 从小端字节序转换到当前平台字节序，自动匹配位数
	/// @tparam T 任意整数类型
	/// @param data 任意整数类型的值
	/// @return 平台字节序的值
	/// @note 如果平台字节序与小端相同，则返回值不变
	template<typename T>
	requires std::integral<T>
	static T LittleToNativeAny(T data) noexcept
	{
		if constexpr (IsLittleEndian())//当前也是little
		{
			return data;
		}

		//当前是big，little转换到big
		return AutoByteSwap(data);
	}
};
