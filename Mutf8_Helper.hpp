#pragma once

/*
Convert Modified UTF-8  <==>  UTF-32.
*/


/*
function : Convert Modified UTF-8 to UTF-32.
input : str_mutf8, a null terminated string in Modified UTF-8.
output : str_utf32, a null terminated string in UTF-32.
input : str_utf32_limit, the max length(character count)
		of str_utf32 plus one(for 'null'), str_utf32 must have enough space
		for str_utf32_limit characters.
return : -1 for errors;
		else the length(character count) of str_utf32,
				maybe larger than (str_utf32_limit-1) if the space
				of str_utf32 isn't enougn.
note : convert 0xc080 to U+0000 字符串未结束
		convert 0x00 to U+0000 字符串结束
*/
int mutf8_to_utf32(const unsigned char *str_mutf8,
	unsigned int *str_utf32, int str_utf32_limit);

/*
function : Convert UTF-32 to Modified UTF-8.
input : str_utf32, a null terminated string in UTF-32.
output : str_mutf8, a null terminated string in Modified UTF-8.
input : str_mutf8_limit, the max length(byte count)
		of str_mutf8 plus one(for 'null'), str_mutf8 must have enough space
		for str_mutf8_limit bytes.
return : -1 for errors;
		else the length(byte count) of str_mutf8,
				maybe larger than (str_mutf8_limit-1) if the space
				of str_mutf8 isn't enougn.
note : convet U+0000 to 0x00, not 0xc080 字符串结束
*/
int utf32_to_mutf8(const unsigned int *str_utf32,
	unsigned char *str_mutf8, int str_mutf8_limit);



#include <stdio.h> 
#include <Windows.h>


/*
A U+0001 to U+007F
0+++ ++++ u &0x80 => 0x00

B U+0080 to U+07FF, and null character (U+0000)
110+ ++++ u &0xe0 => 0xc0
10++ ++++ v &0xc0 => 0x80
((u & 0x1f) << 6) + (v & 0x3f)

C U+0800 to U+FFFF
1110 ++++ u &0xf0 => 0xe0
10++ ++++ v &0xc0 => 0x80
10++ ++++ w &0xc0 => 0x80
((u & 0xf) << 12) + ((v & 0x3f) << 6) + (w & 0x3f)

D above U+FFFF (U+10000 to U+10FFFF)
1110 1101 u &0xff => 0xed
1010 ++++ v &0xf0 => 0xa0
10++ ++++ w &0xc0 => 0x80
1110 1101 x &0xff => 0xed
1011 ++++ y &0xf0 => 0xb0
10++ ++++ z &0xc0 => 0x80
0x10000+((v&0x0f)<<16)+((w&0x3f)<<10)+(y&0x0f)<<6)+(z&0x3f)
*/

int mutf8_to_utf32(const unsigned char *str_mutf8,
	unsigned int *str_utf32, int str_utf32_limit)
{
	unsigned int cod, u, v, w, x, y, z;
	int len32 = 0;
	if ((NULL == str_mutf8) || (0 > str_utf32_limit))
	{
		return (-1);
	}

#define  __ADD_UTF32_COD_Z__   do {\
                if ( (NULL != str_utf32) && (len32 < str_utf32_limit) ) {\
                        str_utf32[ len32 ] = cod;\
                }\
                ++len32;\
        } while ( 0 )

	for (; ; )
	{
		u = *str_mutf8++;

		if (0 == u)
		{
			break;
		}

		if (0x00 == (0x80 & u))
		{
			cod = u;
			__ADD_UTF32_COD_Z__;
			continue;
		}

		if (0xc0 == (0xe0 & u))
		{
			v = *str_mutf8++;
			if (0x80 != (0xc0 & v))
			{
				return (-1);
			}
			cod = ((u & 0x1f) << 6) |
				(v & 0x3f);
			__ADD_UTF32_COD_Z__;
			continue;
		}

		if (0xe0 == (0xf0 & u))
		{
			v = *str_mutf8++;
			if (0x80 != (0xc0 & v))
			{
				return (-1);
			}
			w = *str_mutf8++;
			if (0x80 != (0xc0 & w))
			{
				return (-1);
			}
			if ((0xed == (0xff & u)) &&
				(0xa0 == (0xf0 & v)) &&
				(0x80 == (0xc0 & w))
				)
			{
				x = *str_mutf8++;
				if (0xed != (0xff & x))
				{
					return (-1);
				}
				y = *str_mutf8++;
				if (0xb0 != (0xf0 & y))
				{
					return (-1);
				}
				z = *str_mutf8++;
				if (0x80 != (0xc0 & z))
				{
					return (-1);
				}
				cod = 0x10000 + (
					((v & 0x0f) << 16) |
					((w & 0x3f) << 10) |
					((y & 0x0f) << 6) |
					(z & 0x3f));
				__ADD_UTF32_COD_Z__;
				continue;
			}
			cod = ((u & 0xf) << 12) |
				((v & 0x3f) << 6) |
				(w & 0x3f);
			__ADD_UTF32_COD_Z__;
			continue;
		}

		return (-1);
	}

	if (NULL == str_utf32)
	{
	}
	else if (len32 < str_utf32_limit)
	{
		str_utf32[len32] = 0;
	}
	else
	{
		str_utf32[str_utf32_limit - 1] = 0;
	}

	return len32;
