#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>//size_t
#include <fstream>
#include <vector>
#include <filesystem>

#include "vcpkg_config.h"//包含vcpkg生成的配置以确认库安装情况

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
			.total_in = 0,

			.next_out = Z_NULL,
			.avail_out = 0,
			.total_out = 0,

			.msg = Z_NULL,
			.state = Z_NULL,

			.zalloc = (alloc_func)Z_NULL,
			.zfree = (free_func)Z_NULL,
			.opaque = (voidpf)Z_NULL,

			.data_type = Z_BINARY,

			.adler = 0,
			.reserved = {},
		};

		if (inflateInit2(&zs, 32 + 15) != Z_OK)//32+15自动判断是gzip还是zlib
		{
			throw std::runtime_error("Failed to initialize zlib decompression");
		}

		//对于大于uint_max的数据来说，切分为多个uint_max的块作为流依次输入
		//注意zlib在实现内会移动指针，所以对于大于一定字节的情况下，只需要更新
		//avail_in，而无须更新next_in。这里的容器保证不变，所以地址不会失效
		zs.next_in = (z_const Bytef *)iData.data();

		//默认解压大小为压缩大小，后续扩容
		oData.resize(iData.size());

		//两个变量用于记录已经压缩的大小和剩余数据大小
		size_t szDecompressedSize = 0;
		size_t szRemainingSize = iData.size();
		int iRet = Z_OK;
		do
		{
			//首先计算剩余可用空间，然后决定是否扩容
			size_t szOut = oData.size() - szDecompressedSize;
			if (szOut == 0)
			{
				oData.resize(oData.size() * 2);
				szOut = oData.size() - szDecompressedSize;
			}

			//虽然next_out也会在内部实现被移动，但是oData是容器，会被扩容
			//一旦扩容触发，那么实际上的地址就可能会改变，所以必须每次重新获取
			//切记上面与这里的代码顺序不可改变，必须在上面计算并扩容完成后获取地址
			zs.next_out = (Bytef *)(&oData.data()[szDecompressedSize]);

			//如果输入被消耗完，重新赋值
			if (zs.avail_in == 0)
			{
				constexpr uInt uIntMax = (uInt)-1;
				zs.avail_in = szRemainingSize > (size_t)uIntMax ? uIntMax : (uInt)szRemainingSize;
				szRemainingSize -= zs.avail_in;//缩小剩余待处理大小
			}
			
			//如果输出大小耗尽，重新赋值
			if (zs.avail_out == 0)
			{
				constexpr uInt uIntMax = (uInt)-1;
				zs.avail_out = szOut > (size_t)uIntMax ? uIntMax : (uInt)szOut;
				//这里不对szOut处理，因为会在开头与结尾计算
			}

			//解压，如果剩余不为0则代表分块了，使用Z_NO_FLUSH，否则Z_FINISH
			iRet = inflate(&zs, szRemainingSize != 0 ? Z_NO_FLUSH : Z_FINISH);

			//计算本次解压的大小
			szDecompressedSize += szOut - zs.avail_out;
		} while (iRet == Z_OK || iRet == Z_BUF_ERROR);//只要没错误（缓冲区不够大除外）或者没到结尾就继续运行

		inflateEnd(&zs);//结束解压
		oData.resize(szDecompressedSize);//设置解压大小

		//错误处理
		if (iRet != Z_STREAM_END)
		{
			if (zs.msg != NULL)//错误消息不为null则抛出带消息异常
			{
				throw std::runtime_error(std::string("Zlib decompression failed with error message: ") + std::string(zs.msg));
			}
			else//如果为null直接使用错误码
			{
				throw std::runtime_error(std::string("Zlib decompression failed with error code: ") + std::to_string(iRet));
			}
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
			.total_in = 0,

			.next_out = Z_NULL,
			.avail_out = 0,
			.total_out = 0,

			.msg = Z_NULL,
			.state = Z_NULL,

			.zalloc = (alloc_func)Z_NULL,
			.zfree = (free_func)Z_NULL,
			.opaque = (voidpf)Z_NULL,

			.data_type = Z_BINARY,

			.adler = 0,
			.reserved = {},
		};

		if (deflateInit2(&zs, iLevel, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY) != Z_OK)//16+15使用gzip（mojang默认格式）
		{
			throw std::runtime_error("Failed to initialize zlib compression");
		}

		//与上方解压例程相同，不做重复解释
		zs.next_in = (z_const Bytef *)iData.data();

		/*
			如果范围溢出，则设置为比原始数据的大小加12字节大0.1%的一半
			这样做的目的是为了尽可能缩小一开始的体积
			比如实际上压缩率非常低的情况下可能根本用不到一半
			但是如果实际上压缩率很高，那么下面二倍扩容一次后刚好就是最坏情况
			这样基本上性能较优，下面zlib文档说明的最坏情况：
			
			Upon entry, destLen is the total size of the
			destination buffer, which must be at least 0.1%
			larger than sourceLen plus 12 bytes.
		*/

		constexpr uLong uLongMax = (uLong)-1;
		if (iData.size() > (size_t)uLongMax)
		{
			size_t szNeedSize = iData.size() + 12;//先比原始数据大12byte
			//注意这里使用了向上取整的整数除法
			//加等于自身的0.1%相当于比原先的自己大0.1%，这里的1/1000就是0.1/100
			szNeedSize += (szNeedSize + (1000 - 1)) / 1000;
			//设置目标大小
			oData.resize((szNeedSize + (2 - 1)) / 2);//向上取整除以二
		}
		else
		{
			//在范围未溢出的情况下，进行预测
			//把压缩大小设置为预测的压缩大小
			oData.resize(deflateBound(&zs, (uLong)iData.size()));
		}
		
		//设置压缩后大小与待处理大小
		size_t szCompressedSize = 0;
		size_t szRemainingSize = iData.size();
		int iRet = Z_OK;
		do
		{
			//计算剩余大小并在不足时扩容
			size_t szOut = oData.size() - szCompressedSize;
			if (szOut == 0)
			{
				oData.resize(oData.size() * 2);
				szOut = oData.size() - szCompressedSize;
			}

			//获取新的地址
			zs.next_out = (Bytef *)(&oData.data()[szCompressedSize]);

			//如果输入被消耗完，重新赋值
			if (zs.avail_in == 0)
			{
				constexpr uInt uIntMax = (uInt)-1;
				zs.avail_in = szRemainingSize > (size_t)uIntMax ? uIntMax : (uInt)szRemainingSize;
				szRemainingSize -= zs.avail_in;//缩小剩余待处理大小
			}

			//如果输出大小耗尽，重新赋值
			if (zs.avail_out == 0)
			{
				constexpr uInt uIntMax = (uInt)-1;
				zs.avail_out = szOut > (size_t)uIntMax ? uIntMax : (uInt)szOut;
				//这里不对szOut处理，因为会在开头与结尾计算
			}
			
			//解压，如果剩余不为0则代表分块了，使用Z_NO_FLUSH，否则Z_FINISH
			iRet = deflate(&zs, szRemainingSize != 0 ? Z_NO_FLUSH : Z_FINISH);

			//计算本次压缩的大小
			szCompressedSize += szOut - zs.avail_out;
		} while (iRet == Z_OK || iRet == Z_BUF_ERROR);//只要没错误（缓冲区不够大除外）或者没到结尾就继续运行

		//结束并设置大小
		deflateEnd(&zs);
		oData.resize(szCompressedSize);

		//错误处理
		if (iRet != Z_STREAM_END)
		{
			if (zs.msg != NULL)
			{
				throw std::runtime_error(std::string("Zlib compression failed with error message: ") + std::string(zs.msg));
			}
			else
			{
				throw std::runtime_error(std::string("Zlib compression failed with error code: ") + std::to_string(iRet));
			}
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

#include "vcpkg_config.h"//二次包含取消定义防止宏污染