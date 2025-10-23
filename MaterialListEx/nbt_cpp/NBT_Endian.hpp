#pragma once

#include <bit>//字节序
#include <stdint.h>//定义
#include <stddef.h>//size_t
#include <utility>//std::index_sequence
#include <type_traits>//std::make_unsigned_t

#include "Compiler_Define.h"//编译器类型判断

class NBT_Endian
{
	NBT_Endian(void) = delete;
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

	static uint16_t ByteSwap16(uint16_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if __COMPILER_MSVC__
		return _byteswap_ushort(data);
#elif __COMPILER_GCC__ || __COMPILER_CLANG__
		return __builtin_bswap16(data);
#else
		return ByteSwapAny(data);
#endif
	}

	static uint32_t ByteSwap32(uint32_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if __COMPILER_MSVC__
		return _byteswap_ulong(data);
#elif __COMPILER_GCC__ || __COMPILER_CLANG__
		return __builtin_bswap32(data);
#else
		return ByteSwapAny(data);
#endif
	}

	static uint64_t ByteSwap64(uint64_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if __COMPILER_MSVC__
		return _byteswap_uint64(data);
#elif __COMPILER_GCC__ || __COMPILER_CLANG__
		return __builtin_bswap64(data);
#else
		return ByteSwapAny(data);
#endif
	}

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

#include "Compiler_Define.h"//二次包含取消定义