#undef __ADD_UTF32_COD_Z__
}

int utf32_to_mutf8(const unsigned int *str_utf32,
	unsigned char *str_mutf8, int str_mutf8_limit)
{
	unsigned int cod;
	int len8 = 0;
	if ((NULL == str_utf32) || (0 > str_mutf8_limit))
	{
		return (-1);
	}

#define __ADD_MUTF8_B_Z__(b)   do {\
                if ( (NULL != str_mutf8) && (len8 < str_mutf8_limit) ) {\
                        str_mutf8[ len8 ] = (unsigned char)(b);\
                }\
                ++len8;\
        } while ( 0 )

	for (; ; )
	{
		cod = *str_utf32++;

		if (0 == cod)
		{
			break;
		}

		if (0x007f >= cod)
		{
			__ADD_MUTF8_B_Z__(cod);
			continue;
		}

		if (0x07ff >= cod)
		{
			__ADD_MUTF8_B_Z__(0xc0 | ((cod >> 6) & 0x1f));
			__ADD_MUTF8_B_Z__(0x80 | (cod & 0x3f));
			continue;
		}

		if (0xffff >= cod)
		{
			__ADD_MUTF8_B_Z__(0xe0 | ((cod >> 12) & 0x0f));
			__ADD_MUTF8_B_Z__(0x80 | ((cod >> 6) & 0x3f));
			__ADD_MUTF8_B_Z__(0x80 | (cod & 0x3f));
			continue;
		}

		if (0x10ffff >= cod)
		{
			cod -= 0x10000;
			__ADD_MUTF8_B_Z__(0xed);
			__ADD_MUTF8_B_Z__(0xa0 | ((cod >> 16) & 0x0f));
			__ADD_MUTF8_B_Z__(0x80 | ((cod >> 10) & 0x3f));
			__ADD_MUTF8_B_Z__(0xed);
			__ADD_MUTF8_B_Z__(0xb0 | ((cod >> 6) & 0x0f));
			__ADD_MUTF8_B_Z__(0x80 | (cod & 0x3f));
			continue;
		}

		return (-1);
	}

	if (NULL == str_mutf8)
	{
	}
	else if (len8 < str_mutf8_limit)
	{
		str_mutf8[len8] = 0;
	}
	else
	{
		str_mutf8[str_mutf8_limit - 1] = 0;
	}

	return len8;
#undef __ADD_MUTF8_B_Z__
}


class MUTF8_HELPER
{
private:
	static std::vector<uint32_t> decode_mutf8(const std::string &mutf8)
	{
		std::vector<uint32_t> code_points;
		size_t i = 0;
		while (i < mutf8.length())
		{
			if (mutf8[i] <= 0x7F)
			{ // 1字节字符
				code_points.push_back(mutf8[i]);
				i += 1;
			}
			else if ((mutf8[i] & 0xE0) == 0xC0)
			{ // 2字节字符
				if (i + 1 >= mutf8.length() || (mutf8[i + 1] & 0xC0) != 0x80)
				{
					code_points.push_back(0xFFFD); // 无效序列处理
					i = (i + 1 < mutf8.length()) ? i + 1 : mutf8.length();
					continue;
				}
				uint32_t cp = ((mutf8[i] & 0x1F) << 6) | (mutf8[i + 1] & 0x3F);
				code_points.push_back(cp);
				i += 2;
			}
			else if ((mutf8[i] & 0xF0) == 0xE0)
			{ // 3字节字符
				if (i + 2 >= mutf8.length() || (mutf8[i + 1] & 0xC0) != 0x80 || (mutf8[i + 2] & 0xC0) != 0x80)
				{
					code_points.push_back(0xFFFD);
					i = (i + 1 < mutf8.length()) ? i + 1 : mutf8.length();
					continue;
				}
				uint32_t cp = ((mutf8[i] & 0x0F) << 12) | ((mutf8[i + 1] & 0x3F) << 6) | (mutf8[i + 2] & 0x3F);
				code_points.push_back(cp);
				i += 3;
			}
			else
			{ // 非法起始字节
				code_points.push_back(0xFFFD);
				i += 1;
			}
		}
		return code_points;
	}

