#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>//size_t
#include <fstream>
#include <vector>
#include <filesystem>

#ifdef USE_ZLIB

/*zlib*/
#define ZLIB_CONST
#include <zlib.h>
/*zlib*/

#endif

#include "NBT_Print.hpp"//打印输出

class NBT_IO
{
	NBT_IO(void) = delete;
	~NBT_IO(void) = delete;

public:
	template<typename T = std::vector<uint8_t>>
	static bool WriteFile(const std::string &strFileName, const T &tData)
	{
		std::fstream fWrite;
		fWrite.open(strFileName, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		if (!fWrite)
		{
			return false;
		}

		//获取文件大小并写出
		uint64_t qwFileSize = tData.size();
		if (!fWrite.write((const char *)tData.data(), sizeof(tData[0]) * qwFileSize))
		{
			return false;
		}

		//完成，关闭文件
		fWrite.close();

		return true;
	}

	template<typename T = std::vector<uint8_t>>
	static bool ReadFile(const std::string &strFileName, T &tData)
	{
		std::fstream fRead;
		fRead.open(strFileName, std::ios_base::binary | std::ios_base::in);
		if (!fRead)
		{
			return false;
		}

		//获取文件大小
		// 移动到文件末尾
		if (!fRead.seekg(0, std::ios::end))
		{
			return false;
		}

		// 获取文件大小
		size_t szFileSize = fRead.tellg();

		//回到文件开头
		if (!fRead.seekg(0, std::ios::beg))
		{
			return false;
		}

		//直接给数据塞string里
		tData.resize(szFileSize);//设置长度 c++23用resize_and_overwrite
		if (!fRead.read((char *)tData.data(), sizeof(tData[0]) * szFileSize))//直接读入data
		{
			return false;
		}

		//完成，关闭文件
		fRead.close();

		return true;
	}

	static bool IsFileExist(const std::string &sFileName)
	{
		std::error_code ec;//判断这东西是不是true确定有没有error
		bool bExists = std::filesystem::exists(sFileName, ec);

		return !ec && bExists;//没有错误并且存在
	}

#ifdef USE_ZLIB

	static bool IsZlib(uint8_t u8DataFirst, uint8_t u8DataSecond)
	{
		return u8DataFirst == (uint8_t)0x78 &&
			(
				u8DataSecond == (uint8_t)0x9C ||
				u8DataSecond == (uint8_t)0x01 ||
				u8DataSecond == (uint8_t)0xDA ||
				u8DataSecond == (uint8_t)0x5E
			);
	}

	static bool IsGzip(uint8_t u8DataFirst, uint8_t u8DataSecond)
	{
		return u8DataFirst == (uint8_t)0x1F && u8DataSecond == (uint8_t)0x8B;
	}

	template<typename T>
	requires (sizeof(typename T::value_type) == 1 && std::is_trivially_copyable_v<typename T::value_type>)
	static bool IsDataZipped(T &tData)
	{
		if (tData.size() <= 2)
		{
			return false;
		}

		uint8_t u8DataFirst = (uint8_t)tData[0];
		uint8_t u8DataSecond = (uint8_t)tData[1];

		return IsZlib(u8DataFirst, u8DataSecond) || IsGzip(u8DataFirst, u8DataSecond);
	}

	template<typename I, typename O>
	requires (sizeof(typename I::value_type) == 1 && std::is_trivially_copyable_v<typename I::value_type> &&
			  sizeof(typename O::value_type) == 1 && std::is_trivially_copyable_v<typename O::value_type>)
	static void DecompressData(O &oData, const I &iData)
	{
		if (std::addressof(oData) == std::addressof(iData))
		{
			throw std::runtime_error("The oData object cannot be the iData object");
		}

		if (iData.empty())
		{
			oData.clear();
			return;
		}

		z_stream zs
		{
			.next_in = Z_NULL,
			.avail_in = 0,
			.zalloc = Z_NULL,
			.zfree = Z_NULL,
			.opaque = Z_NULL,
		};

		if (inflateInit2(&zs, 32 + MAX_WBITS) != Z_OK)//32+MAX_WBITS自动判断是gzip还是zlib
		{
			throw std::runtime_error("Failed to initialize zlib decompression");
		}

		zs.next_in = (z_const Bytef *)iData.data();
		zs.avail_in = (uInt)iData.size();

		oData.resize(iData.size() * 4);

		std::size_t szDecompressedSize = 0;
		int iRet = Z_OK;
		do
		{
			size_t szOut = oData.size() - szDecompressedSize;
			if (szOut == 0)
			{
				oData.resize(oData.size() * 2);
				szOut = oData.size() - szDecompressedSize;
			}

			zs.next_out = (Bytef *)(&oData.data()[szDecompressedSize]);
			zs.avail_out = (uInt)szOut;
			
			iRet = inflate(&zs, Z_FINISH);
			if (iRet != Z_STREAM_END &&
				iRet != Z_OK &&
				iRet != Z_BUF_ERROR)
			{
				inflateEnd(&zs);
				
				switch (iRet)
				{
				case Z_NEED_DICT:
					throw std::runtime_error("Zlib decompression failed: need dictionary");
				case Z_DATA_ERROR:
					throw std::runtime_error("Zlib decompression failed: invalid or corrupted data");
				case Z_MEM_ERROR:
					throw std::runtime_error("Zlib decompression failed: insufficient memory");
				case Z_STREAM_ERROR:
					throw std::runtime_error("Zlib decompression failed: invalid stream state");
				default:
					throw std::runtime_error("Zlib decompression failed with error code: " + std::to_string(iRet));
				}
			}

			szDecompressedSize += szOut - zs.avail_out;
		} while (iRet != Z_STREAM_END && zs.avail_in > 0);

		inflateEnd(&zs);
		oData.resize(szDecompressedSize);

		if (iRet != Z_STREAM_END)
		{
			throw std::runtime_error("Zlib decompression incomplete: stream did not end properly");
		}
	}

	template<typename I, typename O>
	requires (sizeof(typename I::value_type) == 1 && std::is_trivially_copyable_v<typename I::value_type> &&
			  sizeof(typename O::value_type) == 1 && std::is_trivially_copyable_v<typename O::value_type>)
	static void CompressData(O &oData, const I &iData, int iLevel = Z_DEFAULT_COMPRESSION)
	{
		if (std::addressof(oData) == std::addressof(iData))
		{
			throw std::runtime_error("The oData object cannot be the iData object");
		}

		if (iData.empty())
		{
			oData.clear();
			return;
		}

		z_stream zs
		{
			.next_in = Z_NULL,
			.avail_in = 0,
			.zalloc = Z_NULL,
			.zfree = Z_NULL,
			.opaque = Z_NULL,
		};

		if (deflateInit2(&zs, iLevel, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)//15+16使用gzip（mojang默认格式）
		{
			throw std::runtime_error("Failed to initialize zlib compression");
		}

		zs.next_in = (z_const Bytef *)iData.data();
		zs.avail_in = (uInt)iData.size();

		oData.resize(deflateBound(&zs, iData.size()));//预测压缩大小

		std::size_t szCompressedSize = 0;
		int iRet = Z_OK;
		do
		{
			size_t szOut = oData.size() - szCompressedSize;
			if (szOut == 0)
			{
				oData.resize(oData.size() * 2);
				szOut = oData.size() - szCompressedSize;
			}

			zs.next_out = (Bytef *)(&oData.data()[szCompressedSize]);
			zs.avail_out = (uInt)szOut;
			
			iRet = deflate(&zs, Z_FINISH);
			if (iRet != Z_STREAM_END &&
				iRet != Z_OK &&
				iRet != Z_BUF_ERROR)
			{
				deflateEnd(&zs);

				switch (iRet)
				{
				case Z_STREAM_ERROR:
					throw std::runtime_error("Zlib compression failed: invalid compression level or stream state");
				case Z_DATA_ERROR:
					throw std::runtime_error("Zlib compression failed: input data is invalid");
				case Z_MEM_ERROR:
					throw std::runtime_error("Zlib compression failed: insufficient memory");
				case Z_BUF_ERROR:
					throw std::runtime_error("Zlib compression failed: no progress possible");
				default:
					throw std::runtime_error("Zlib compression failed with error code: " + std::to_string(iRet));
				}
			}

			szCompressedSize += (szOut - zs.avail_out);
		} while (iRet != Z_STREAM_END && zs.avail_in > 0);

		deflateEnd(&zs);
		oData.resize(szCompressedSize);

		if (iRet != Z_STREAM_END)
		{
			throw std::runtime_error("Zlib compression incomplete: stream did not end properly");
		}
	}

	template<typename I, typename O, typename ErrInfoFunc = NBT_Print>
	requires (sizeof(typename I::value_type) == 1 && std::is_trivially_copyable_v<typename I::value_type> &&
			  sizeof(typename O::value_type) == 1 && std::is_trivially_copyable_v<typename O::value_type>)
	static bool DecompressDataNoThrow(O &oData, const I &iData, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept
	{
		try
		{
			DecompressData(oData, iData);
			return true;
		}
		catch (const std::bad_alloc &e)
		{
			funcErrInfo("std::bad_alloc:[{}]\n", e.what());
			return false;
		}
		catch (const std::exception &e)
		{
			funcErrInfo("std::exception:[{}]\n", e.what());
			return false;
		}
		catch (...)
		{
			funcErrInfo("Unknown Error\n");
			return false;
		}
	}

	template<typename I, typename O, typename ErrInfoFunc = NBT_Print>
	requires (sizeof(typename I::value_type) == 1 && std::is_trivially_copyable_v<typename I::value_type> &&
			  sizeof(typename O::value_type) == 1 && std::is_trivially_copyable_v<typename O::value_type>)
	static bool CompressDataNoThrow(O &oData, const I &iData, int iLevel = Z_DEFAULT_COMPRESSION, ErrInfoFunc funcErrInfo = NBT_Print{ stderr }) noexcept
	{
		try
		{
			CompressData(oData, iData, iLevel);
			return true;
		}
		catch (const std::bad_alloc &e)
		{
			funcErrInfo("std::bad_alloc:[{}]\n", e.what());
			return false;
		}
		catch (const std::exception &e)
		{
			funcErrInfo("std::exception:[{}]\n", e.what());
			return false;
		}
		catch (...)
		{
			funcErrInfo("Unknown Error\n");
			return false;
		}
	}

#endif
};