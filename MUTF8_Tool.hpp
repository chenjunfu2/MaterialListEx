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
			static_assert(false, "Error szBytes Size");//��С����
		}
	}

	static void EncodeMUTF8Supplementary(uint32_t u32RawChar, MU8T(&mu8CharArr)[6])//u32RawChar ��u16t����չƽ����ɵ�
	{
		mu8CharArr[0] = (uint8_t)0b1110'1101;//�̶��ֽ�
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 19-16//20�̶�Ϊ1
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1010'0000);//10 15-10

		mu8CharArr[3] = (uint8_t)0b1110'1101;//�̶��ֽ�
		mu8CharArr[4] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1100'0000) >>  6) | (uint16_t)0b1011'0000);//1011 9-6
		mu8CharArr[5] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0000'0011'1111) >>  0) | (uint16_t)0b0010'0000);//10 5-0
	}



public:
//v=val b=beg e=end ע�ⷶΧ�����ұ߽������ϵ����������ͨ����߽����
#define IN_RANGE(v,b,e) ((uint16_t)(v)>=(uint16_t)(b)&&(uint16_t)(v)<=(uint16_t)(e))
	static std::basic_string<MU8T> U16ToMU8(const std::basic_string<U16T> &u16String)
	{
		std::basic_string<MU8T> mu8String;
		for (auto it = u16String.begin(), end = u16String.end(); it != end; ++it)
		{
			auto u16Char = *it;
			if (IN_RANGE(u16Char, 0x0001, 0x007F))//���ֽ����
			{
				MU8T mu8Char[1];
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0080, 0x07FF) || u16Char == 0x0000)//˫�ֽ���㣬0�ֽ�����
			{
				MU8T mu8Char[2];
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0800, 0xFFFF))//���ֽ����or���ֽ����
			{
				if (IN_RANGE(u16Char, 0xD800, 0xDBFF))//�����ߴ����
				{
					U16T u16HighSurrogate = u16Char;//����
					//��ȡ��һ���ֽ�
					if (++it == end)
					{
						u16Char = 0xFFFD;//���󣬸ߴ����������
						MU8T mu8Char[3];
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
						break;
					}
					u16Char = *it;

					//����ʹ����
					if (!IN_RANGE(u16Char, 0xDC00, 0xDFFF))
					{
						u16Char = 0xFFFD;//���󣬸ߴ����ǵʹ���
						MU8T mu8Char[3];
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
					else
					{
						U16T u16LowSurrogate = u16Char;
						//ȡ����������ݲ����10000-10FFFF
						uint32_t u32RawChar = (uint32_t)0b0000'0000'0000'0001'0000'0000'0000'0000 |//U16�����ʵ�ʱ����λ bit 20 == 1
							((uint32_t)u16HighSurrogate & (uint32_t)0b0000'0000'0011'1111) << 10 |//ȡ���ߴ���ĵ�6λ�ŵ���10λ��
							((uint32_t)u16LowSurrogate & (uint32_t)0b0000'0011'1111'1111) << 0;//Ȼ��ȡ���ʹ���ĵ�10λ���10λƴ��

						//��������⴦����6�ֽڱ�ʾһ��ʵ�ʴ������
						MU8T mu8Char[6];
						EncodeMUTF8Supplementary(u32RawChar, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
				}
				else
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//�����ʹ����
					{
						u16Char = 0xFFFD;//�����ڸߴ���֮ǰ�����ʹ���
					}
					
					MU8T mu8Char[3];
					EncodeMUTF8Bmp(u16Char, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
			}
			else
			{
				//??????????????��ô���е�
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
4.4.7. CONSTANT_Utf8_info �ṹ
CONSTANT_Utf8_info �ṹ���ڱ�ʾ�����ַ���ֵ��

CONSTANT_Utf8_info {
u1 tag;
u2 length;
u1 bytes[length];
}
�ýṹ�ĳ�Ա���£�

tag
tag ���ֵΪ CONSTANT_Utf8 (1)��

length
length ���ֵ��ʾ bytes ������ֽ����������ַ������ȣ���

bytes[]
bytes ��������ַ������ֽڡ�

�ֽ�ֵ��ֹΪ (byte)0��

�ֽ�ֵ��ֹ�� (byte)0xf0 �� (byte)0xff ��Χ�ڡ�

�ַ�������ʹ���޸ĺ�� UTF-8 ���롣�Ľ��� UTF-8 ����ʹ�ý������ǿ� ASCII �ַ��Ĵ�������п�����ÿ����� 1 �ֽڱ�ʾ��ͬʱ�ܱ�ʾ Unicode ����ռ�����д���㡣�޸ĺ�� UTF-8 �ַ������Կ��ַ���β������������£�

'\u0001' �� '\u007F' ��Χ�Ĵ����ʹ�õ��ֽڱ�ʾ��

�� 4.7.

0 λ6-0
�ֽ��е�7λ���ݸ���������ֵ

�մ���㣨'\u0000'���� '\u0080' �� '\u07FF' ��Χ�Ĵ����ʹ�������ֽ� x �� y ��ʾ��

�� 4.8.

x:
�� 4.9.

1 1 0 λ10-6
y:
�� 4.10.

1 0 λ5-0

�������ֽڱ�ʾ�Ĵ����ֵΪ��
((x & 0x1f) << 6) + (y & 0x3f)

'\u0800' �� '\uFFFF' ��Χ�Ĵ����ʹ�������ֽ� x, y �� z ��ʾ��

�� 4.11.

x:
�� 4.12.

1 1 1 0 λ15-12
y:
�� 4.13.

1 0 λ11-6
z:
�� 4.14.

1 0 λ5-0

�������ֽڱ�ʾ�Ĵ����ֵΪ��
((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f)

U+FFFF ���ϵĴ���㣨�����ַ���ͨ���ֱ������ UTF-16 ��ʾ������������뵥Ԫʵ�֡�ÿ��������뵥Ԫ�������ֽڱ�ʾ���������ַ������������ֽ� u, v, w, x, y �� z ��ʾ��

�� 4.15.

u:
�� 4.16.

1 1 1 0 1 1 0 1
v:
�� 4.17.

1 0 1 0 (λ20-16)-1
w:
�� 4.18.

1 0 λ15-10
x:
�� 4.19.

1 1 1 0 1 1 0 1
y:
�� 4.20.

1 0 1 1 λ9-6
z:
�� 4.21.

1 0 λ5-0

�������ֽڱ�ʾ�Ĵ����ֵΪ��
0x10000 + ((v & 0x0f) << 16) + ((w & 0x3f) << 10) +
((y & 0x0f) << 6) + (z & 0x3f)

���ֽ��ַ����ֽڰ�����򣨸�λ�ֽ���ǰ���洢��class�ļ��С�

����ʽ��"��׼"UTF-8��ʽ���������𣺵�һ�����ַ�(char)0ʹ��˫�ֽڸ�ʽ���ǵ��ֽڸ�ʽ������޸ĺ��UTF-8�ַ����������Ƕ��ʽ��ֵ���ڶ�����ʹ�ñ�׼UTF-8�ĵ��ֽڡ�˫�ֽں����ֽڸ�ʽ��Java�������ʶ���׼UTF-8�����ֽڸ�ʽ������ʹ���Զ����˫���ֽڸ�ʽ��

���ڱ�׼UTF-8��ʽ�ĸ�����Ϣ������ġ�The Unicode Standard, Version 13.0����3.9��"Unicode Encoding Forms"��
*/