	static void process_surrogates(const std::vector<uint32_t> &code_points, std::vector<uint32_t> &processed)
	{
		size_t i = 0;
		while (i < code_points.size())
		{
			uint32_t cp = code_points[i];
			if (cp >= 0xD800 && cp <= 0xDBFF)
			{ // 高代理
				if (i + 1 < code_points.size() && code_points[i + 1] >= 0xDC00 && code_points[i + 1] <= 0xDFFF)
				{
					uint32_t high = cp - 0xD800;
					uint32_t low = code_points[i + 1] - 0xDC00;
					processed.push_back(0x10000 + (high << 10) + low);
					i += 2;
				}
				else
				{
					processed.push_back(0xFFFD); // 无效代理对
					i += 1;
				}
			}
			else if (cp >= 0xDC00 && cp <= 0xDFFF)
			{ // 孤立低代理
				processed.push_back(0xFFFD);
				i += 1;
			}
			else
			{
				processed.push_back(cp);
				i += 1;
			}
		}
	}

	static void encode_utf8(uint32_t cp, std::string &output)
	{
		if (cp <= 0x7F)
		{
			output.push_back((char8_t)cp);
		}
		else if (cp <= 0x7FF)
		{
			output.push_back((char8_t)(0xC0 | ((cp >> 6) & 0x1F)));
			output.push_back((char8_t)(0x80 | (cp & 0x3F)));
		}
		else if (cp <= 0xFFFF)
		{
			output.push_back((char8_t)(0xE0 | ((cp >> 12) & 0x0F)));
			output.push_back((char8_t)(0x80 | ((cp >> 6) & 0x3F)));
			output.push_back((char8_t)(0x80 | (cp & 0x3F)));
		}
		else if (cp <= 0x10FFFF)
		{
			output.push_back((char8_t)(0xF0 | ((cp >> 18) & 0x07)));
			output.push_back((char8_t)(0x80 | ((cp >> 12) & 0x3F)));
			output.push_back((char8_t)(0x80 | ((cp >> 6) & 0x3F)));
			output.push_back((char8_t)(0x80 | (cp & 0x3F)));
		}
		else
		{
			encode_utf8(0xFFFD, output); // 替换无效码点
		}
	}

public:
	static std::string mutf8_to_utf8(const std::string &mutf8)
	{
		std::vector<uint32_t> code_points = decode_mutf8(mutf8);
		std::vector<uint32_t> processed;
		process_surrogates(code_points, processed);
		std::string utf8;
		for (uint32_t cp : processed)
		{
			encode_utf8(cp, utf8);
		}
		return utf8;
	}

	static std::string utf8_to_local(const std::string &utf8_str)
	{
#ifdef _WIN32
		// Windows 实现（UTF-8 → UTF-16 → ANSI）
		if (utf8_str.empty())
		{
			return std::string{};
		}

		// Step 1: UTF-8 → UTF-16
		int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_str.length(), NULL, 0);
		if (wlen == 0)
		{
			return std::string{};
		}

		std::wstring utf16_str{};
		utf16_str.resize(wlen);//c++23用resize_and_overwrite
		MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_str.length(), utf16_str.data(), wlen);

		// Step 2: UTF-16 → ANSI（本地代码页）
		int len = WideCharToMultiByte(CP_ACP, 0, utf16_str.c_str(), utf16_str.length(), NULL, 0, NULL, NULL);
		if (len == 0)
		{
			return std::string{};
		}

		std::string local_str{};
		local_str.resize(len);//c++23用resize_and_overwrite
		WideCharToMultiByte(CP_ACP, 0, utf16_str.c_str(), utf16_str.length(), local_str.data(), len, NULL, NULL);

		return local_str;
