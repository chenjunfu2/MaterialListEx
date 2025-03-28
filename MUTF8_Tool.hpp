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
			static_assert(false, "Error szBytes Size");//��С����
		}
	}

	static void EncodeMUTF8Supplementary(const U16T u16HighSurrogate, const U16T u16LowSurrogate,  MU8T(&mu8CharArr)[6])
	{
		//ȡ����������ݲ���� ��Χ��100000-1FFFFF
		uint32_t u32RawChar = //(uint32_t)0b0000'0000'0001'0000'0000'0000'0000'0000 |//U16�����ʵ�ʱ����λ bit20 -> 1 ignore
							  ((uint32_t)((uint16_t)u16HighSurrogate & (uint16_t)0b0000'0011'1111'1111)) << 10 |//10bit
							  ((uint32_t)((uint16_t)u16LowSurrogate  & (uint16_t)0b0000'0011'1111'1111)) << 0;//10bit

		//�ߴ���
		mu8CharArr[0] = (uint8_t)0b1110'1101;//�̶��ֽ�
		mu8CharArr[1] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'1111'0000'0000'0000'0000) >> 16) | (uint32_t)0b1010'0000);//1010 19-16(20�̶�Ϊ1)   4bit
		mu8CharArr[2] = (uint8_t)(((u32RawChar & (uint32_t)0b0000'0000'0000'0000'1111'1100'0000'0000) >> 10) | (uint32_t)0b1000'0000);//10 15-10   6bit
		//�ʹ���
		mu8CharArr[3] = (uint8_t)0b1110'1101;//�̶��ֽ�
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
			static_assert(false, "Error szBytes Size");//��С����
		}
	}

	static void DecodeMUTF8Supplementary(const MU8T(&mu8CharArr)[6], U16T &u16HighSurrogate, U16T &u16LowSurrogate)
	{
		//����mu8CharArr[0]��mu8CharArr[3]���̶��ֽڣ�
		uint32_t u32RawChar = //(uint16_t)0b0000'0000'0001'0000'0000'0000'0000'0000 |//bit20->1 ignore
												 //mu8CharArr[0] ignore
							  ((uint16_t)((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'1111)) << 16 |//4bit
							  ((uint16_t)((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1111)) << 10 |//6bit
												 //mu8CharArr[3] ignore
							  ((uint16_t)((uint8_t)mu8CharArr[4] & (uint8_t)0b0000'1111)) <<  6 |//4bit
							  ((uint16_t)((uint8_t)mu8CharArr[5] & (uint8_t)0b0011'1111)) <<  0;//6bit

		//�������ߵʹ���

		u16HighSurrogate = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'1111'1111'1100'0000'0000) >> 10 | (uint32_t)0b1101'1000'0000'0000);
		u16LowSurrogate  = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1111'1111) >>  0 | (uint32_t)0b1101'1100'0000'0000);
	}



public:
//v=val b=beg e=end ע�ⷶΧ�����ұ߽������ϵ����������ͨ����߽����
#define IN_RANGE(v,b,e) ((uint16_t)(v)>=(uint16_t)(b)&&(uint16_t)(v)<=(uint16_t)(e))
	static std::basic_string<MU8T> U16ToMU8(const std::basic_string<U16T> &u16String)
	{
		std::basic_string<MU8T> mu8String{};//�ַ�����βΪ0xC0 0x80����0x00
		//��Ϊstring��������Ϣ�����ô���0�ַ������for������룬ֱ�ӷ���sizeΪ0��mu8str
		//if (u16String.size() == 0)
		//{
		//	mu8String.push_back((uint8_t)0xC0);
		//	mu8String.push_back((uint8_t)0x80);
		//}

		for (auto it = u16String.begin(), end = u16String.end(); it != end; ++it)
		{
			U16T u16Char = *it;
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
						MU8T mu8Char[3];//ת��u16δ֪�ַ�
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
						break;
					}
					else
					{
						u16Char = *it;
					}
					
					//����ʹ����
					if (!IN_RANGE(u16Char, 0xDC00, 0xDFFF))
					{
						u16Char = 0xFFFD;//���󣬸ߴ����ǵʹ���
						MU8T mu8Char[3];//ת��u16δ֪�ַ�
						EncodeMUTF8Bmp(u16Char, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
					else
					{
						U16T u16LowSurrogate = u16Char;
						//��������⴦����6�ֽڱ�ʾһ��ʵ�ʴ������
						MU8T mu8Char[6];
						EncodeMUTF8Supplementary(u16HighSurrogate, u16LowSurrogate, mu8Char);
						mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
					}
				}
				else
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//�����ʹ����
					{
						u16Char = 0xFFFD;//�����ڸߴ���֮ǰ�����ʹ���
						//�·�ת��u16δ֪�ַ�
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
#undef IN_RANGE

//������������ȡ��һ���ֽڣ�������ԣ�����������
#define GET_NEXTCHAR(c) if (++it == end) { break; } else { c = *it; }
	static std::basic_string<U16T> MU8ToU16(const std::basic_string<MU8T> &mu8String)
	{
		std::basic_string<U16T> u16String{};//�ַ���ĩβΪ0x00
		//��Ϊstring��������Ϣ�����ô���0�ַ������for������룬ֱ�ӷ���sizeΪ0��u16str
		//if (mu8String.size() == 0)
		//{
		//	u16String.push_back((uint16_t)0x00);
		//}

		for (auto it = mu8String.begin(), end = mu8String.end(); it != end; ++it)
		{
			MU8T mu8Char = *it;
			//�ж��Ǽ��ֽڵ�mu8
			if ((uint8_t)mu8Char & (uint8_t)0b1000'0000 == (uint8_t)0b0000'0000)//���λ��Ϊ1�����ֽ����
			{
				//��������
				MU8T mu8CharArr[1] = { mu8Char };

				//ת��
				U16T u16Char;
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if((uint8_t)mu8Char & (uint8_t)0b1110'0000 == (uint8_t)0b1100'0000)//��3λ�������λΪ1��˫�ֽ����
			{
				//�ȱ����һ���ֽ�
				MU8T mu8CharArr[2] = { mu8Char };//[0]=mu8Char
				//���Ի�ȡ��һ���ֽ�
				GET_NEXTCHAR(mu8Char);
				//�ж��ֽںϷ���
				if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)//��2λ����10����������
				{
					--it;//���ض�ȡ������for�Զ����������ղŵ��ַ���
					continue;//���ԣ���Ϊ��ǰ�ַ������Ǵ���ģ����ղŶ��ȡ�Ĳ�����ȷ�ģ�������Ҫ����continue���³���
				}
				//��������
				mu8CharArr[1] = mu8Char;

				//ת��
				U16T u16Char;
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if ((uint8_t)mu8Char & (uint8_t)0b1111'0000 == (uint8_t)0b1110'0000)//��4λΪ1110�����ֽڻ���ֽ����
			{
				//����
				MU8T mu8First = mu8Char;
				//���Ի�ȡ��һ���ֽ�
				GET_NEXTCHAR(mu8Char);
				//�Ϸ����жϣ������Ƿ�Ϊ����
				//�������֣���ΪD800��ͷ��Ϊ�ߴ����ز�������Ϊ���ֽ����0b1010'xxxx���֣�����ֻҪ��4λ��1010��Ϊ�����
				//Ҳ����˵mu8CharArr3[0]�ĵ�4bit�����1101����mu8Char�ĸ�4bit��1010������£������ֽ����10xx'xxxx�е��������xxΪ01��
				//�����Ǻ���������1101'10xx Ҳ����0xD8����u16�ĸߴ���Կ�ʼ�ַ������������encode�����ߵ���һ�����̣���������3�ֽ���������������
				if (mu8First == (uint8_t)0b1110'1101 && ((uint8_t)mu8Char & (uint8_t)0b1111'0000 == (uint8_t)0b1010'0000))//����ԣ��������жϣ�����Ҫ��
				{
					MU8T mu8CharArr[6] = { mu8First,mu8Char };//0 1�ѳ�ʼ����0�ǹ̶���ʼֵ��1�Ǹߴ���ĸ�4λ
					//������ȡ��4������֤

					//��һ��Ϊ�ߴ���ĵ�6λ
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)
					{
						--it;//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						continue;
					}
					mu8CharArr[2] = mu8Char;
					//��һ��Ϊ�̶��ַ�
					GET_NEXTCHAR(mu8Char);
					if (mu8Char != (uint8_t)0b1110'1101)
					{
						--it;//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						continue;
					}
					mu8CharArr[3] = mu8Char;
					//��һ��Ϊ�ʹ����4λ
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1111'0000 != (uint8_t)0b1011'0000)
					{
						--it;//�������ζ�ȡ������ǰ����ȷ����0b1110'1101�����Ǵ���111��ͷ�ĺϷ�3���
						--it;
						continue;
					}
					mu8CharArr[4] = mu8Char;
					//��ȡ���һ���ʹ���ĵ�6λ
					GET_NEXTCHAR(mu8Char);
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)
					{
						--it;//����һ�ζ�ȡ����Ϊ�����ڶ�ǰһ����ȷ�ϵ�101��ͷ�ĺϷ���㣬����ǰһ����ͷΪ111��������111���101��3������������
						continue;
					}
					mu8CharArr[5] = mu8Char;

					//��֤ȫ��ͨ����ת�������
					U16T u16HighSurrogate, u16LowSurrogate;
					DecodeMUTF8Supplementary(mu8CharArr, u16HighSurrogate, u16LowSurrogate);
					u16String.push_back(u16HighSurrogate);
					u16String.push_back(u16LowSurrogate);
				}
				else if((uint8_t)mu8Char & (uint8_t)0b1100'0000 == (uint8_t)0b1000'0000)//���ֽ���㣬�ų�����Ժ�ֻ���������
				{
					//����
					MU8T mu8CharArr[3] = { mu8First,mu8Char };
					GET_NEXTCHAR(mu8Char);//���Ի�ȡ��һ�ַ�
					if ((uint8_t)mu8Char & (uint8_t)0b1100'0000 != (uint8_t)0b1000'0000)//����3�ֽ�������һ��������ȷ�ַ�
					{
						--it;//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						continue;
					}
					mu8CharArr[2] = mu8Char;

					//3λ�Ѿ�����ת��
					U16T u16Char;
					DecodeMUTF8Bmp(mu8CharArr, u16Char);
					u16String.push_back(u16Char);
				}
				else
				{
					--it;//���ظղŵĶ�ȡ������
					continue;
				}

			}
			else
			{
				//δ֪�����������ԣ�ֱ��������һ����ȷ��ʼ�ַ�
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