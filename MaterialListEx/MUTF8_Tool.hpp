#pragma once

#include <string>
#include <type_traits>
#include <assert.h>
#include <stdint.h>
#include <array>
#include <algorithm>

template<typename MU8T = uint8_t, typename U16T = char16_t, typename U8T = char8_t>
class MUTF8_Tool
{
	static_assert(sizeof(MU8T) == 1, "MU8T size must be at 1 byte");
	static_assert(sizeof(U16T) == 2, "U16T size must be at 2 bytes");
	static_assert(sizeof(U8T) == 1, "U8T size must be at 1 bytes");

	MUTF8_Tool(void) = delete;
	~MUTF8_Tool(void) = delete;
public:
	using MU8_T = MU8T;
	using U16_T = U16T;
	using U8_T = U8T;
private:
	template<size_t szBytes>
	static constexpr void EncodeMUTF8Bmp(const U16T u16Char, MU8T(&mu8CharArr)[szBytes])
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

	static constexpr void EncodeMUTF8Supplementary(const U16T u16HighSurrogate, const U16T u16LowSurrogate,  MU8T(&mu8CharArr)[6])
	{
		//ȡ����������ݲ���� ��Χ��10'0000-1F'FFFF ע����Ȼӳ���˳���10'FFFF�����򣬵���u16�����ڴ���10'FFFF����㣬�������λ��Ϊ10
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
	static constexpr void DecodeMUTF8Bmp(const MU8T(&mu8CharArr)[szBytes], U16T &u16Char)
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

	static constexpr void DecodeMUTF8Supplementary(const MU8T(&mu8CharArr)[6], U16T &u16HighSurrogate, U16T &u16LowSurrogate)
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
		//��Χ10'0000-1F'FFFFע����Ȼ�������10~1F��λ��Χ������u16��׼�涨���ֵ��10'0000-10'FFFF��Ҳ���Ǹ�λ��Ϊ10��������Ȼ�������㣬����
		u16HighSurrogate = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'1111'1111'1100'0000'0000) >> 10 | (uint32_t)0b1101'1000'0000'0000);
		u16LowSurrogate  = (uint32_t)((u32RawChar & (uint32_t)0b0000'0000'0000'0000'0000'0011'1111'1111) >>  0 | (uint32_t)0b1101'1100'0000'0000);
	}

private:
//c=char d=do������������ȡ��һ���ֽڣ�������ԣ�����ִ��ָ������������
#define GET_NEXTCHAR(c,d) if (++it == end) { (d);break; } else { c = *it; }
//v=value m=mask p=pattern t=test ��������λ֮��Ľ���Ƿ���ָ��ֵ��ֵ�Ƿ�����ָ��bits���
#define HAS_BITMASK(v,m,p) (((uint8_t)(v) & (uint8_t)(m)) == (uint8_t)(p))
#define IS_BITS(v,t) ((uint8_t)(v) == (uint8_t)(t))
//v=value b=begin e=end ע�ⷶΧ�����ұ߽������ϵ����������ͨ����߽����
#define IN_RANGE(v,b,e) (((uint16_t)(v) >= (uint16_t)(b)) && ((uint16_t)(v) <= (uint16_t)(e)))

