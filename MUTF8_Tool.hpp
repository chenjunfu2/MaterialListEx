#pragma once

#include <string>
#include <type_traits>
#include <assert.h>

template<typename MU8T = char8_t, typename U16T = char16_t>
class MUTF8_Tool
{
	static_assert(sizeof(MU8T) == 1, "MU8T size must be at 1 byte");
	static_assert(sizeof(U16T) == 2, "U16T size must be at 2 bytes");

	MUTF8_Tool() = delete;
	~MUTF8_Tool() = delete;
private:
	template<size_t szBytes>
	static void EncodeMUTF8Bmp(const U16T u16Char, MU8T(&mu8CharArr)[szBytes])
	{
		if constexpr (szBytes == 1)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0111'1111) >>  0) | (uint16_t)0b0000'0000);//0 6-0   7bit
		}
		else if constexpr (szBytes == 2)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0111'1100'0000) >>  6) | (uint16_t)0b1100'0000);//110 10-6  5bit
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b1000'0000);//10 5-0   6bit
		}
		else if constexpr (szBytes == 3)
		{
			mu8CharArr[0] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b1111'0000'0000'0000) >> 12) | (uint16_t)0b1110'0000);//1110 15-12   4bit
			mu8CharArr[1] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'1111'1100'0000) >>  6) | (uint16_t)0b1000'0000);//10 11-6   6bit
			mu8CharArr[2] = (uint8_t)((((uint16_t)u16Char & (uint16_t)0b0000'0000'0011'1111) >>  0) | (uint16_t)0b1000'0000);//10 5-0   6bit
		}
		else
		{
			static_assert(false, "Error szBytes Size");//大小错误
		}
	}

	static void EncodeMUTF8Supplementary(const U16T u16HighSurrogate, const U16T u16LowSurrogate,  MU8T(&mu8CharArr)[6])
	{
		//取出代理对数据并组合 范围：100000-1FFFFF
		uint32_t u32RawChar = //(uint32_t)0b0000'0000'0001'0000'0000'0000'0000'0000 |//U16代理对实际编码高位 bit20 -> 1 ignore
							  ((uint32_t)((uint16_t)u16HighSurrogate & (uint16_t)0b0000'0011'1111'1111)) << 10 |//10bit
							  ((uint32_t)((uint16_t)u16LowSurrogate  & (uint16_t)0b0000'0011'1111'1111)) << 0;//10bit

		//高代理
		mu8CharArr[0] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 19-16(20固定为1)   4bit
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1000'0000);//10 15-10   6bit
		//低代理
		mu8CharArr[3] = (uint8_t)0b1110'1101;//固定字节
		mu8CharArr[4] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1100'0000) >>  6) | (uint32_t)0b1011'0000);//1011 9-6   4bit
		mu8CharArr[5] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint32_t)0b1000'0000);//10 5-0   6bit
	}

	template<size_t szBytes>
	static void DecodeMUTF8Bmp(const MU8T(&mu8CharArr)[szBytes], U16T &u16Char)
	{
		if constexpr (szBytes == 1)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0111'1111)) << 0;//7bit
		}
		else if constexpr (szBytes == 2)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0001'1111)) << 6 |//5bit
					  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0011'1111)) << 0;//6bit
		}
		else if constexpr (szBytes == 3)
		{
			u16Char = ((uint16_t)((uint8_t)mu8CharArr[0] & (uint8_t)0b0000'1111)) << 12 |//4bit
					  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0011'1111)) <<  6 |//6bit
					  ((uint16_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) <<  0;//6bit
		}
		else
		{
			static_assert(false, "Error szBytes Size");//大小错误
		}
	}

	static void DecodeMUTF8Supplementary(const MU8T(&mu8CharArr)[6], U16T &u16HighSurrogate, U16T &u16LowSurrogate)
	{
		//忽略mu8CharArr[0]和mu8CharArr[3]（固定字节）
		uint32_t u32RawChar = //(uint16_t)0b0000'0000'0001'0000'0000'0000'0000'0000 |//bit20->1 ignore
												 //mu8CharArr[0] ignore
							  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'1111)) << 16 |//4bit
							  ((uint16_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) << 10 |//6bit
												 //mu8CharArr[3] ignore
							  ((uint16_t)((uint8_t)mu8CharArr[4] & (uint8_t)0b0000'1111)) <<  6 |//4bit
							  ((uint16_t)((uint8_t)mu8CharArr[5] & (uint8_t)0b0011'1111)) <<  0;//6bit

		//解析到高低代理

		u16HighSurrogate = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'1111'1111'1100'0000'0000) >> 10 | (uint32_t)0b1101'1000'0000'0000);
		u16LowSurrogate  = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1111'1111) >>  0 | (uint32_t)0b1101'1100'0000'0000);
	}



public:
//v=val b=beg e=end 注意范围是左右边界包含关系，而不是普通的左边界包含
#define IN_RANGE(v,b,e) ((uint16_t)(v)>=(uint16_t)(b)&&(uint16_t)(v)<=(uint16_t)(e))
	static std::basic_string<MU8T> U16ToMU8(const std::basic_string<U16T> &u16String)
	{
		std::basic_string<MU8T> mu8String{};//字符串结尾为0xC0 0x80而非0x00
		//因为string带长度信息，则不用处理0字符情况，for不会进入，直接返回size为0的mu8str
		//if (u16String.size() == 0)
		//{
		//	mu8String.push_back((uint8_t)0xC0);
		//	mu8String.push_back((uint8_t)0x80);
		//}

		for (auto it = u16String.begin(), end = u16String.end(); it != end; ++it)
		{
			U16T u16Char = *it;
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
						MU8T mu8Char[3];//转换u16未知字符
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
						break;
					}
					else
					{
						u16Char = *it;
					}
					
					//处理低代理对
					if (!IN_RANGE(u16Char, 0xDC00, 0xDFFF))
					{
						u16Char = 0xFFFD;//错误，高代理后非低代理
						MU8T mu8Char[3];//转换u16未知字符
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
					else
					{
						U16T u16LowSurrogate = u16Char;
						//代理对特殊处理：共6字节表示一个实际代理码点
						MU8T mu8Char[6];
						EncodeMUTF8Supplementary(u16HighSurrogate, u16LowSurrogate, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
				}
				else
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//遇到低代理对
					{
						u16Char = 0xFFFD;//错误，在高代理之前遇到低代理
						//下方转换u16未知字符
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
#undef IN_RANGE

//检查迭代器并获取下一个字节（如果可以，否则跳出）
#define GET_NEXTCHAR(c) if (++it == end) { break; } else { c = *it; }
	static std::basic_string<U16T> MU8ToU16(const std::basic_string<MU8T> &mu8String)
	{
		std::basic_string<U16T> u16String{};//字符串末尾为0x00
		//因为string带长度信息，则不用处理0字符情况，for不会进入，直接返回size为0的u16str
		//if (mu8String.size() == 0)
		//{
		//	u16String.push_back((uint16_t)0x00);
		//}

		for (auto it = mu8String.begin(), end = mu8String.end(); it != end; ++it)
		{
			MU8T mu8Char = *it;
			//判断是几字节的mu8
			if ((uint8_t)mu8Char & (uint8_t)0b1000'0000 == (uint8_t)0b0000'0000)//最高位不为1，单字节码点
			{
				//放入数组
				MU8T mu8CharArr[1] = { mu8Char };

				//转换
				U16T u16Char;
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if((uint8_t)mu8Char & (uint8_t)0b1110'0000 == (uint8_t)0b1100'0000)//高3位中最高两位为1，双字节码点
			{
				//先保存第一个字节
				MU8T mu8CharArr[2] = { mu8Char };//[0]=mu8Char
				//尝试获取下一个字节
				GET_NEXTCHAR(mu8Char);
				//判断字节合法性
				if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)//高2位不是10，错误，跳过
				{
					--it;//撤回读取（避免for自动递增跳过刚才的字符）
					continue;//重试，因为当前字符可能是错误的，而刚才多读取的才是正确的，所以需要撤回continue重新尝试
				}
				//放入数组
				mu8CharArr[1] = mu8Char;

				//转换
				U16T u16Char;
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if ((uint8_t)mu8Char & (uint8_t)0b1111'0000 == (uint8_t)0b1110'0000)//高4位为1110，三字节或多字节码点
			{
				//保存
				MU8T mu8First = mu8Char;
				//尝试获取下一个字节
				GET_NEXTCHAR(mu8Char);
				//合法性判断（区分是否为代理）
				//代理区分：因为D800开头的为高代理，必不可能作为三字节码点0b1010'xxxx出现，所以只要高4位是1010必为代理对
				//也就是说mu8CharArr3[0]的低4bit如果是1101并且mu8Char的高4bit是1010的情况下，即三字节码点10xx'xxxx中的最高两个xx为01，
				//把他们合起来就是1101'10xx 也就是0xD8，即u16的高代理对开始字符，而代理对在encode过程走的另一个流程，不存在与3字节码点混淆处理的情况
				if (mu8First == (uint8_t)0b1110'1101 && ((uint8_t)mu8Char & (uint8_t)0b1111'0000 == (uint8_t)0b1010'0000))//代理对，必须先判断，很重要！
				{
					MU8T mu8CharArr[6] = { mu8First,mu8Char };//0 1已初始化，0是固定起始值，1是高代理的高4位
					//继续读取后4个并验证

					//下一个为高代理的低6位
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)
					{
						--it;//撤回一次读取（为什么不是两次？因为前一个字符已确认高2bit为10，没有以10开头的存在，跳过）
						continue;
					}
					mu8CharArr[2] = mu8Char;
					//下一个为固定字符
					GET_NEXTCHAR(mu8Char);
					if (mu8Char != (uint8_t)0b1110'1101)
					{
						--it;//撤回一次读取（为什么不是两次？因为前一个字符已确认高2bit为10，没有以10开头的存在，跳过）
						continue;
					}
					mu8CharArr[3] = mu8Char;
					//下一个为低代理高4位
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1111'0000 != (uint8_t)0b1011'0000)
					{
						--it;//撤回两次读取，尽管前面已确认是0b1110'1101，但是存在111开头的合法3码点
						--it;
						continue;
					}
					mu8CharArr[4] = mu8Char;
					//读取最后一个低代理的低6位
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)
					{
						--it;//撤回一次读取，因为不存在而前一个已确认的101开头的合法码点，且再前一个开头为111，不存在111后跟101的3码点情况，跳过
						continue;
					}
					mu8CharArr[5] = mu8Char;

					//验证全部通过，转换代理对
					U16T u16HighSurrogate, u16LowSurrogate;
					DecodeMUTF8Supplementary(mu8CharArr, u16HighSurrogate, u16LowSurrogate);
					u16String.push_back(u16HighSurrogate);
					u16String.push_back(u16LowSurrogate);
				}
				else if((uint8_t)mu8Char & (uint8_t)0b1100'0000 == (uint8_t)0b1000'0000)//三字节码点，排除代理对后只有这个可能
				{
					//保存
					MU8T mu8CharArr[3] = { mu8First,mu8Char };
					GET_NEXTCHAR(mu8Char);//尝试获取下一字符
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)//错误，3字节码点最后一个不是正确字符
					{
						--it;//撤回一次读取（为什么不是两次？因为前一个字符已确认高2bit为10，没有以10开头的存在，跳过）
						continue;
					}
					mu8CharArr[2] = mu8Char;

					//3位已就绪，转换
					U16T u16Char;
					DecodeMUTF8Bmp(mu8CharArr, u16Char);
					u16String.push_back(u16Char);
				}
				else
				{
					--it;//撤回刚才的读取并重试
					continue;
				}

			}
			else
			{
				//未知，跳过并忽略，直到遇到下一个正确起始字符
				continue;
			}
		}
	
		return u16String;
	}
#undef GET_NEXTCHAR

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