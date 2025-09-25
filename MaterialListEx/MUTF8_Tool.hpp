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

	//�����ڴ���������utf16�����ֽ�0xFFFD��mu8��u8��ʽ��0xEF 0xBF 0xBD
	static inline constexpr MU8T mu8FailChar[3]{ (MU8T)0xEF, (MU8T)0xBF, (MU8T)0xBD };
	static inline constexpr U16T u16FailChar = (U16T)0xFFFD;
	static inline constexpr U8T u8FailChar[3]{ (U8T)0xEF, (U8T)0xBF, (U8T)0xBD };
public:
	using MU8_T = MU8T;
	using U16_T = U16T;
	using U8_T = U8T;

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

		template<typename U>
		constexpr FakeStringCounter &append_cvt(const U *const, size_t szSize) noexcept
		{
			szCounter += szSize;//��̬��ֵ�붯̬��ֵ�������𣬲�������
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

		constexpr StaticString &append(const T *const pData, size_t szLength) noexcept
		{
			if (szIndex + szLength > arrData.size())
			{
				return *this;
			}

			//��pData�� 0 ~ szLength ������arrData�� szIndex ~ szIndex + szLength
			std::ranges::copy(&pData[0], &pData[szLength], &arrData[szIndex]);
			szIndex += szLength;

			return *this;
		}

		template<typename U>
		constexpr StaticString &append_cvt(const U *const pData, size_t szLength) noexcept
		{
			if (std::is_constant_evaluated())
			{
				if (szIndex + szLength > arrData.size())
				{
					return *this;
				}

				//��̬��ֵ��˳��ת��
				std::ranges::transform(&pData[0], &pData[szLength], &arrData[szIndex],
					[](const U &u) -> T
					{
						return (T)u;
					});
				szIndex += szLength;

				return *this;
			}
			else//ע����Ȼ��������������������κ���������ڶ�̬������Ϊ����ȷ��������Ȼ��ôд
			{
				return append((const T *const)pData, szLength);//�Ǿ�ֱ̬�ӱ���ת��ָ��
			}
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

	//ħ����3����String��Ӿ�̬ת���ӿڣ���ԭ��һ�£���ֹ����
	template<typename String>
	class DynamicString : public String//ע��ʹ�ü̳У���������ֱ����ʽת��������
	{
	public:
		using String::String;

		template<typename U>
		DynamicString &append_cvt(const U *const pData, size_t szLength)//�����ϴ˺�����Զ��̬����
		{
			String::append((const typename String::value_type *const)pData, szLength);
			return *this;
		}
	};

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


	static constexpr void UTF8SupplementaryToMUTF8(const U8T(&u8CharArr)[4], MU8T(&mu8CharArr)[6])
	{
		/*
		4bytes utf-8 bit distribution:��ע��utf32��utf16��ʱ��i�ᱻ����������������i�Ǻ�Ϊ0�ģ�
		000i'uuuu zzzz'yyyy yyxx'xxxx - 1111'0iuu 10uu'zzzz 10yy'yyyy 10xx'xxxx
		
		6bytes modified utf-8 bit distribution:��ע��mutf8ֱ�Ӻ�����i�����룩
		000i'uuuu zzzz'yyyy yyxx'xxxx - 1110'1101 1010'uuuu 10zz'zzyy 1110'1101 1011'yyyy 10xx'xxxx
		*/

		mu8CharArr[0] = (uint8_t)0b1110'1101;//�̶��ֽ�1110'1101
		mu8CharArr[1] = (uint8_t)(((uint8_t)u8CharArr[0] & (uint8_t)0b0000'0011) << 2 | ((uint8_t)u8CharArr[1] & (uint8_t)0b0011'0000) >> 4 | (uint8_t)0b1010'0000);//�ó�u8[0]��uu:1~0bit�ŵ�3~2bit���ó�u8[1]��uu:5~4bit�ŵ�1~0bit��������4λ1010�����1010'uuuu
		mu8CharArr[2] = (uint8_t)(((uint8_t)u8CharArr[1] & (uint8_t)0b0000'1111) << 2 | ((uint8_t)u8CharArr[2] & (uint8_t)0b0011'0000) >> 4 | (uint8_t)0b1000'0000);//�ó�u8[1]��zzzz:3~0bit�ŵ�5~2bit���ó�u8[2]��yy:5~4bit�ŵ�1~0bit��������2λ10�����10zz'zzyy
		mu8CharArr[3] = (uint8_t)0b1110'1101;//�̶��ֽ�1110'1101
		mu8CharArr[4] = (uint8_t)(((uint8_t)u8CharArr[2] & (uint8_t)0b0000'1111) << 0 | (uint8_t)0b1011'0000);//�ó�u8[2]��yyyy:3~0bit�ŵ�3~0bit��������4λ1011�����1011'yyyy
		mu8CharArr[5] = (uint8_t)u8CharArr[3];//���һ���ֽ�mu8��u8��ȫ��ͬ��ֱ�ӿ���
	}

	static constexpr void MUTF8SupplementaryToUTF8(const MU8T(&mu8CharArr)[6], U8T(&u8CharArr)[4])
	{
		//mu8CharArr[0] �� mu8CharArr[3] ����
		u8CharArr[0] = (uint8_t)(((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'1100) >> 2 | (uint8_t)0b1111'0000);//ע���ǲ�1111'0000��������1111'0100����Ϊ�����i��utf32��utf16����ԵĹ��̱���������λ��Ϊ0�����ǲ�֪��Ϊʲôutf8��Ȼ�����˴�λ
		u8CharArr[1] = (uint8_t)(((uint8_t)mu8CharArr[1] & (uint8_t)0b0000'0011) << 4 | ((uint8_t)mu8CharArr[2] & (uint8_t)0b0011'1100) >> 2 | (uint8_t)0b1000'0000);
		u8CharArr[2] = (uint8_t)(((uint8_t)mu8CharArr[2] & (uint8_t)0b0000'0011) << 4 | ((uint8_t)mu8CharArr[4] & (uint8_t)0b0000'1111) >> 0 | (uint8_t)0b1000'0000);
		u8CharArr[3] = (uint8_t)mu8CharArr[5];//���һ���ֽ�mu8��u8��ȫ��ͬ��ֱ�ӿ���
	}

private:
//c=char d=do������������ȡ��һ���ֽڣ�������ԣ�����ִ��ָ������������
#define GET_NEXTCHAR(c,d) if (++it == end) { (d);break; } else { (c) = *it; }
//v=value m=mask p=pattern t=test ��������λ֮��Ľ���Ƿ���ָ��ֵ��ֵ�Ƿ�����ָ��bits���
#define HAS_BITMASK(v,m,p) (((uint8_t)(v) & (uint8_t)(m)) == (uint8_t)(p))
#define IS_BITS(v,t) ((uint8_t)(v) == (uint8_t)(t))
//v=value b=begin e=end ע�ⷶΧ�����ұ߽������ϵ����������ͨ����߽����
#define IN_RANGE(v,b,e) (((uint16_t)(v) >= (uint16_t)(b)) && ((uint16_t)(v) <= (uint16_t)(e)))

	template<typename T = std::basic_string<MU8T>>
	static constexpr T U16ToMU8Impl(const U16T *u16String, size_t szStringLength)
	{
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
#define PUSH_FAIL_U16CHAR u16String.push_back(u16FailChar)

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
				GET_NEXTCHAR(mu8CharArr[1],
					(PUSH_FAIL_U16CHAR));//�ڶ���
				//�ж��ֽںϷ���
				if (!HAS_BITMASK(mu8CharArr[1], 0b1100'0000, 0b1000'0000))//��2λ����10����������
				{
					--it;//���ض�ȡ������for�Զ����������ղŵ��ַ���
					PUSH_FAIL_U16CHAR;//�滻Ϊutf16�����ַ�
					continue;//���ԣ���Ϊ��ǰ�ַ������Ǵ���ģ����ղŶ��ȡ�Ĳ�����ȷ�ģ�������Ҫ����continue���³���
				}

				//ת��
				U16T u16Char{};
				DecodeMUTF8Bmp(mu8CharArr, u16Char);
				u16String.push_back(u16Char);
			}
			else if (HAS_BITMASK(mu8Char, 0b1111'0000, 0b1110'0000))//��4λΪ1110�����ֽڻ���ֽ����
			{
				//��ǰ��ȡ��һ���ַ������Ǵ���Ե��ж�����
				MU8T mu8Next{};
				GET_NEXTCHAR(mu8Next, (PUSH_FAIL_U16CHAR));//�ڶ���

				//�Ϸ����жϣ������Ƿ�Ϊ����
				//�������֣���ΪD800��ͷ��Ϊ�ߴ����ز�������Ϊ���ֽ����0b1010'xxxx���֣�����ֻҪ��4λ��1010��Ϊ�����
				//Ҳ����˵mu8CharArr3[0]�ĵ�4bit�����1101����mu8Char�ĸ�4bit��1010������£������ֽ����10xx'xxxx�е���߶���xxΪ01��
				//�����Ǻ���������1101'10xx Ҳ����0xD8����u16�ĸߴ���Կ�ʼ�ַ������������encode�����ߵ���һ�����̣���������3�ֽ���������������
				if (IS_BITS(mu8Char, 0b1110'1101) && HAS_BITMASK(mu8Next, 0b1111'0000, 0b1010'0000))//����ԣ��������жϣ�����Ҫ��
				{
					//���浽����
					MU8T mu8CharArr[6] = { mu8Char,mu8Next };//[0] = mu8Char, [1] = mu8Next

					//������ȡ��4������֤

					//��һ��Ϊ�ߴ���ĵ�6λ
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//������
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))
					{
						//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
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
						//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
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
						//���ض��ζ�ȡ������ǰ����ȷ����0b1110'1101�����Ǵ���111��ͷ�ĺϷ�3���
						--it;
						--it;
						//�滻Ϊ����utf16�����ַ�����Ϊ���ض��Σ�������4�������ֽڵ�����ֻҪ3��
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
						//����һ�ζ�ȡ����Ϊ������ǰһ����ȷ�ϵ�101��ͷ�ĺϷ���㣬����ǰһ����ͷΪ111��Ҳ���ǲ�����111���101��3������������
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
				else if(HAS_BITMASK(mu8Next, 0b1100'0000, 0b1000'0000))//���ֽ���㣬�ų�����Ժ�ֻ��������ܣ������ǲ���10��ͷ��β���ֽ�
				{
					//����
					MU8T mu8CharArr[3] = { mu8Char,mu8Next };//[0] = mu8Char, [1] = mu8Next

					//���Ի�ȡ��һ�ַ�
					GET_NEXTCHAR(mu8CharArr[2],
						(PUSH_FAIL_U16CHAR, PUSH_FAIL_U16CHAR));//������
					if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))//����3�ֽ�������һ��������ȷ�ַ�
					{
						//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
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
					//����mu8Next�Ķ�ȡ����Ϊmu8Char�Ѿ��жϹ��������е����
					//֤�����ֽڴ���������ص�mu8Char�ᵼ�����޴���ѭ����
					//ֻ���ص�mu8Next���ɣ�for������++it���൱�����Ե�ǰ*it
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
	Modified UTF-8 �� "��׼"UTF-8 ��ʽ�ж�������
	��һ�����ַ�(char)0ʹ��˫�ֽڸ�ʽ0xC0 0x80���ǵ��ֽڸ�ʽ0x00��
	��� Modified UTF-8�ַ����������Ƕ��ʽ��ֵ��

	�ڶ�����ʹ�ñ�׼UTF-8�ĵ��ֽڡ�˫�ֽں����ֽڸ�ʽ��
	Java�������ʶ���׼UTF-8�����ֽڸ�ʽ��
	����ʹ���Զ����˫���ֽڣ�6�ֽڴ���ԣ���ʽ��
	*/

	template<typename T = DynamicString<std::basic_string<MU8T>>>
	static constexpr T U8ToMU8Impl(const U8T *u8String, size_t szStringLength)
	{
#define PUSH_FAIL_MU8CHAR mu8String.append(mu8FailChar, sizeof(mu8FailChar) / sizeof(MU8T))
#define INSERT_NORMAL(p) (mu8String.append_cvt((p) - szNormalLength, szNormalLength), szNormalLength = 0)
		
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

				U8T u8CharArr[4]{ u8Char };//[0] = u8Char

				GET_NEXTCHAR(u8CharArr[1],
					(PUSH_FAIL_MU8CHAR));//�ڶ���
				if (!HAS_BITMASK(u8CharArr[1], 0b1100'0000, 0b1000'0000))//ȷ����2bit��10
				{
					//���һ�������ַ�
					PUSH_FAIL_MU8CHAR;
					--it;//����һ�ζ�ȡ�����Դ�����10��ͷ��
					continue;
				}

				GET_NEXTCHAR(u8CharArr[2],
					(PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR));//������
				if (!HAS_BITMASK(u8CharArr[1], 0b1100'0000, 0b1000'0000))//ȷ����2bit��10
				{
					//������������ַ�
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					--it;//����һ�ζ�ȡ�����Դ�����10��ͷ��
					continue;
				}

				GET_NEXTCHAR(u8CharArr[3],
					(PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR, PUSH_FAIL_MU8CHAR));//���Ĵ�
				if (!HAS_BITMASK(u8CharArr[3], 0b1100'0000, 0b1000'0000))//ȷ����2bit��10
				{
					//������������ַ�
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					PUSH_FAIL_MU8CHAR;
					--it;//����һ�ζ�ȡ�����Դ�����10��ͷ��
					continue;
				}

				//��ȡ�ɹ����
				MU8T mu8CharArr[6]{};
				UTF8SupplementaryToMUTF8(u8CharArr, mu8CharArr);
				mu8String.append(mu8CharArr, sizeof(mu8CharArr) / sizeof(MU8T));
			}
			else if (IS_BITS(u8Char, 0b0000'0000))//\0�ַ�
			{
				INSERT_NORMAL(it);//�ڴ���֮ǰ�Ȳ���֮ǰ����������ͨ�ַ�

				MU8T mu8EmptyCharArr[2] = { (MU8T)0xC0,(MU8T)0x80 };//mu8�̶�0�ֽ�
				mu8String.append(mu8EmptyCharArr, sizeof(mu8EmptyCharArr) / sizeof(MU8T));
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
#undef PUSH_FAIL_MU8CHAR
	}

	template<typename T = DynamicString<std::basic_string<U8T>>>
	static constexpr T MU8ToU8Impl(const MU8T *mu8String, size_t szStringLength)
	{
#define PUSH_FAIL_U8CHAR u8String.append(u8FailChar, sizeof(u8FailChar) / sizeof(U8T))
#define INSERT_NORMAL(p) (u8String.append_cvt((p) - szNormalLength, szNormalLength), szNormalLength = 0)

		//���ڱ���ת�����utf8
		T u8String{};

		size_t szNormalLength = 0;//��ͨ�ַ��ĳ��ȣ������Ż���������
		for (auto it = mu8String, end = mu8String + szStringLength; it != end; ++it)
		{
			MU8T mu8Char = *it;//��һ��

			if (HAS_BITMASK(mu8Char, 0b1111'0000, 0b1110'0000))//��4ΪΪ1110��mu8��3�ֽڻ���ֽ����
			{
				//��ǰ��ȡ��һ��
				MU8T mu8Next{};
				if (++it == end)
				{
					//��ǰ��Ķ�����һ��
					INSERT_NORMAL(it - 1);//ע�������-1����Ϊ������Ҫ�ڿ���俪ͷִ�еģ������Ѿ���ǰ�ƶ���һ�µ�����������1��ǰλ��
					PUSH_FAIL_U8CHAR;//��������ַ�
					break;
				}
				mu8Next = *it;//�ڶ���


				//��1110'1101�ֽڿ�ʼ����һ���ֽڸ�4λ��1010��ͷ�ı�Ȼ�Ǵ����
				if (!IS_BITS(mu8Char, 0b1110'1101) || !HAS_BITMASK(mu8Next, 0b1111'0000, 0b1010'0000))
				{
					szNormalLength += 2;//ǰ����������������������
					continue;//Ȼ�����ѭ��
				}

				//��ȷ���Ǵ���ԣ���ǰ��Ķ�����һ��
				INSERT_NORMAL(it - 1);//ע�������-1����Ϊ������Ҫ�ڿ���俪ͷִ�еģ������Ѿ���ǰ��ȡ��һ��mu8Next������1��ǰλ��

				//������ȡ��4������֤
				MU8T mu8CharArr[6] = { mu8Char, mu8Next };//[0] = mu8Char, [1] = mu8Next

				//��ȡ��һ��
				GET_NEXTCHAR(mu8CharArr[2],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//������
				if (!HAS_BITMASK(mu8CharArr[2], 0b1100'0000, 0b1000'0000))
				{
					//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
					--it;
					//�滻Ϊ����utf8�����ַ�
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//��ȡ��һ��
				GET_NEXTCHAR(mu8CharArr[3],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//���Ĵ�
				if (!IS_BITS(mu8CharArr[3], 0b1110'1101))
				{
					//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
					--it;
					//�滻Ϊ����utf8�����ַ�
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//��ȡ��һ��
				GET_NEXTCHAR(mu8CharArr[4],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//�����
				if (!HAS_BITMASK(mu8CharArr[4], 0b1111'0000, 0b1011'0000))
				{
					//���ض��ζ�ȡ������ǰ����ȷ����0b1110'1101�����Ǵ���111��ͷ�ĺϷ�3���
					--it;
					--it;
					//�滻Ϊ����utf8�����ַ�����Ϊ���ض��Σ�������4�������ֽڵ�����ֻҪ3��
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//��ȡ��һ��
				GET_NEXTCHAR(mu8CharArr[5],
					(PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR, PUSH_FAIL_U8CHAR));//������
				if (!HAS_BITMASK(mu8CharArr[5], 0b1100'0000, 0b1000'0000))
				{
					//����һ�ζ�ȡ��Ϊʲô���Ƕ��Σ���Ϊǰһ���ַ���ȷ����10��ͷ��β���ַ���������
					--it;
					//�滻Ϊ���utf8�����ַ�
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					PUSH_FAIL_U8CHAR;
					continue;
				}

				//���ˣ�ȫ����֤ͨ��������ת��
				U8T u8CharArr[4]{};
				MUTF8SupplementaryToUTF8(mu8CharArr, u8CharArr);
				u8String.append(u8CharArr, sizeof(u8CharArr) / sizeof(U8T));
			}
			else if (IS_BITS(mu8Char, 0xC0))//ע����0xC0��ͷ�ģ���Ȼ��2�ֽ��룬�����������û�еڶ����ַ������Ȼ����
			{
				//��ǰ��ȡ��һ��
				MU8T mu8Next{};
				if (++it == end)
				{
					//��ǰ��Ķ�����һ��
					INSERT_NORMAL(it - 1);
					PUSH_FAIL_U8CHAR;//��������ַ�
					break;
				}
				mu8Next = *it;//�ڶ���

				if (!IS_BITS(mu8Next, 0x80))//������ǣ�˵���Ǳ���ֽ�ģʽ
				{
					szNormalLength += 2;//��ͨ�ַ�����2Ȼ�����
					continue;
				}

				//��ȷ����0�ַ�������һ��ǰ�����������
				INSERT_NORMAL(it - 1);//ע�������-1����Ϊ������Ҫ�ڿ���俪ͷִ�еģ������Ѿ���ǰ��ȡ��һ��mu8Next������1��ǰλ��
				u8String.push_back((U8T)0x00);//����0�ַ�
			}
			else
			{
				++szNormalLength;//��ͨ�ַ�������
				continue;//����
			}
		}
		//����ٰ�for��ʣ��δ����Ĳ���һ�£�ע��������ʼλ����ʵ��for�е�endλ��
		INSERT_NORMAL(mu8String + szStringLength);

		return u8String;

#undef INSERT_NORMAL
#undef PUSH_FAIL_U8CHAR
	}

#undef IN_RANGE
#undef IS_BITS
#undef HAS_BITMASK
#undef GET_NEXTCHAR

public:
	static constexpr size_t U16ToMU8Length(const std::basic_string_view<U16T> &u16String)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String.data(), u16String.size()).GetData();
	}
	static constexpr size_t U16ToMU8Length(const U16T *u16String, size_t szStringLength)
	{
		return U16ToMU8Impl<FakeStringCounter<MU8T>>(u16String, szStringLength).GetData();
	}
	template<size_t N>
	static consteval size_t U16ToMU8Length(const U16T(&u16String)[N])
	{
		size_t szStringLength =
			N > 0 && u16String[N - 1] == (U16T)0x00
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
	template<size_t szNewLength, size_t N>//size_t szNewLength = U16ToMU8Length(u16String);
	static consteval auto U16ToMU8(const U16T(&u16String)[N])
	{
		size_t szStringLength = 
			N > 0 && u16String[N - 1] == (U16T)0x00
			? N - 1 
			: N;
	
		return U16ToMU8Impl<StaticString<MU8T, szNewLength>>(u16String, szStringLength).GetData();
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

	//---------------------------------------------------------------------------------------------//

	//---------------------------------------------------------------------------------------------//

	static constexpr size_t U8ToMU8Length(const std::basic_string_view<U8T> &u8String)
	{
		return U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String.data(), u8String.size()).GetData();
	}
	static constexpr size_t U8ToMU8Length(const U8T *u8String, size_t szStringLength)
	{
		return U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String, szStringLength).GetData();
	}
	template<size_t N>
	static consteval size_t U8ToMU8Length(const U8T(&u8String)[N])
	{
		size_t szStringLength =
			N > 0 && u8String[N - 1] == (U8T)0x00
			? N - 1
			: N;

		return U8ToMU8Impl<FakeStringCounter<MU8T>>(u8String, szStringLength).GetData();
	}

	static std::basic_string<MU8T> U8ToMU8(const std::basic_string_view<U8T> &u8String)
	{
		return U8ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u8String.data(), u8String.size());
	}
	static std::basic_string<MU8T> U8ToMU8(const U8T *u8String, size_t szStringLength)
	{
		return U8ToMU8Impl<DynamicString<std::basic_string<MU8T>>>(u8String, szStringLength);
	}
	template<size_t szNewLength, size_t N>//size_t szNewLength = U8ToMU8Length(u8String);
	static consteval auto U8ToMU8(const U8T(&u8String)[N])
	{
		size_t szStringLength =
			N > 0 && u8String[N - 1] == (U8T)0x00
			? N - 1
			: N;

		return U8ToMU8Impl<StaticString<MU8T, szNewLength>>(u8String, szStringLength).GetData();
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

	static std::basic_string<U8T> MU8ToU8(const std::basic_string_view<MU8T> &mu8String)
	{
		return MU8ToU8Impl<DynamicString<std::basic_string<U8T>>>(mu8String.data(), mu8String.size());
	}
	static std::basic_string<U8T> MU8ToU8(const MU8T *mu8String, size_t szStringLength)
	{
		return MU8ToU8Impl<DynamicString<std::basic_string<U8T>>>(mu8String, szStringLength);
	}
};

//�������ú�
#define U16CV2MU8(u16String) MUTF8_Tool<>::U16ToMU8(u16String)
#define MU8CV2U16(mu8String) MUTF8_Tool<>::MU8ToU16(mu8String)

#define U8CV2MU8(u8String) MUTF8_Tool<>::U8ToMU8(u8String)
#define MU8CV2U8(mu8String) MUTF8_Tool<>::MU8ToU8(mu8String)

//ת��Ϊ��̬�ַ�������
//��mutf-8�У��κ��ַ�����β\0���ᱻӳ���0xC0 0x80���ұ�֤���в�����0x00������һ���̶��Ͽ��Ժ�c-str����0��β������
#define U16TOMU8STR(u16LiteralString) (MUTF8_Tool<>::U16ToMU8<MUTF8_Tool<>::U16ToMU8Length(u16LiteralString)>(u16LiteralString))
#define U8TOMU8STR(u8LiteralString) (MUTF8_Tool<>::U8ToMU8<MUTF8_Tool<>::U8ToMU8Length(u8LiteralString)>(u8LiteralString))

//Ӣ��ԭ��
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

//���ķ���
/*
�޸ĺ�� UTF-8 �ַ���
JNI ʹ���޸ĺ�� UTF-8 �ַ�������ʾ�����ַ������͡��޸ĺ�� UTF-8 �ַ����� Java VM ��ʹ�õ��ַ�����ͬ���޸ĺ�� UTF-8 �ַ����������룬ʹ�ý������ǿ� ASCII �ַ����ַ����п���ÿ���ַ���ʹ��һ���ֽ�����ʾ�������� Unicode �ַ������Ա���ʾ��

��Χ�� \u0001 �� \u007F ֮��������ַ����ɵ����ֽڱ�ʾ��������ʾ��

0xxxxxxx
�ֽ��е���λ���ݸ���������ʾ�ַ���ֵ��

���ַ� ('\u0000') �ͷ�Χ�� '\u0080' �� '\u07FF' ֮����ַ���һ���ֽ� x �� y ��ʾ��

x: 110xxxxx
y: 10yyyyyy
��Щ�ֽڱ�ʾֵΪ ((x & 0x1f) << 6) + (y & 0x3f) ���ַ���

��Χ�� '\u0800' �� '\uFFFF' ֮����ַ��������ֽ� x��y �� z ��ʾ��

x: 1110xxxx
y: 10yyyyyy
z: 10zzzzzz
ֵΪ ((x & 0xf) << 12) + ((y & 0x3f) << 6) + (z & 0x3f) ���ַ�����Щ�ֽڱ�ʾ��

������ U+FFFF ���ַ�������ν�Ĳ����ַ���ͨ���ֱ������ UTF-16 ��ʾ�Ķ���������Ԫ����ʾ��ÿ��������Ԫ�������ֽڱ�ʾ������ζ�ţ������ַ��������ֽ� u��v��w��x��y �� z ��ʾ��

u: 11101101
v: 1010vvvv
w: 10wwwwww
x: 11101101
y: 1011yyyy
z: 10zzzzzz
ֵΪ 0x10000+((v&0x0f)<<16)+((w&0x3f)<<10)+(y&0x0f)<<6)+(z&0x3f) ���ַ����������ֽڱ�ʾ��

���ֽ��ַ����ֽ������ļ����Դ���򣨸�λ�ֽ���ǰ���洢��

�˸�ʽ���׼ UTF-8 ��ʽ�ж����������ȣ����ַ� (char)0 ʹ��˫�ֽڸ�ʽ���ǵ��ֽڸ�ʽ���б��롣����ζ���޸ĺ�� UTF-8 �ַ�����Զ�������Ƕ��Ŀ��ַ�����Σ���ʹ�ñ�׼ UTF-8 �ĵ��ֽڡ�˫�ֽں����ֽڸ�ʽ��Java VM ��ʶ���׼ UTF-8 �����ֽڸ�ʽ����ʹ���Լ��Ķ������ֽڸ�ʽ�����档

�йر�׼ UTF-8 ��ʽ�ĸ�����Ϣ������� Unicode ��׼ 4.0 ��ĵ� 3.9 �� Unicode ������ʽ��
*/