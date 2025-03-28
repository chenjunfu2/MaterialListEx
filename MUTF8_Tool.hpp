#pragma once

#include <string>
#include <type_traits>
#include <assert.h>

template<typename MU8T = char8_t, typename U16T = char16_t>
class MUTF8_Tool
{
	static_assert(sizeof(MU8T) >= 1, "MU8T size must be at least 1 byte");
	static_assert(sizeof(U16T) >= 2, "U16T size must be at least 2 bytes");

private:
	template<size_t szBytes>
	static void EncodeMUTF8Bmp(U16T u16Char, MU8T(&mu8CharArr)[szBytes])
	{
		if constexpr (szBytes == 1)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0111'1111) >>  0) | (uint16_t)0b0000'0000);//0 6-0
		}
		else if constexpr (szBytes == 2)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0111'1100'0000) >>  6) | (uint16_t)0b0110'0000);//110 10-6
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b0010'0000);//10 5-0
		}
		else if constexpr (szBytes == 3)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b1111'0000'0000'0000) >> 12) | (uint16_t)0b1110'0000);//1110 15-12
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'1111'1100'0000) >>  6) | (uint16_t)0b0010'0000);//10 11-6
			mu8CharArr[2] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b0010'0000);//10 5-0
		}
		else
		{
			static_assert(false, "Error szBytes Size");//大小错误
		}
	}

	static void EncodeMUTF8Supplementary(uint32_t u32RawChar, MU8T(&mu8CharArr)[6])//u32RawChar 是u16t的扩展平面组成的
	{
		mu8CharArr[0] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 19-16//20固定为1
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1010'0000);//10 15-10

		mu8CharArr[3] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[4] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1100'0000) >>  6) | (uint16_t)0b1011'0000);//1011 9-6
		mu8CharArr[5] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint16_t)0b0010'0000);//10 5-0
	}



