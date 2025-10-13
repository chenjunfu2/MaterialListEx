#pragma once

#include <string>
#include <type_traits>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>//size_t
#include <array>
#include <algorithm>

//来个static string包装类，使得模板能接受字符串字面量
//必须放在外面，否则NTTP推导主类模板会失败，
//导致此并不依赖主类模板的模板也推导失败
template<typename T, size_t N>
class StringLiteral : public std::array<T, N>
{
public:
	using Super = std::array<T, N>;

public:
	constexpr StringLiteral(const T(&_tStr)[N]) noexcept : Super(std::to_array(_tStr))
	{}
	constexpr ~StringLiteral(void) = default;

};

template<typename MU8T = uint8_t, typename U16T = char16_t, typename U8T = char8_t>
class MUTF8_Tool
{
	static_assert(sizeof(MU8T) == 1, "MU8T size must be at 1 byte");
	static_assert(sizeof(U16T) == 2, "U16T size must be at 2 bytes");
	static_assert(sizeof(U8T) == 1, "U8T size must be at 1 bytes");

	MUTF8_Tool(void) = delete;
	~MUTF8_Tool(void) = delete;

	//用于在错误情况输出utf16错误字节0xFFFD和mu8、u8形式的0xEF 0xBF 0xBD
	static inline constexpr MU8T mu8FailChar[3]{ (MU8T)0xEF, (MU8T)0xBF, (MU8T)0xBD };
	static inline constexpr U16T u16FailChar = (U16T)0xFFFD;
	static inline constexpr U8T u8FailChar[3]{ (U8T)0xEF, (U8T)0xBF, (U8T)0xBD };

public:
	using MU8_T = MU8T;
	using U16_T = U16T;
	using U8_T = U8T;

private:
	//来点魔法类，伪装成basic string，在插入的时候进行数据长度计数，忽略插入的数据，最后转换为size_t长度
	//这样就能在最小修改的情况下用同一个函数套模板来获取转换后的长度（且100%准确），而不是重写一个例程
	template<typename T>
	class FakeStringCounter
	{
	private:
		size_t szCounter = 0;
	public:
		constexpr FakeStringCounter(void) = default;
		constexpr ~FakeStringCounter(void) = default;

		constexpr void clear(void) noexcept
		{
			szCounter = 0;
		}

		constexpr FakeStringCounter &append(const T *const, size_t szSize) noexcept
		{
			szCounter += szSize;
			return *this;
		}

		template<typename U>
		constexpr FakeStringCounter &append_cvt(const U *const, size_t szSize) noexcept
		{
			szCounter += szSize;//静态求值与动态求值并无区别，不做特例
			return *this;
		}

		constexpr void push_back(const T &) noexcept
		{
			szCounter += 1;
		}

		constexpr const size_t &GetData(void) const noexcept
		{
			return szCounter;
		}
	};

	//魔法类2，伪装成string，转换到静态字符串作为std::array返回
	template<typename T, size_t N>
	class StaticString
	{
	public:
		using ARRAY_TYPE = std::array<T, N>;
	private:
		ARRAY_TYPE arrData{};
		size_t szIndex = 0;
	public:
		constexpr StaticString(void) = default;
		constexpr ~StaticString(void) = default;

		constexpr void clear(void) noexcept
		{
			szIndex = 0;
		}

		constexpr StaticString &append(const T *const pData, size_t szLength) noexcept
		{
			if (szLength == 0 || szLength > arrData.size() - szIndex)//减法避免溢出，注意这里不是大等于而是大于，否则会导致拷贝差1错误
			{
				return *this;
			}

			//从pData的 0 ~ szLength 拷贝到arrData的 szIndex ~ szIndex + szLength
			std::ranges::copy(&pData[0], &pData[szLength], &arrData[szIndex]);
			szIndex += szLength;

			return *this;
		}

		template<typename U>
		constexpr StaticString &append_cvt(const U *const pData, size_t szLength) noexcept
		{
			if (std::is_constant_evaluated())
			{
				if (szLength == 0 || szLength > arrData.size() - szIndex)
				{
					return *this;
				}

				//静态求值按顺序转换
				std::ranges::transform(&pData[0], &pData[szLength], &arrData[szIndex],
					[](const U &u) -> T
					{
						return (T)u;
					});
				szIndex += szLength;

				return *this;
			}
			else//注意虽然理论上这个函数不会在任何情况被用于动态，但是为了明确操作，仍然这么写
			{
				return append((const T *const)pData, szLength);//非静态直接暴力转换指针
			}
		}

		constexpr void push_back(const T &tData) noexcept
		{
			if (1 > arrData.size() - szIndex)
			{
				return;
			}

			arrData[szIndex] = tData;
			szIndex += 1;
		}

		constexpr const ARRAY_TYPE &GetData(void) const noexcept
		{
			return arrData;
		}
	};

	//魔法类3，给String添加静态转换接口，与原先一致，防止报错
	template<typename String>
	class DynamicString : public String//注意使用继承，这样可以直接隐式转换到基类
	{
	public:
		DynamicString(size_t szReserve = 0) :String({})
		{
			String::reserve(szReserve);
		}
		~DynamicString(void) = default;