#else	//注意以下代码未经测试（使用前请测试）
		// Linux/macOS 实现（使用 iconv）
		iconv_t cd = iconv_open("", "UTF-8"); // 目标编码为当前 locale
		if (cd == (iconv_t)-1)
		{
			if (errno == EINVAL) throw std::runtime_error("Conversion not supported");
			throw std::runtime_error("iconv_open failed");
		}

		size_t in_bytes = utf8_str.size();
		char *in_ptr = const_cast<char *>(utf8_str.data());
		size_t out_bytes = in_bytes * 4; // 预估最大可能长度
		std::string local_str(out_bytes, 0);
		char *out_ptr = &local_str[0];

		size_t result = iconv(cd, &in_ptr, &in_bytes, &out_ptr, &out_bytes);
		iconv_close(cd);

		if (result == (size_t)-1)
		{
			throw std::runtime_error(std::string("iconv failed: ") + strerror(errno));
		}

		local_str.resize(local_str.size() - out_bytes); // 调整实际长度
		return local_str;
#endif
	}
};



size_t Utf32_to_Utf8(uint32_t src, unsigned char *des)
{
	if (src == 0) return 0;

	static const char PREFIX[] = { 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
	static const uint32_t CODE_UP[] =
	{
	   0x80,           // U+00000000 - U+0000007F
	   0x800,          // U+00000080 - U+000007FF
	   0x10000,        // U+00000800 - U+0000FFFF
	   0x200000,       // U+00010000 - U+001FFFFF
	   0x4000000,      // U+00200000 - U+03FFFFFF
	   0x80000000      // U+04000000 - U+7FFFFFFF
	};

	size_t i, len = sizeof(CODE_UP) / sizeof(uint32_t);
	for (i = 0; i < len; ++i)
	{
		if (src < CODE_UP[i]) break;
	}

	if (i == len) return 0; // the src is invalid

	len = i + 1;
	if (des)
	{
		for (; i > 0; --i)
		{
			des[i] = static_cast<char>((src & 0x3F) | 0x80);
			src >>= 6;
		}
		des[0] = static_cast<char>(src | PREFIX[len - 1]);
	}

	return len;
}

int VerifyValidUtf8(unsigned char *in, uint32_t in_len, unsigned char *out, uint32_t &out_len)
{
#define EMOJI_SIZE 6
#define TO_UINT32(d) (uint32_t)(*(d))

	if (in_len <= 0 || *in == '\0')
	{
		return in_len == 0 ? 0 : -1;
	}
	size_t step = 0;
	if (in_len >= EMOJI_SIZE)
	{
		uint32_t offset = 0;
		if (
			(
				(TO_UINT32(in + offset) & 0xED) == 0xED &&
				(TO_UINT32(in + offset + 1) >= 0xA0 && TO_UINT32(in + offset + 1) <= 0xBF) &&
				(TO_UINT32(in + offset + 2) >= 0xA0 && TO_UINT32(in + offset + 2) <= 0xBF)
			)
			&&
			(
				(TO_UINT32(in + offset + 3) & 0xED) == 0xED &&
				(TO_UINT32(in + offset + 4) >= 0x80 && TO_UINT32(in + offset + 5) <= 0xBF) &&
				(TO_UINT32(in + offset + 4) >= 0x80 && TO_UINT32(in + offset + 5) <= 0xBF)
			)
		   )
		{
			uint32_t high = ((TO_UINT32(in + offset) & 0x0F) << 12) +
				((TO_UINT32(in + offset + 1) & 0x3F) << 6) +
				(TO_UINT32(in + offset + 2) & 0x3F);
			uint32_t low = ((TO_UINT32(in + offset + 3) & 0x0F) << 12) +
				((TO_UINT32(in + offset + 4) & 0x3F) << 6) +
				(TO_UINT32(in + offset + 5) & 0x3F);

			uint32_t codepoint = (high & 0x03FF) << 10 | (low & 0x03FF);
			codepoint += 0x10000;
			step = Utf32_to_Utf8(codepoint, out);
			in += EMOJI_SIZE;
			in_len -= EMOJI_SIZE;
			out += step;
			out_len += step;
		}
	}
	// 判断是否有跳跃，如果没有则自动+1
	if (step == 0)
	{
		*out++ = *in++;
		in_len--;
		out_len++;
	}

	return VerifyValidUtf8(in, in_len, out, out_len);
}
// 使用方法
//int main()
//{
//	unsigned char ch1[] = { 0xAB, 0xCD, '\n', 0xED, 0xA0, 0xBE, 0xED, 0xB4, 0x93, 0x00 };
//	unsigned char ch2[128] = { 0x00 };
//
//	uint32_t len = 0; // 一定要初始化为0
//	VerifyValidUtf8(ch1, sizeof(ch1) / sizeof(ch1[0]), ch2, len);
//
//	fprintf(stdout, "modified utf-8 : %s\n", ch1);
//	fprintf(stdout, "orgin utf-8  : %s, len:%d\n", ch2, len);
//
//	return 0;
//}