public:
//v=val b=beg e=end 注意范围是左右边界包含关系，而不是普通的左边界包含
#define IN_RANGE(v,b,e) ((uint16_t)(v)>=(uint16_t)(b)&&(uint16_t)(v)<=(uint16_t)(e))
	static std::basic_string<MU8T> U16ToMU8(const std::basic_string<U16T> &u16String)
	{
		std::basic_string<MU8T> mu8String;
		for (auto it = u16String.begin(), end = u16String.end(); it != end; ++it)
		{
			auto u16Char = *it;
			if (IN_RANGE(u16Char, 0x0001, 0x007F))//单字节码点
			{
				MU8T mu8Char[1];
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0080, 0x07FF) || u16Char == 0x0000)//双字节码点，0字节特判
			{
				MU8T mu8Char[2];
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0800, 0xFFFF))//三字节码点or多字节码点
			{
				if (IN_RANGE(u16Char, 0xD800, 0xDBFF))//遇到高代理对
				{
					U16T u16HighSurrogate = u16Char;//保存
					//读取下一个字节
					if (++it == end)
					{
						u16Char = 0xFFFD;//错误，高代理后无数据
						MU8T mu8Char[3];
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
						break;
					}
					u16Char = *it;

					//处理低代理对
					if (!IN_RANGE(u16Char, 0xDC00, 0xDFFF))
					{
						u16Char = 0xFFFD;//错误，高代理后非低代理
						MU8T mu8Char[3];
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
					else
					{
						U16T u16LowSurrogate = u16Char;
						//取出代理对数据并组合10000-10FFFF
						uint32_t u32RawChar = (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000 |//U16代理对实际编码高位 bit 20 == 1
							((uint32_t)u16HighSurrogate & (uint32_t)0b0000'0000'0011'1111) << 10 |//取出高代理的低6位放到高10位上
							((uint32_t)u16LowSurrogate & (uint32_t)0b0000'0011'1111'1111) << 0;//然后取出低代理的低10位与高10位拼接

						//代理对特殊处理：共6字节表示一个实际代理码点
						MU8T mu8Char[6];
						EncodeMUTF8Supplementary(u32RawChar, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
				}
				else
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//遇到低代理对
					{
						u16Char = 0xFFFD;//错误，在高代理之前遇到低代理
					}
					
					MU8T mu8Char[3];
					EncodeMUTF8Bmp(u16Char, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
			}
			else
			{
				//??????????????怎么命中的
				assert(false);
			}
		}

		return mu8String;
	}

	static std::basic_string<U16T> MMU8ToU16(const std::basic_string<MU8T> &mu8String)
	{
		std::basic_string<char16_t> u16String;






		return u16String;
	}


};


/*
4.4.7. The CONSTANT_Utf8_info Structure
The CONSTANT_Utf8_info structure is used to represent constant string values:

CONSTANT_Utf8_info {
	u1 tag;
	u2 length;
	u1 bytes[length];
}
The items of the CONSTANT_Utf8_info structure are as follows:

tag
The tag item has the value CONSTANT_Utf8 (1).

length
The value of the length item gives the number of bytes in the bytes array (not the length of the resulting string).

bytes[]
The bytes array contains the bytes of the string.

No byte may have the value (byte)0.

No byte may lie in the range (byte)0xf0 to (byte)0xff.

String content is encoded in modified UTF-8. Modified UTF-8 strings are encoded so that code point sequences that contain only non-null ASCII characters can be represented using only 1 byte per code point, but all code points in the Unicode codespace can be represented. Modified UTF-8 strings are not null-terminated. The encoding is as follows:

Code points in the range '\u0001' to '\u007F' are represented by a single byte:

Table 4.7.

0	bits 6-0
The 7 bits of data in the byte give the value of the code point represented.

The null code point ('\u0000') and code points in the range '\u0080' to '\u07FF' are represented by a pair of bytes x and y :

Table 4.8.

x:
Table 4.9.

1	1	0	bits 10-6
y:
Table 4.10.

1	0	bits 5-0

The two bytes represent the code point with the value:

((x & 0x1f) << 6) + (y & 0x3f)

Code points in the range '\u0800' to '\uFFFF' are represented by 3 bytes x, y, and z :

Table 4.11.

x:
Table 4.12.

1	1	1	0	bits 15-12
y:
Table 4.13.

1	0	bits 11-6
z:
Table 4.14.

1	0	bits 5-0

The three bytes represent the code point with the value:

((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f)

Characters with code points above U+FFFF (so-called supplementary characters) are represented by separately encoding the two surrogate code units of their UTF-16 representation. Each of the surrogate code units is represented by three bytes. This means supplementary characters are represented by six bytes, u, v, w, x, y, and z :

Table 4.15.

u:
Table 4.16.

1	1	1	0	1	1	0	1
v:
Table 4.17.

1	0	1	0	(bits 20-16)-1
w:
Table 4.18.

1	0	bits 15-10
x:
Table 4.19.

1	1	1	0	1	1	0	1
y:
Table 4.20.

1	0	1	1	bits 9-6
z:
Table 4.21.

1	0	bits 5-0

The six bytes represent the code point with the value:

0x10000 + ((v & 0x0f) << 16) + ((w & 0x3f) << 10) +
((y & 0x0f) << 6) + (z & 0x3f)

The bytes of multibyte characters are stored in the class file in big-endian (high byte first) order.

There are two differences between this format and the "standard" UTF-8 format. First, the null character (char)0 is encoded using the 2-byte format rather than the 1-byte format, so that modified UTF-8 strings never have embedded nulls. Second, only the 1-byte, 2-byte, and 3-byte formats of standard UTF-8 are used. The Java Virtual Machine does not recognize the four-byte format of standard UTF-8; it uses its own two-times-three-byte format instead.

For more information regarding the standard UTF-8 format, see Section 3.9 Unicode Encoding Forms of The Unicode Standard, Version 13.0.
*/

/*
/*
4.4.7. CONSTANT_Utf8_info 结构
CONSTANT_Utf8_info 结构用于表示常量字符串值：

CONSTANT_Utf8_info {
u1 tag;
u2 length;
u1 bytes[length];
}
该结构的成员如下：

tag
tag 项的值为 CONSTANT_Utf8 (1)。

length
length 项的值表示 bytes 数组的字节数（不是字符串长度）。

bytes[]
bytes 数组包含字符串的字节。

字节值禁止为 (byte)0。

字节值禁止在 (byte)0xf0 到 (byte)0xff 范围内。

字符串内容使用修改后的 UTF-8 编码。改进的 UTF-8 编码使得仅包含非空 ASCII 字符的代码点序列可以用每代码点 1 字节表示，同时能表示 Unicode 编码空间的所有代码点。修改后的 UTF-8 字符串不以空字符结尾。编码规则如下：

'\u0001' 到 '\u007F' 范围的代码点使用单字节表示：

表 4.7.

0 位6-0
字节中的7位数据给出代码点的值

空代码点（'\u0000'）和 '\u0080' 到 '\u07FF' 范围的代码点使用两个字节 x 和 y 表示：

表 4.8.

x:
表 4.9.

1 1 0 位10-6
y:
表 4.10.

1 0 位5-0

这两个字节表示的代码点值为：
((x & 0x1f) << 6) + (y & 0x3f)

'\u0800' 到 '\uFFFF' 范围的代码点使用三个字节 x, y 和 z 表示：

表 4.11.

x:
表 4.12.

1 1 1 0 位15-12
y:
表 4.13.

1 0 位11-6
z:
表 4.14.

1 0 位5-0

这三个字节表示的代码点值为：
((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f)

U+FFFF 以上的代码点（补充字符）通过分别编码其 UTF-16 表示的两个代理代码单元实现。每个代理代码单元用三个字节表示，即补充字符最终由六个字节 u, v, w, x, y 和 z 表示：

表 4.15.

u:
表 4.16.

1 1 1 0 1 1 0 1
v:
表 4.17.

1 0 1 0 (位20-16)-1
w:
表 4.18.

1 0 位15-10
x:
表 4.19.

1 1 1 0 1 1 0 1
y:
表 4.20.

1 0 1 1 位9-6
z:
表 4.21.

1 0 位5-0

这六个字节表示的代码点值为：
0x10000 + ((v & 0x0f) << 16) + ((w & 0x3f) << 10) +
((y & 0x0f) << 6) + (z & 0x3f)

多字节字符的字节按大端序（高位字节在前）存储在class文件中。

本格式与"标准"UTF-8格式有两点区别：第一，空字符(char)0使用双字节格式而非单字节格式，因此修改后的UTF-8字符串不会包含嵌入式空值；第二，仅使用标准UTF-8的单字节、双字节和三字节格式。Java虚拟机不识别标准UTF-8的四字节格式，而是使用自定义的双三字节格式。

关于标准UTF-8格式的更多信息，请参阅《The Unicode Standard, Version 13.0》第3.9节"Unicode Encoding Forms"。
*/