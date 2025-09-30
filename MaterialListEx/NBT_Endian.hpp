#pragma once

#include <bit>
#include <stdint.h>

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#define COMPILER_GCC 0
#define COMPILER_CLANG 0
#define COMPILER_NAME "MSVC"
#elif defined(__clang__)
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#define COMPILER_CLANG 1
#define COMPILER_NAME "Clang"
#elif defined(__GNUC__)
#define COMPILER_MSVC 0
#define COMPILER_GCC 1
#define COMPILER_CLANG 0
#define COMPILER_NAME "GCC"
#else
#define COMPILER_MSVC 0
#define COMPILER_GCC 0
#define COMPILER_CLANG 0
#define COMPILER_NAME "Unknown"
#endif

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
	constexpr static T ByteSwapAny(T data) noexcept
	{
		//必须是2的倍数才能正确执行byteswap
		static_assert(sizeof(T) % 2 == 0 || sizeof(T) == 1, "The size of T is not a multiple of 2");

		//如果大小是1直接返回
		if constexpr (sizeof(T) == 1)
		{
			return data;
		}

		//统一到无符号类型
		using UT = typename std::make_unsigned<T>::type;
		static_assert(sizeof(UT) == sizeof(T), "Unsigned type size mismatch");

		//获取静态大小
		constexpr size_t szSize = sizeof(T);
		constexpr size_t szHalf = sizeof(T) / 2;

		//临时交换量
		UT tmp = 0;

		//(Is < sizeof(T) / 2)前半，左移
		[&] <size_t... i>(std::index_sequence<i...>) -> void
		{
			((tmp |= ((UT)data & ((UT)0xFF << (8 * i))) << 8 * (szSize - (i * 2) - 1)), ...);
		}(std::make_index_sequence<szHalf>{});

		//(Is >= sizeof(T) / 2)后半，右移
		[&] <size_t... i>(std::index_sequence<i...>) -> void
		{
			((tmp |= ((UT)data & ((UT)0xFF << (8 * (i + szHalf)))) >> 8 * (i * 2 + 1)), ...);
		}(std::make_index_sequence<szHalf>{});

		return (T)tmp;
	}

	static uint16_t ByteSwap16(uint16_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if COMPILER_MSVC
		return _byteswap_ushort(data);
#elif COMPILER_GCC || COMPILER_CLANG
		return __builtin_bswap16(data);
#else
		return ByteSwapAny(data);
#endif
	}

	static uint32_t ByteSwap32(uint32_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if COMPILER_MSVC
		return _byteswap_ulong(data);
#elif COMPILER_GCC || COMPILER_CLANG
		return __builtin_bswap32(data);
#else
		return ByteSwapAny(data);
#endif
	}

	static uint64_t ByteSwap64(uint64_t data) noexcept
	{
		//根据编译器切换内建指令或使用默认位移实现
#if COMPILER_MSVC
		return _byteswap_uint64(data);
#elif COMPILER_GCC || COMPILER_CLANG
		return __builtin_bswap64(data);
#else
		return ByteSwapAny(data);
#endif
	}

	//------------------------------------------------------//

	static uint16_t ToBig16(uint16_t data) noexcept
	{
		if constexpr (IsBigEndian())
		{
			return data;
		}

		return ByteSwap16(data);
	}

	static uint32_t ToBig32(uint32_t data) noexcept
	{
		if constexpr (IsBigEndian())
		{
			return data;
		}

		return ByteSwap32(data);
	}

	static uint64_t ToBig64(uint64_t data) noexcept
	{
		if constexpr (IsBigEndian())
		{
			return data;
		}

		return ByteSwap64(data);
	}

	template<typename T>
	static T ToBigAny(T data) noexcept
	{
		if constexpr (IsBigEndian())
		{
			return data;
		}

		if constexpr (sizeof(T) == sizeof(uint8_t))
		{
			return data;
		}
		else if constexpr (sizeof(T) == sizeof(uint16_t))
		{
			return ByteSwap16(data);
		}
		else if constexpr (sizeof(T) == sizeof(uint32_t))
		{
			return ByteSwap32(data);
		}
		else if constexpr (sizeof(T) == sizeof(uint64_t))
		{
			return ByteSwap64(data);
		}
		else
		{
			return ByteSwapAny(data);
		}
	}

	//-----------------------------------//

	static uint16_t ToLittle16(uint16_t data) noexcept
	{
		if constexpr (IsLittleEndian())
		{
			return data;
		}

		return ByteSwap16(data);
	}

	static uint32_t ToLittle32(uint32_t data) noexcept
	{
		if constexpr (IsLittleEndian())
		{
			return data;
		}

		return ByteSwap32(data);
	}

	static uint64_t ToLittle64(uint64_t data) noexcept
	{
		if constexpr (IsLittleEndian())
		{
			return data;
		}

		return ByteSwap64(data);
	}

	template<typename T>
	static T ToLittleAny(T data) noexcept
	{
		if constexpr (IsLittleEndian())
		{
			return data;
		}

		if constexpr (sizeof(T) == sizeof(uint8_t))
		{
			return data;
		}
		else if constexpr (sizeof(T) == sizeof(uint16_t))
		{
			return ByteSwap16(data);
		}
		else if constexpr (sizeof(T) == sizeof(uint32_t))
		{
			return ByteSwap32(data);
		}
		else if constexpr (sizeof(T) == sizeof(uint64_t))
		{
			return ByteSwap64(data);
		}
		else
		{
			return ByteSwapAny(data);
		}
	}

};