		template<typename U>
		DynamicString &append_cvt(const U *const pData, size_t szLength)//理论上此函数永远动态调用
		{
			String::append((const typename String::value_type *const)pData, szLength);
			return *this;
		}

		constexpr const String &GetData(void) const noexcept
		{
			return *this;
		}
	};

private:
	template<size_t szBytes>
	static constexpr void EncodeMUTF8Bmp(const U16T u16Char, MU8T(&mu8CharArr)[szBytes])
	{
		if constexpr (szBytes == 1)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0111'1111) >>  0) | (uint16_t)0b0000'0000);//0 + 6-0   7bit
		}
		else if constexpr (szBytes == 2)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0111'1100'0000) >>  6) | (uint16_t)0b1100'0000);//110 + 10-6  5bit
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b1000'0000);//10 + 5-0   6bit
		}
		else if constexpr (szBytes == 3)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b1111'0000'0000'0000) >> 12) | (uint16_t)0b1110'0000);//1110 + 15-12   4bit
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'1111'1100'0000) >>  6) | (uint16_t)0b1000'0000);//10 + 11-6   6bit
			mu8CharArr[2] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b1000'0000);//10 + 5-0   6bit
		}
		else
		{
			static_assert(false, "Error szBytes Size");//大小错误
		}
	}

	static constexpr void EncodeMUTF8Supplementary(const U16T u16HighSurrogate, const U16T u16LowSurrogate,  MU8T(&mu8CharArr)[6])
	{
		//取出代理对数据并组合 范围：1'0000 ~ 10'FFFF 通过u16代理对组成：高代理10位低代理10位，
		//最后加上0x1'0000得到此范围，也就是从utf16组成的0x0'0000 ~ 0xF'FFFF + 0x1'0000得到
		uint32_t u32RawChar = ((uint32_t)((uint16_t)u16HighSurrogate & (uint16_t)0b0000'0011'1111'1111)) << 10 |//10bit
							  ((uint32_t)((uint16_t)u16LowSurrogate  & (uint16_t)0b0000'0011'1111'1111)) <<  0; //10bit

		//因为mutf8直接存储utf16的代理对位，不进行+0x1'0000运算
		//u32RawChar += (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000;//bit16->1 = 0x1'0000 注意此处为加法而非位运算

		//高代理
		mu8CharArr[0] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 + 19-16   4bit
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1000'0000);//10 + 15-10   6bit
		//低代理
		mu8CharArr[3] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[4] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1100'0000) >>  6) | (uint32_t)0b1011'0000);//1011 + 9-6   4bit
		mu8CharArr[5] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint32_t)0b1000'0000);//10 + 5-0   6bit
	}

	template<size_t szBytes>
	static constexpr void DecodeMUTF8Bmp(const MU8T(&mu8CharArr)[szBytes], U16T &u16Char)
	{
		if constexpr (szBytes == 1)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0111'1111)) << 0;//7bit
		}
		else if constexpr (szBytes == 2)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0001'1111)) << 6 |//5bit
					  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0011'1111)) << 0; //6bit
		}
		else if constexpr (szBytes == 3)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0000'1111)) << 12 |//4bit
					  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0011'1111)) <<  6 |//6bit
					  ((uint16_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) <<  0; //6bit
		}
		else
		{
			static_assert(false, "Error szBytes Size");//大小错误
		}
	}

	static constexpr void DecodeMUTF8Supplementary(const MU8T(&mu8CharArr)[6], U16T &u16HighSurrogate, U16T &u16LowSurrogate)
	{
		uint32_t u32RawChar = 					 //mu8CharArr[0] ignore 固定字节忽略
							  ((uint32_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'1111)) << 16 |//4bit
							  ((uint32_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) << 10 |//6bit
												 //mu8CharArr[3] ignore 固定字节忽略
							  ((uint32_t)((uint8_t)mu8CharArr[4] & (uint8_t)0b0000'1111)) <<  6 |//4bit
							  ((uint32_t)((uint8_t)mu8CharArr[5] & (uint8_t)0b0011'1111)) <<  0 ;//6bit

		//因为mutf8直接存储utf16的代理对位，不进行-0x1'0000运算
		//u32RawChar -= (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000;//bit16->1 = 0x1'0000 注意此处为减法而非位运算

		//解析到高低代理
		//范围1'0000-10'FFFF
		u16HighSurrogate = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'1111'1111'1100'0000'0000) >> 10 | (uint32_t)0b1101'1000'0000'0000);//1101'10 + 19-10   10bit
		u16LowSurrogate  = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1111'1111) >>  0 | (uint32_t)0b1101'1100'0000'0000);//1101'11 + 9-0   10bit
	}

	/*
		4bytes utf-8 bit distribution:（utf8直接存储utf32的映射，需要两次转换到mutf8）
		000u'uuuu zzzz'yyyy yyxx'xxxx - 1111'0uuu 10uu'zzzz 10yy'yyyy 10xx'xxxx

		6bytes modified utf-8 bit distribution:（mutf8直接存储了utf16的映射，不需要增减0x1'0000，所以要比utf8小一位）
		0000'uuuu zzzz'yyyy yyxx'xxxx - 1110'1101 1010'uuuu 10zz'zzyy 1110'1101 1011'yyyy 10xx'xxxx
	*/

	static constexpr void UTF8SupplementaryToMUTF8(const U8T(&u8CharArr)[4], MU8T(&mu8CharArr)[6])
	{
		//先把utf8映射到utf32，减去0x1'0000，得到utf16的中间形式，再进行下一步转换
		uint32_t u32RawChar = ((uint32_t)((uint8_t)u8CharArr[0] & (uint8_t)0b0000'0111)) << 18 |//3bit
							  ((uint32_t)((uint8_t)u8CharArr[1] & (uint8_t)0b0011'1111)) << 12 |//6bit
							  ((uint32_t)((uint8_t)u8CharArr[2] & (uint8_t)0b0011'1111)) <<  6 |//6bit
							  ((uint32_t)((uint8_t)u8CharArr[3] & (uint8_t)0b0011'1111)) <<  0; //6bit

		//删除位
		u32RawChar -= (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000;//bit16->1 = 0x1'0000 注意此处为减法而非位运算

		//高代理
		mu8CharArr[0] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 + 19-16   4bit
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1000'0000);//10 + 15-10   6bit
		//低代理
		mu8CharArr[3] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[4] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1100'0000) >>  6) | (uint32_t)0b1011'0000);//1011 + 9-6   4bit
		mu8CharArr[5] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint32_t)0b1000'0000);//10 + 5-0   6bit
	}

	static constexpr void MUTF8SupplementaryToUTF8(const MU8T(&mu8CharArr)[6], U8T(&u8CharArr)[4])
	{
		//先把mutf8映射到utf16组成的中间形式的utf32，再加上0x1'0000得到utf8的utf32表示形式
		uint32_t u32RawChar = 					 //mu8CharArr[0] ignore 固定字节忽略
							  ((uint32_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'1111)) << 16 |//4bit
							  ((uint32_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) << 10 |//6bit
												 //mu8CharArr[3] ignore 固定字节忽略
							  ((uint32_t)((uint8_t)mu8CharArr[4] & (uint8_t)0b0000'1111)) <<  6 |//4bit
							  ((uint32_t)((uint8_t)mu8CharArr[5] & (uint8_t)0b0011'1111)) <<  0 ;//6bit

		//恢复位
		u32RawChar += (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000;//bit16->1 = 0x1'0000 注意此处为加法而非位运算

		//转换到utf8
		u8CharArr[0] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0001'1100'0000'0000'0000'0000) >> 18) | (uint32_t)0b1111'0000);//1111'0 + 20-18   3bit
		u8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0011'1111'0000'0000'0000) >> 12) | (uint32_t)0b1000'0000);//10 + 17-12   6bit
		u8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'1111'1100'0000) >>  6) | (uint32_t)0b1000'0000);//10 + 11-6   6bit
		u8CharArr[3] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint32_t)0b1000'0000);//10 + 5-0   6bit
	}

private:
//c=char d=do检查迭代器并获取下一个字节（如果可以，否则执行指定语句后跳出）
#define GET_NEXTCHAR(c,d) if (++it == end) { (d);break; } else { (c) = *it; }
//v=value m=mask p=pattern t=test 测试遮罩位之后的结果是否是指定值或值是否是由指定bits组成
#define HAS_BITMASK(v,m,p) (((uint8_t)(v) & (uint8_t)(m)) == (uint8_t)(p))
#define IS_BITS(v,t) ((uint8_t)(v) == (uint8_t)(t))
//v=value b=begin e=end 注意范围是左右边界包含关系，而不是普通的左边界包含
#define IN_RANGE(v,b,e) (((uint16_t)(v) >= (uint16_t)(b)) && ((uint16_t)(v) <= (uint16_t)(e)))

	template<typename T = std::basic_string<MU8T>>
	static constexpr T U16ToMU8Impl(const U16T *u16String, size_t szStringLength, T mu8String = {})
	{
#define PUSH_FAIL_MU8CHAR mu8String.append(mu8FailChar, sizeof(mu8FailChar) / sizeof(MU8T))

		//因为string带长度信息，则不用处理0字符情况，for不会进入，直接返回size为0的mu8str
		//mu8字符串结尾为0xC0 0x80而非0x00

		for (auto it = u16String, end = u16String + szStringLength; it != end; ++it)
		{
			U16T u16Char = *it;//第一次
			if (IN_RANGE(u16Char, 0x0001, 0x007F))//单字节码点
			{
				MU8T mu8Char[1]{};
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0080, 0x07FF) || u16Char == 0x0000)//双字节码点，0字节特判
			{
				MU8T mu8Char[2]{};
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0800, 0xFFFF))//三字节码点or多字节码点
			{
				if (IN_RANGE(u16Char, 0xD800, 0xDBFF))//遇到高代理对
				{	
					U16T u16HighSurrogate = u16Char;//保存高代理
					U16T u16LowSurrogate{};//读取低代理
					GET_NEXTCHAR(u16LowSurrogate, (PUSH_FAIL_MU8CHAR));//第二次
					//如果上面读取提前返回，则高代理后无数据，插入转换后的u16未知字符
					
					//判断低代理范围
					if (!IN_RANGE(u16LowSurrogate, 0xDC00, 0xDFFF))//错误，高代理后非低代理
					{
						--it;//撤回一次刚才的读取，重新判断非低代理字节
						PUSH_FAIL_MU8CHAR;//插入u16未知字符
						continue;//重试，for会重新++it，相当于重试当前*it
					}

					//代理对特殊处理：共6字节表示一个实际代理码点
					MU8T mu8Char[6]{};
					EncodeMUTF8Supplementary(u16HighSurrogate, u16LowSurrogate, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
				else//高代理之前遇到低代理或其它合法3字节字符
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//在高代理之前遇到低代理
					{
						//不撤回读取，丢弃错误的低代理
						PUSH_FAIL_MU8CHAR;//错误，插入u16未知字符
						continue;//重试
					}

					//转换3字节字符
					MU8T mu8Char[3]{};
					EncodeMUTF8Bmp(u16Char, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
			}
			else
			{
				assert(false);//??????????????怎么命中的
			}
		}

		return mu8String;

#undef PUSH_FAIL_MU8CHAR
	}

	template<typename T = std::basic_string<U16T>>
	static constexpr T MU8ToU16Impl(const MU8T *mu8String, size_t szStringLength, T u16String = {})
	{
#define PUSH_FAIL_U16CHAR u16String.push_back(u16FailChar)

		//因为string带长度信息，则不用处理0字符情况，for不会进入，直接返回size为0的u16str
		//u16字符串末尾为0x0000
		
		for (auto it = mu8String, end = mu8String + szStringLength; it != end; ++it)
		{
			MU8T mu8Char = *it;//第一次
			//判断是几字节的mu8
			if (HAS_BITMASK(mu8Char, 0b1000'0000, 0b0000'0000))//最高位为0，单字节码点
			{
				//放入数组
				MU8T mu8CharArr[1] = { mu8Char };

				//转换
				U16T u16Char{};
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if (HAS_BITMASK(mu8Char, 0b1110'0000, 0b1100'0000))//高3位为110，双字节码点
			{
				//先保存第一个字节
				MU8T mu8CharArr[2] = { mu8Char };//[0]=mu8Char
				//尝试获取下一个字节
				GET_NEXTCHAR(mu8CharArr[1], (PUSH_FAIL_U16CHAR));//第二次
				//判断字节合法性
				if (!HAS_BITMASK(mu8CharArr[1], 0b1100'0000, 0b1000'0000))//高2位不是10，错误，跳过
				{
					--it;//撤回读取（避免for自动递增跳过刚才的字符）
					PUSH_FAIL_U16CHAR;//替换为utf16错误字符
					continue;//重试，因为当前字符可能是错误的，而刚才多读取的才是正确的，所以需要撤回continue重新尝试
				}

				//转换
				U16T u16Char{};
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if (HAS_BITMASK(mu8Char, 0b1111'0000, 0b1110'0000))//高4位为1110，三字节或多字节码点
			{
				//提前获取下一个字符，这是代理对的判断依据
				MU8T mu8Next{};
				GET_NEXTCHAR(mu8Next, (PUSH_FAIL_U16CHAR));//第二次

				//合法性判断（区分是否为代理）
				//代理区分：因为D800开头的为高代理，必不可能作为三字节码点0b1010'xxxx出现，所以只要高4位是1010必为代理对
				//也就是说mu8CharArr3[0]的低4bit如果是1101并且mu8Char的高4bit是1010的情况下，即三字节码点10xx'xxxx中的最高二个xx为01，
				//把他们合起来就是1101'10xx 也就是0xD8，即u16的高代理对开始字符，而代理对在encode过程走的另一个流程，不存在与3字节码点混淆处理的情况
				if (IS_BITS(mu8Char, 0b1110'1101) && HAS_BITMASK(mu8Next, 0b1111'0000, 0b1010'0000))//代理对，必须先判断，很重要！
				{
					//保存到数组
					MU8T mu8CharArr[6] = { mu8Char,mu8Next };//[0] = mu8Char, [1] = mu8Next

					//继续读取后4个并验证

					//下一个为高代理的低6位
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//第三次
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))
					{
						//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
						--it;
						//替换为二个utf16错误字符
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//下一个为固定字符
					GET_NEXTCHAR(mu8CharArr[3],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//第四次
					if (!IS_BITS(mu8CharArr[3], 0b1110'1101))
					{
						//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
						--it;
						//替换为三个utf16错误字符
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//下一个为低代理高4位
					GET_NEXTCHAR(mu8CharArr[4],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//第五次
					if (!HAS_BITMASK(mu8CharArr[4], 0b1111'0000, 0b1011'0000))
					{
						//撤回二次读取，尽管前面已确认是0b1110'1101，但是存在111开头的合法3码点
						--it;
						--it;
						//替换为三个utf16错误字符，因为撤回二次，本来有4个错误字节的现在只要3个
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//读取最后一个低代理的低6位
					GET_NEXTCHAR(mu8CharArr[5],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//第六次
					if (!HAS_BITMASK(mu8CharArr[5], 0b1100'0000, 0b1000'0000))
					{
						//撤回一次读取，因为不存在前一个已确认的101开头的合法码点，且再前一个开头为111，也就是不存在111后跟101的3码点情况，跳过
						--it;
						//替换为五个utf16错误字符
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//验证全部通过，转换代理对
					U16T u16HighSurrogate{}, u16LowSurrogate{};
					DecodeMUTF8Supplementary(mu8CharArr, u16HighSurrogate, u16LowSurrogate);
					u16String.push_back(u16HighSurrogate);
					u16String.push_back(u16LowSurrogate);
				}
				else if(HAS_BITMASK(mu8Next, 0b1100'0000, 0b1000'0000))//三字节码点，排除代理对后只有这个可能，看看是不是10开头的尾随字节
				{
					//保存
					MU8T mu8CharArr[3] = { mu8Char,mu8Next };//[0] = mu8Char, [1] = mu8Next

					//尝试获取下一字符
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//第三次
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))//错误，3字节码点最后一个不是正确字符
					{
						//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
						--it;
						//替换为二个utf16错误字符
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//3位已就绪，转换
					U16T u16Char{};
					DecodeMUTF8Bmp(mu8CharArr, u16Char);
					u16String.push_back(u16Char);
				}
				else
				{
					//撤回mu8Next的读取，因为mu8Char已经判断过，能运行到这里，
					//证明此字节错误，如果撤回到mu8Char会导致无限错误循环，
					//只撤回到mu8Next即可，for会重新++it，相当于重试当前*it
					--it;
					//替换为一个utf16错误字符
					PUSH_FAIL_U16CHAR;
					continue;
				}
			}
			else
			{
				//未知，跳过并忽略，直到遇到下一个正确起始字符
				//替换为一个utf16错误字符
				PUSH_FAIL_U16CHAR;
				continue;
			}
		}
	
		return u16String;

#undef PUSH_FAIL_U16CHAR
	}

	/*
	Modified UTF-8 与 "标准"UTF-8 格式有二点区别：
	第一，空字符(char)0使用双字节格式0xC0 0x80而非单字节格式0x00，
	因此 Modified UTF-8字符串不会包含嵌入式空值；

	第二，仅使用标准UTF-8的单字节、双字节和三字节格式。
	Java虚拟机不识别标准UTF-8的四字节格式，
	而是使用自定义的双三字节（6字节代理对）格式。
	*/

	template<typename T = DynamicString<std::basic_string<MU8T>>>
	static constexpr T U8ToMU8Impl(const U8T *u8String, size_t szStringLength, T mu8String = {})
	{
#define PUSH_FAIL_MU8CHAR mu8String.append(mu8FailChar, sizeof(mu8FailChar) / sizeof(MU8T))
#define INSERT_NORMAL(p) (mu8String.append_cvt((p) - szNormalLength, szNormalLength), szNormalLength = 0)

		size_t szNormalLength = 0;//普通字符的长度，用于优化批量插入
		for (auto it = u8String, end = u8String + szStringLength; it != end; ++it)
		{
			//u8到mu8，处理u8空字符，处理4字节u8转换到6字节mu8
			U8T u8Char = *it;//第一次
			if (HAS_BITMASK(u8Char, 0b1111'1000, 0b1111'0000))//高5位为11110，utf8的4字节
			{
				INSERT_NORMAL(it);//在处理之前先插入之前被跳过的普通字符
				//转换u8的4字节到mu8的6字节，并处理错误

				U8T u8CharArr[4]{ u8Char };//[0] = u8Char

				GET_NEXTCHAR(u8CharArr[1], (PUSH_FAIL_MU8CHAR));//第二次
				if (!HAS_BITMASK(u8CharArr[1], 0b1100'0000, 0b1000'0000))//确保高2bit是10
				{
					//输出一个错误字符
					PUSH_FAIL_MU8CHAR;
					--it;//回退一次读取，尝试处理不以10开头的
					continue;
				}

				GET_NEXTCHAR(u8CharArr[2],
					(PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR));//第三次
				if (!HAS_BITMASK(u8CharArr[2], 0b1100'0000, 0b1000'0000))//确保高2bit是10
				{
					//输出二个错误字符
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					--it;//回退一次读取，尝试处理不以10开头的
					continue;
				}

				GET_NEXTCHAR(u8CharArr[3],
					(PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR));//第四次
				if (!HAS_BITMASK(u8CharArr[3], 0b1100'0000, 0b1000'0000))//确保高2bit是10
				{
					//输出三个错误字符
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					--it;//回退一次读取，尝试处理不以10开头的
					continue;
				}

				//读取成功完成
				MU8T mu8CharArr[6]{};
				UTF8SupplementaryToMUTF8(u8CharArr, mu8CharArr);
				mu8String.append(mu8CharArr, sizeof(mu8CharArr) / sizeof(MU8T));
			}
			else if (IS_BITS(u8Char, 0b0000'0000))//\0字符
			{
				INSERT_NORMAL(it);//在处理之前先插入之前被跳过的普通字符

				MU8T mu8EmptyCharArr[2] = { (MU8T)0xC0,(MU8T)0x80 };//mu8固定0字节
				mu8String.append(mu8EmptyCharArr, sizeof(mu8EmptyCharArr) / sizeof(MU8T));
			}
			else//都不是，递增普通字符长度，直到遇到特殊字符的时候插入
			{
				++szNormalLength;
			}
		}
		//结束后再插入一次，因为for内可能完全没有进入过任何一个特殊块，
		//且因为结束后for是从末尾退出的，所以从末尾开始作为当前指针插入
		INSERT_NORMAL(u8String + szStringLength);


		return mu8String;

#undef INSERT_NORMAL
#undef PUSH_FAIL_MU8CHAR
	}

	template<typename T = DynamicString<std::basic_string<U8T>>>
	static constexpr T MU8ToU8Impl(const MU8T *mu8String, size_t szStringLength, T u8String = {})
	{
#define PUSH_FAIL_U8CHAR u8String.append(u8FailChar, sizeof(u8FailChar) / sizeof(U8T))
#define INSERT_NORMAL(p) (u8String.append_cvt((p) - szNormalLength, szNormalLength), szNormalLength = 0)

		size_t szNormalLength = 0;//普通字符的长度，用于优化批量插入
		for (auto it = mu8String, end = mu8String + szStringLength; it != end; ++it)
		{
			MU8T mu8Char = *it;//第一次

			if (HAS_BITMASK(mu8Char, 0b1111'0000, 0b1110'0000))//高4为为1110，mu8的3字节或多字节码点
			{
				//提前获取下一个
				MU8T mu8Next{};
				if (++it == end)
				{
					//把前面的都插入一下
					INSERT_NORMAL(it - 1);//注意这里的-1，因为正常是要在块语句开头执行的，这里已经超前移动了一下迭代器，回退1当前位置
					PUSH_FAIL_U8CHAR;//插入错误字符
					break;
				}
				mu8Next = *it;//第二次


				//以1110'1101字节开始且下一个字节高4位是1010开头的必然是代理对
				if (!IS_BITS(mu8Char, 0b1110'1101) || !HAS_BITMASK(mu8Next, 0b1111'0000, 0b1010'0000))
				{
					szNormalLength += 2;//前面消耗了两个，递增两次
					continue;//然后继续循环
				}

				//已确认是代理对，把前面的都插入一下
				INSERT_NORMAL(it - 1);//注意这里的-1，因为正常是要在块语句开头执行的，这里已经超前读取了一个mu8Next，回退1当前位置

				//继续读取后4个并验证
				MU8T mu8CharArr[6] = { mu8Char, mu8Next };//[0] = mu8Char, [1] = mu8Next

				//获取下一个
				GET_NEXTCHAR(mu8CharArr[2],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//第三次
				if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))
				{
					//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
					--it;
					//替换为二个utf8错误字符
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//获取下一个
				GET_NEXTCHAR(mu8CharArr[3],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//第四次
				if (!IS_BITS(mu8CharArr[3], 0b1110'1101))
				{
					//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
					--it;
					//替换为三个utf8错误字符
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//获取下一个
				GET_NEXTCHAR(mu8CharArr[4],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//第五次
				if (!HAS_BITMASK(mu8CharArr[4], 0b1111'0000, 0b1011'0000))
				{
					//撤回二次读取，尽管前面已确认是0b1110'1101，但是存在111开头的合法3码点
					--it;
					--it;
					//替换为三个utf8错误字符，因为撤回二次，本来有4个错误字节的现在只要3个
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//获取下一个
				GET_NEXTCHAR(mu8CharArr[5],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//第六次
				if (!HAS_BITMASK(mu8CharArr[5], 0b1100'0000, 0b1000'0000))
				{
					//撤回一次读取（为什么不是二次？因为前一个字符已确认是10开头的尾随字符，跳过）
					--it;
					//替换为五个utf8错误字符
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//到此，全部验证通过，进行转换
				U8T u8CharArr[4]{};
				MUTF8SupplementaryToUTF8(mu8CharArr, u8CharArr);
				u8String.append(u8CharArr, sizeof(u8CharArr) / sizeof(U8T));
			}
			else if (IS_BITS(mu8Char, 0xC0))//注意以0xC0开头的，必然是2字节码，所以如果里面没有第二个字符，则必然错误
			{
				//提前获取下一个
				MU8T mu8Next{};
				if (++it == end)
				{
					//把前面的都插入一下
					INSERT_NORMAL(it - 1);
					PUSH_FAIL_U8CHAR;//插入错误字符
					break;
				}
				mu8Next = *it;//第二次

				if (!IS_BITS(mu8Next, 0x80))//如果不是，说明是别的字节模式
				{
					szNormalLength += 2;//普通字符数加2然后继续
					continue;
				}

				//已确认是0字符，插入一下前面的所有内容
				INSERT_NORMAL(it - 1);//注意这里的-1，因为正常是要在块语句开头执行的，这里已经超前读取了一个mu8Next，回退1当前位置
				u8String.push_back((U8T)0x00);//插入0字符
			}
			else
			{
				++szNormalLength;//普通字符，递增
				continue;//继续
			}
		}
		//最后再把for中剩余未插入的插入一下，注意这里起始位置其实是for中的end位置
		INSERT_NORMAL(mu8String + szStringLength);

		return u8String;

#undef INSERT_NORMAL
#undef PUSH_FAIL_U8CHAR
	}

#undef IN_RANGE
#undef IS_BITS
#undef HAS_BITMASK
#undef GET_NEXTCHAR

private:
	template<typename T>
	static consteval size_t ContentLength(const T &tStr)//获取不包含末尾0的长度（如果有）
	{
		return tStr.size() > 0 && tStr[tStr.size() - 1] == (typename T::value_type)0x00
			 ? tStr.size() - 1
			 : tStr.size();
	}

public:
	//---------------------------------------------------------------------------------------------//

	static constexpr size_t U16ToMU8Length(const std::basic_string_view<U16T> &u16String)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String.data(), u16String.size()).GetData();
	}
	static constexpr size_t U16ToMU8Length(const U16T *u16String, size_t szStringLength)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String, szStringLength).GetData();
	}
	
	static std::basic_string<MU8T> U16ToMU8(const std::basic_string_view<U16T> &u16String, size_t szReserve = 0)
	{
		return U16ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u16String.data(), u16String.size(), { szReserve }).GetData();
	}
	static std::basic_string<MU8T> U16ToMU8(const U16T *u16String, size_t szStringLength, size_t szReserve = 0)
	{
		return U16ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u16String, szStringLength, { szReserve }).GetData();
	}

	template<StringLiteral u16String>
	requires std::is_same_v<typename decltype(u16String)::value_type, U16T>//限定类型
	static consteval auto U16ToMU8(void)
	{
		constexpr size_t szStringLength = ContentLength(u16String);
		constexpr size_t szNewLength = U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String.data(), szStringLength).GetData();

		return U16ToMU8Impl<StaticString<MU8T, szNewLength>>(u16String.data(), szStringLength).GetData();
	}

	//---------------------------------------------------------------------------------------------//

	static constexpr size_t U8ToMU8Length(const std::basic_string_view<U8T> &u8String)
	{
		return U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String.data(), u8String.size()).GetData();
	}
	static constexpr size_t U8ToMU8Length(const U8T *u8String, size_t szStringLength)
	{
		return U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String, szStringLength).GetData();
	}

	static std::basic_string<MU8T> U8ToMU8(const std::basic_string_view<U8T> &u8String, size_t szReserve = 0)
	{
		return U8ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u8String.data(), u8String.size(), { szReserve }).GetData();
	}
	static std::basic_string<MU8T> U8ToMU8(const U8T *u8String, size_t szStringLength, size_t szReserve = 0)
	{
		return U8ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u8String, szStringLength, { szReserve }).GetData();
	}

	template<StringLiteral u8String>
	requires std::is_same_v<typename decltype(u8String)::value_type, U8T>//限定类型
	static consteval auto U8ToMU8(void)
	{
		constexpr size_t szStringLength = ContentLength(u8String);
		constexpr size_t szNewLength = U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String.data(), szStringLength).GetData();

		return U8ToMU8Impl<StaticString<MU8T, szNewLength>>(u8String.data(), szStringLength).GetData();
	}

	//---------------------------------------------------------------------------------------------//
	//---------------------------------------------------------------------------------------------//

	static constexpr size_t MU8ToU16Size(const std::basic_string_view<MU8T> &mu8String)
	{
		return MU8ToU16Impl<FakeStringCounter<U16T>>(mu8String.data(), mu8String.size()).GetData();
	}
	static constexpr size_t MU8ToU16Size(const MU8T *mu8String, size_t szStringLength)
	{
		return MU8ToU16Impl<FakeStringCounter<U16T>>(mu8String, szStringLength).GetData();
	}

	static std::basic_string<U16T> MU8ToU16(const std::basic_string_view<MU8T> &mu8String, size_t szReserve = 0)
	{
		return MU8ToU16Impl<DynamicString<std::basic_string<U16T>>>(mu8String.data(), mu8String.size(), { szReserve }).GetData();
	}
	static std::basic_string<U16T> MU8ToU16(const MU8T *mu8String, size_t szStringLength, size_t szReserve = 0)
	{
		return MU8ToU16Impl<DynamicString<std::basic_string<U16T>>>(mu8String, szStringLength, { szReserve }).GetData();
	}

	//---------------------------------------------------------------------------------------------//

	static constexpr size_t MU8ToU8Length(const std::basic_string_view<MU8T> &mu8String)
	{
		return MU8ToU8Impl<FakeStringCounter<U8T>>(mu8String.data(), mu8String.size()).GetData();
	}
	static constexpr size_t MU8ToU8Length(const MU8T *mu8String, size_t szStringLength)
	{
		return MU8ToU8Impl<FakeStringCounter<U8T>>(mu8String, szStringLength).GetData();
	}

	static std::basic_string<U8T> MU8ToU8(const std::basic_string_view<MU8T> &mu8String, size_t szReserve = 0)
	{
		return MU8ToU8Impl<DynamicString<std::basic_string<U8T>>>(mu8String.data(), mu8String.size(), { szReserve }).GetData();
	}
	static std::basic_string<U8T> MU8ToU8(const MU8T *mu8String, size_t szStringLength, size_t szReserve = 0)
	{
		return MU8ToU8Impl<DynamicString<std::basic_string<U8T>>>(mu8String, szStringLength, { szReserve }).GetData();
	}
};

//辅助调用宏
#define U16CV2MU8(u16String) MUTF8_Tool<>::U16ToMU8(u16String)
#define MU8CV2U16(mu8String) MUTF8_Tool<>::MU8ToU16(mu8String)

#define U8CV2MU8(u8String) MUTF8_Tool<>::U8ToMU8(u8String)
#define MU8CV2U8(mu8String) MUTF8_Tool<>::MU8ToU8(mu8String)

//转换为静态字符串数组
//在mutf-8中，任何字符串结尾\0都会被映射成0xC0 0x80，且保证串中不包含0x00，所以一定程度上可以和c-str（以0结尾）兼容
#define U16TOMU8STR(u16LiteralString) (MUTF8_Tool<>::U16ToMU8<u16LiteralString>())
#define U8TOMU8STR(u8LiteralString) (MUTF8_Tool<>::U8ToMU8<u8LiteralString>())

//英文原文
/*
Modified UTF-8 Strings
The JNI uses modified UTF-8 strings to represent various string types. Modified UTF-8 strings are the same as those used by the Java VM. Modified UTF-8 strings are encoded so that character sequences that contain only non-null ASCII characters can be represented using only one byte per character, but all Unicode characters can be represented.

All characters in the range \u0001 to \u007F are represented by a single byte, as follows:

0xxxxxxx
The seven bits of data in the byte give the value of the character represented.

The null character ('\u0000') and characters in the range '\u0080' to '\u07FF' are represented by a pair of bytes x and y:

x: 110xxxxx
y: 10yyyyyy
The bytes represent the character with the value ((x & 0x1f) << 6) + (y & 0x3f).

Characters in the range '\u0800' to '\uFFFF' are represented by 3 bytes x, y, and z:

x: 1110xxxx
y: 10yyyyyy
z: 10zzzzzz
The character with the value ((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f) is represented by the bytes.

Characters with code points above U+FFFF (so-called supplementary characters) are represented by separately encoding the two surrogate code units of their UTF-16 representation. Each of the surrogate code units is represented by three bytes. This means, supplementary characters are represented by six bytes, u, v, w, x, y, and z:

u: 11101101
v: 1010vvvv
w: 10wwwwww
x: 11101101
y: 1011yyyy
z: 10zzzzzz
The character with the value 0x10000+((v&0x0f)<<16)+((w&0x3f)<<10)+(y&0x0f)<<6)+(z&0x3f) is represented by the six bytes.

The bytes of multibyte characters are stored in the class file in big-endian (high byte first) order.

There are two differences between this format and the standard UTF-8 format. First, the null character (char)0 is encoded using the two-byte format rather than the one-byte format. This means that modified UTF-8 strings never have embedded nulls. Second, only the one-byte, two-byte, and three-byte formats of standard UTF-8 are used. The Java VM does not recognize the four-byte format of standard UTF-8; it uses its own two-times-three-byte format instead.

For more information regarding the standard UTF-8 format, see section 3.9 Unicode Encoding Forms of The Unicode Standard, Version 4.0.
*/

//中文翻译
/*
修改后的 UTF-8 字符串
JNI 使用修改后的 UTF-8 字符串来表示各种字符串类型。修改后的 UTF-8 字符串与 Java VM 所使用的字符串相同。修改后的 UTF-8 字符串经过编码，使得仅包含非空 ASCII 字符的字符序列可以每个字符仅使用一个字节来表示，但所有 Unicode 字符都可以被表示。

范围在 \u0001 到 \u007F 之间的所有字符都由单个字节表示，如下所示：

0xxxxxxx
字节中的七位数据给出了所表示字符的值。

空字符 ('\u0000') 和范围在 '\u0080' 到 '\u07FF' 之间的字符由一对字节 x 和 y 表示：

x: 110xxxxx
y: 10yyyyyy
这些字节表示值为 ((x & 0x1f) << 6) + (y & 0x3f) 的字符。

范围在 '\u0800' 到 '\uFFFF' 之间的字符由三个字节 x、y 和 z 表示：

x: 1110xxxx
y: 10yyyyyy
z: 10zzzzzz
值为 ((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f) 的字符由这些字节表示。

码点高于 U+FFFF 的字符（即所谓的补充字符）通过分别编码其 UTF-16 表示的二个代理码元来表示。每个代理码元由三个字节表示。这意味着，补充字符由六个字节 u、v、w、x、y 和 z 表示：

u: 11101101
v: 1010vvvv
w: 10wwwwww
x: 11101101
y: 1011yyyy
z: 10zzzzzz
值为 0x10000+((v&0x0f)<<16)+((w&0x3f)<<10)+(y&0x0f)<<6)+(z&0x3f) 的字符由这六个字节表示。

多字节字符的字节在类文件中以大端序（高位字节在前）存储。

此格式与标准 UTF-8 格式有二个区别。首先，空字符 (char)0 使用双字节格式而非单字节格式进行编码。这意味着修改后的 UTF-8 字符串永远不会包含嵌入的空字符。其次，仅使用标准 UTF-8 的单字节、双字节和三字节格式。Java VM 不识别标准 UTF-8 的四字节格式；它使用自己的二次三字节格式来代替。

有关标准 UTF-8 格式的更多信息，请参阅 Unicode 标准 4.0 版的第 3.9 节 Unicode 编码形式。
*/