	template<typename T = std::basic_string<MU8T>>
	static constexpr T U16ToMU8Impl(const U16T *u16String, size_t szStringLength)
	{
		//�����ڴ���������utf16�����ֽ�0xFFFD��mu8��ʽ
		MU8T mu8FailChar[3]{};
		EncodeMUTF8Bmp(0xFFFD, mu8FailChar);
#define PUSH_FAIL_MU8CHAR mu8String.append(mu8FailChar, sizeof(mu8FailChar) / sizeof(MU8T))

		//��Ϊstring��������Ϣ�����ô���0�ַ������for������룬ֱ�ӷ���sizeΪ0��mu8str
		T mu8String{};//�ַ�����βΪ0xC0 0x80����0x00

		for (auto it = u16String, end = u16String + szStringLength; it != end; ++it)
		{
			U16T u16Char = *it;//��һ��
			if (IN_RANGE(u16Char, 0x0001, 0x007F))//���ֽ����
			{
				MU8T mu8Char[1]{};
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0080, 0x07FF) || u16Char == 0x0000)//˫�ֽ���㣬0�ֽ�����
			{
				MU8T mu8Char[2]{};
				EncodeMUTF8Bmp(u16Char, mu8Char);
				mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
			}
			else if (IN_RANGE(u16Char, 0x0800, 0xFFFF))//���ֽ����or���ֽ����
			{
				if (IN_RANGE(u16Char, 0xD800, 0xDBFF))//�����ߴ����
				{	
					U16T u16HighSurrogate = u16Char;//����ߴ���
					U16T u16LowSurrogate{};//��ȡ�ʹ���
					GET_NEXTCHAR(u16LowSurrogate, PUSH_FAIL_MU8CHAR);//�ڶ���
					//��������ȡ��ǰ���أ���ߴ���������ݣ�����ת�����u16δ֪�ַ�
					
					//�жϵʹ���Χ
					if (!IN_RANGE(u16LowSurrogate, 0xDC00, 0xDFFF))//���󣬸ߴ����ǵʹ���
					{
						--it;//����һ�θղŵĶ�ȡ�������жϷǵʹ����ֽ�
						PUSH_FAIL_MU8CHAR;//����u16δ֪�ַ�
						continue;//���ԣ�for������++it���൱�����Ե�ǰ*it
					}

					//��������⴦����6�ֽڱ�ʾһ��ʵ�ʴ������
					MU8T mu8Char[6]{};
					EncodeMUTF8Supplementary(u16HighSurrogate, u16LowSurrogate, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
				else//�ߴ���֮ǰ�����ʹ���������Ϸ�3�ֽ��ַ�
				{
					if (IN_RANGE(u16Char, 0xDC00, 0xDFFF))//�ڸߴ���֮ǰ�����ʹ���
					{
						//�����ض�ȡ����������ĵʹ���
						PUSH_FAIL_MU8CHAR;//���󣬲���u16δ֪�ַ�
						continue;//����
					}

					//ת��3�ֽ��ַ�
					MU8T mu8Char[3]{};
					EncodeMUTF8Bmp(u16Char, mu8Char);
					mu8String.append(mu8Char, sizeof(mu8Char) / sizeof(MU8T));
				}
			}
			else
			{
				assert(false);//??????????????��ô���е�
			}
		}

		return mu8String;

#undef PUSH_FAIL_MU8CHAR
	}


	template<typename T = std::basic_string<U16T>>
	static constexpr T MU8ToU16Impl(const MU8T *mu8String, size_t szStringLength)
	{
#define PUSH_FAIL_U16CHAR u16String.push_back((U16T)0xFFFD)

		//��Ϊstring��������Ϣ�����ô���0�ַ������for������룬ֱ�ӷ���sizeΪ0��u16str
		T u16String{};//�ַ���ĩβΪ0x0000
		
		for (auto it = mu8String, end = mu8String + szStringLength; it != end; ++it)
		{
			MU8T mu8Char = *it;//��һ��
			//�ж��Ǽ��ֽڵ�mu8
			if (HAS_BITMASK(mu8Char, 0b1000'0000, 0b0000'0000))//���λΪ0�����ֽ����
			{
				//��������
				MU8T mu8CharArr[1] = { mu8Char };

				//ת��
				U16T u16Char{};
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if (HAS_BITMASK(mu8Char, 0b1110'0000, 0b1100'0000))//��3λΪ110��˫�ֽ����
			{
				//�ȱ����һ���ֽ�
				MU8T mu8CharArr[2] = { mu8Char };//[0]=mu8Char
				//���Ի�ȡ��һ���ֽ�
				GET_NEXTCHAR(mu8CharArr[1], PUSH_FAIL_U16CHAR);//�ڶ���
				//�ж��ֽںϷ���
				if (!HAS_BITMASK(mu8CharArr[1], 0b1100'0000, 0b1000'0000))//��2λ����10����������
				{
					--it;//���ض�ȡ������for�Զ����������ղŵ��ַ���
					u16String.push_back((U16T)0xFFFD);//�滻Ϊutf16�����ַ�
					continue;//���ԣ���Ϊ��ǰ�ַ������Ǵ���ģ����ղŶ��ȡ�Ĳ�����ȷ�ģ�������Ҫ����continue���³���
				}

				//ת��
				U16T u16Char{};
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if (HAS_BITMASK(mu8Char, 0b1111'0000, 0b1110'0000))//��4λΪ1110�����ֽڻ���ֽ����
			{
				//����
				MU8T mu8Next{};
				//���Ի�ȡ��һ���ֽ�
				GET_NEXTCHAR(mu8Next, PUSH_FAIL_U16CHAR);//�ڶ���
				//�Ϸ����жϣ������Ƿ�Ϊ����
				//�������֣���ΪD800��ͷ��Ϊ�ߴ����ز�������Ϊ���ֽ����0b1010'xxxx���֣�����ֻҪ��4λ��1010��Ϊ�����
				//Ҳ����˵mu8CharArr3[0]�ĵ�4bit�����1101����mu8Char�ĸ�4bit��1010������£������ֽ����10xx'xxxx�е��������xxΪ01��
				//�����Ǻ���������1101'10xx Ҳ����0xD8����u16�ĸߴ���Կ�ʼ�ַ������������encode�����ߵ���һ�����̣���������3�ֽ���������������
				if (IS_BITS(mu8Char, 0b1110'1101) && HAS_BITMASK(mu8Next, 0b1111'0000, 0b1010'0000))//����ԣ��������жϣ�����Ҫ��
				{
					MU8T mu8CharArr[6] = { mu8Char,mu8Next };//0 1�ѳ�ʼ����0�ǹ̶���ʼֵ��1�Ǹߴ���ĸ�4λ
					//������ȡ��4������֤

					//��һ��Ϊ�ߴ���ĵ�6λ
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//������
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))
					{
						//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						--it;
						//�滻Ϊ����utf16�����ַ�
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//��һ��Ϊ�̶��ַ�
					GET_NEXTCHAR(mu8CharArr[3],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//���Ĵ�
					if (!IS_BITS(mu8CharArr[3], 0b1110'1101))
					{
						//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						--it;
						//�滻Ϊ����utf16�����ַ�
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//��һ��Ϊ�ʹ����4λ
					GET_NEXTCHAR(mu8CharArr[4],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//�����
					if (!HAS_BITMASK(mu8CharArr[4], 0b1111'0000, 0b1011'0000))
					{
						//�������ζ�ȡ������ǰ����ȷ����0b1110'1101�����Ǵ���111��ͷ�ĺϷ�3���
						--it;
						--it;
						//�滻Ϊ����utf16�����ַ�����Ϊ�������Σ�������4�������ֽڵ�����ֻҪ3��
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//��ȡ���һ���ʹ���ĵ�6λ
					GET_NEXTCHAR(mu8CharArr[5],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//������
					if (!HAS_BITMASK(mu8CharArr[5], 0b1100'0000, 0b1000'0000))
					{
						//����һ�ζ�ȡ����Ϊ�����ڶ�ǰһ����ȷ�ϵ�101��ͷ�ĺϷ���㣬����ǰһ����ͷΪ111��������111���101��3������������
						--it;
						//�滻Ϊ���utf16�����ַ�
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//��֤ȫ��ͨ����ת�������
					U16T u16HighSurrogate{}, u16LowSurrogate{};
					DecodeMUTF8Supplementary(mu8CharArr, u16HighSurrogate, u16LowSurrogate);
					u16String.push_back(u16HighSurrogate);
					u16String.push_back(u16LowSurrogate);
				}
				else if (HAS_BITMASK(mu8Next, 0b1100'0000, 0b1000'0000))//���ֽ���㣬�ų�����Ժ�ֻ���������
				{
					//����
					MU8T mu8CharArr[3] = { mu8Char,mu8Next };//0 1�ѳ�ʼ��
					//���Ի�ȡ��һ�ַ�
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//������
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))//����3�ֽ�������һ��������ȷ�ַ�
					{
						//����һ�ζ�ȡ��Ϊʲô�������Σ���Ϊǰһ���ַ���ȷ�ϸ�2bitΪ10��û����10��ͷ�Ĵ��ڣ�������
						--it;
						//�滻Ϊ����utf16�����ַ�
						PUSH_FAIL_U16CHAR;
						PUSH_FAIL_U16CHAR;
						continue;
					}

					//3λ�Ѿ�����ת��
					U16T u16Char{};
					DecodeMUTF8Bmp(mu8CharArr, u16Char);
					u16String.push_back(u16Char);
				}
				else
				{
					//����mu8Next�Ķ�ȡ����Ϊmu8Char�Ѿ��жϹ������֤�����ֽڴ���
					//������ص�mu8Char�ᵼ�����޴���ѭ����ֻ���ص�mu8Next����
					//���ظղŵĶ�ȡ�����ԣ�for������++it���൱�����Ե�ǰ*it
					--it;
					//�滻Ϊһ��utf16�����ַ�
					PUSH_FAIL_U16CHAR;
					continue;
				}
			}
			else
			{
				//δ֪�����������ԣ�ֱ��������һ����ȷ��ʼ�ַ�
				//�滻Ϊһ��utf16�����ַ�
				PUSH_FAIL_U16CHAR;
				continue;
			}
		}
	
		return u16String;

#undef PUSH_FAIL_U16CHAR
	}

	/*
	Modified UTF-8 �� "��׼"UTF-8 ��ʽ����������
	��һ�����ַ�(char)0ʹ��˫�ֽڸ�ʽ0xC0 0x80���ǵ��ֽڸ�ʽ0x00��
	��� Modified UTF-8�ַ����������Ƕ��ʽ��ֵ��

	�ڶ�����ʹ�ñ�׼UTF-8�ĵ��ֽڡ�˫�ֽں����ֽڸ�ʽ��
	Java�������ʶ���׼UTF-8�����ֽڸ�ʽ��
	����ʹ���Զ����˫���ֽڣ�6�ֽڴ���ԣ���ʽ��
	*/

	template<typename T = std::basic_string<MU8T>>
	static constexpr T U8ToMU8Impl(const U8T *u8String, size_t szStringLength)
	{
#define INSERT_NORMAL(p) (mu8String.append((const MU8T*)((p) - szNormalLength), szNormalLength), szNormalLength = 0)
		
		//���ڱ���ת���õ�mu8String
		T mu8String{};

		size_t szNormalLength = 0;//��ͨ�ַ��ĳ��ȣ������Ż���������
		for (auto it = u8String, end = u8String + szStringLength; it != end; ++it)
		{
			//u8��mu8������u8���ַ�������4�ֽ�u8ת����6�ֽ�mu8
			U8T u8Char = *it;//��һ��
			if (HAS_BITMASK(u8Char, 0b1111'1000, 0b1111'0000))//��5λΪ11110��utf8��4�ֽ�
			{
				INSERT_NORMAL(it);//�ڴ���֮ǰ�Ȳ���֮ǰ����������ͨ�ַ�
				//ת��u8��4�ֽڵ�mu8��6�ֽڣ����������


			}
			else if (IS_BITS(u8Char, 0b0000'0000))//\0�ַ�
			{
				INSERT_NORMAL(it);//�ڴ���֮ǰ�Ȳ���֮ǰ����������ͨ�ַ�

				MU8T mu8CharArr[2] = { (MU8T)0xC0,(MU8T)0x80 };//mu8�̶�0�ֽ�
				mu8String.append(mu8CharArr, sizeof(mu8CharArr) / sizeof(MU8T));
			}
			else//�����ǣ�������ͨ�ַ����ȣ�ֱ�����������ַ���ʱ�����
			{
				++szNormalLength;
			}
		}
		//�������ٲ���һ�Σ���Ϊfor�ڿ�����ȫû�н�����κ�һ������飬
		//����Ϊ������for�Ǵ�ĩβ�˳��ģ����Դ�ĩβ��ʼ��Ϊ��ǰָ�����
		INSERT_NORMAL(u8String + szStringLength);


		return mu8String;
#undef INSERT_NORMAL
	}

	template<typename T = std::basic_string<U8T>>
	static constexpr T MU8ToU8Impl(const MU8T *mu8String, size_t szStringLength)
	{
		T u8String{};

		for (auto it = mu8String, end = mu8String + szStringLength; it != end; ++it)
		{
			MU8T mu8Char = *it;//��һ��







		}

		return u8String;
	}

#undef IN_RANGE
#undef IS_BITS
#undef HAS_BITMASK
#undef GET_NEXTCHAR

private:
	//����ħ���࣬αװ��basic string���ڲ����ʱ��������ݳ��ȼ��������Բ�������ݣ����ת��Ϊsize_t����
	//������������С�޸ĵ��������ͬһ��������ģ������ȡת����ĳ��ȣ���100%׼ȷ������������дһ������
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

		constexpr void push_back(const T &) noexcept
		{
			szCounter += 1;
		}

		constexpr size_t GetData(void) const noexcept
		{
			return szCounter;
		}
	};

	//ħ����2��αװ��string��ת������̬�ַ�����Ϊstd::array����
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

		constexpr StaticString &append(const T *const pData, size_t szSize) noexcept
		{
			if (szIndex + szSize > arrData.size())
			{
				return *this;
			}

			std::ranges::copy(pData, &pData[szSize], &arrData[szIndex]);
			szIndex += szSize;

			//for (size_t i = 0; i < szSize; ++i, ++szIndex)
			//{
			//	arrData[szIndex] = pData[i];
			//}
			return *this;
		}

		constexpr void push_back(const T &tData) noexcept
		{
			if (szIndex + 1 > arrData.size())
			{
				return;
			}

			arrData[szIndex] = tData;
			szIndex += 1;
		}

		constexpr ARRAY_TYPE GetData(void) const noexcept
		{
			return arrData;
		}
	};

public:
	static constexpr size_t U16ToMU8Size(const std::basic_string_view<U16T> &u16String)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String.data(), u16String.size()).GetData();
	}
	static constexpr size_t U16ToMU8Size(const U16T *u16String, size_t szStringLength)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String, szStringLength).GetData();
	}
	template<size_t N>
	static consteval size_t U16ToMU8Size(const U16T(&u16String)[N])
	{
		size_t szStringLength =
			N > 0 && u16String[N - 1] == u'\0'
			? N - 1
			: N;

		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String, szStringLength).GetData();
	}

	static std::basic_string<MU8T> U16ToMU8(const std::basic_string_view<U16T> &u16String)
	{
		return U16ToMU8Impl<std::basic_string<MU8T>>(u16String.data(), u16String.size());
	}
	static std::basic_string<MU8T> U16ToMU8(const U16T *u16String, size_t szStringLength)
	{
		return U16ToMU8Impl<std::basic_string<MU8T>>(u16String, szStringLength);
	}
	template<size_t szNewSize, size_t N>//size_t szNewSize = U16ToMU8Size(u16String);
	static consteval auto U16ToMU8(const U16T(&u16String)[N])
	{
		size_t szStringLength = 
			N > 0 && u16String[N - 1] == u'\0'
			? N - 1 
			: N;
	
		return U16ToMU8Impl<StaticString<MU8T, szNewSize>>(u16String, szStringLength).GetData();
	}

	//---------------------------------------------------------------------------------------------//

	static constexpr size_t MU8ToU16Size(const std::basic_string_view<MU8T> &mu8String)
	{
		return MU8ToU16Impl<FakeStringCounter<U16T>>(mu8String.data(), mu8String.size()).GetData();
	}
	static constexpr size_t MU8ToU16Size(const MU8T *mu8String, size_t szStringLength)
	{
		return MU8ToU16Impl<FakeStringCounter<U16T>>(mu8String, szStringLength).GetData();
	}

	static std::basic_string<U16T> MU8ToU16(const std::basic_string_view<MU8T> &mu8String)
	{
		return MU8ToU16Impl<std::basic_string<U16T>>(mu8String.data(), mu8String.size());
	}
	static std::basic_string<U16T> MU8ToU16(const MU8T *mu8String, size_t szStringLength)
	{
		return MU8ToU16Impl<std::basic_string<U16T>>(mu8String, szStringLength);
	}
};


#define U16CV2MU8(u16String) MUTF8_Tool<>::U16ToMU8(u16String)
#define MU8CV2U16(mu8String) MUTF8_Tool<>::MU8ToU16(mu8String)

//#define MU8CV2U8(mu8String) MUTF8_Tool<>::(mu8String)
//#define U8CV2MU8(u8String) MUTF8_Tool<>::(u8String)

//��Ӣ������£�ת����Ч������
//#define MU8STR(charLiteralString) (charLiteralString)

//ת��Ϊ��̬�ַ�������
//��mutf-8�У��κ��ַ�����β\0���ᱻӳ���0xC0 0x80���ұ�֤���в�����0x00������һ���̶��Ͽ��Ժ�c-str����0��β������
#define CHAR2MU8STR(charLiteralString) (MUTF8_Tool<>::U16ToMU8<MUTF8_Tool<>::U16ToMU8Size(u##charLiteralString)>(u##charLiteralString))

//Ӣ��ԭ��
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

//���ķ���
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

����ʽ��"��׼"UTF-8��ʽ����������
��һ�����ַ�(char)0ʹ��˫�ֽڸ�ʽ���ǵ��ֽڸ�ʽ��
����޸ĺ��UTF-8�ַ����������Ƕ��ʽ��ֵ��
�ڶ�����ʹ�ñ�׼UTF-8�ĵ��ֽڡ�˫�ֽں����ֽڸ�ʽ��
Java�������ʶ���׼UTF-8�����ֽڸ�ʽ��
����ʹ���Զ����˫���ֽڸ�ʽ��

���ڱ�׼UTF-8��ʽ�ĸ�����Ϣ������ġ�The Unicode Standard, Version 13.0����3.9��"Unicode Encoding Forms"��
*/