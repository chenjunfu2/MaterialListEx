#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>//size_t
#include <string.h>//memcpy
#include <fstream>
#include <vector>
#include <filesystem>

#include "NBT_Print.hpp"//打印输出

#include "vcpkg_config.h"//包含vcpkg生成的配置以确认库安装情况

#ifdef CJF2_NBT_CPP_USE_ZLIB

/*zlib*/
#define ZLIB_CONST
#include <zlib.h>
/*zlib*/

#endif

/// @file
/// @brief IO工具集

/// @brief 用于提供nbt文件读写，解压与压缩功能
class NBT_IO
{
	/// @brief 禁止构造
	NBT_IO(void) = delete;
	/// @brief 禁止析构
	~NBT_IO(void) = delete;

public:
	/// @brief 默认输入流类，用于从标准库容器中读取数据
	/// @tparam T 数据容器类型，必须满足以下要求：
	/// - value_type的大小必须为1字节
	/// - value_type必须是可平凡复制的类型
	/// @note 这个类用于标准库的顺序容器，非标准库容器顺序请使用其它的自定义流对象，而非使用此对象，
	/// 因为此对象对标准库容器的部分实现存在假设，其它非标准库容器极有可能不兼容导致未定义行为。
	/// 可以注意到部分接口在类中并未使用，这是未来扩展时可能用到的，如果自定义流对象，则可以省略部分未使用的接口。
	template <typename T = std::vector<uint8_t>>
	class DefaultInputStream
	{
	private:
		const T &tData = {};
		size_t szIndex = 0;

	public:
		/// @brief 容器类型
		using StreamType = T;
		/// @brief 容器值类型
		using ValueType = typename T::value_type;

		//静态断言确保字节流与可平凡拷贝
		static_assert(sizeof(ValueType) == 1, "Error ValueType Size");
		static_assert(std::is_trivially_copyable_v<ValueType>, "ValueType Must Be Trivially Copyable");

		/// @brief 禁止使用临时对象构造
		DefaultInputStream(const T &&_tData, size_t szStartIdx = 0) = delete;

		/// @brief 构造函数
		/// @param _tData 输入数据容器的常量引用
		/// @param szStartIdx 起始读取索引位置
		/// @note 指定szStartIdx为0则从头开始读取，否则从指定索引位置开始读取
		DefaultInputStream(const T &_tData, size_t szStartIdx = 0) :tData(_tData), szIndex(szStartIdx)
		{}

		/// @brief 默认析构函数
		~DefaultInputStream(void) = default;
		/// @brief 禁止拷贝构造
		DefaultInputStream(const DefaultInputStream &) = delete;
		/// @brief 禁止移动构造
		DefaultInputStream(DefaultInputStream &&) = delete;
		/// @brief 禁止拷贝赋值
		DefaultInputStream &operator=(const DefaultInputStream &) = delete;
		/// @brief 禁止移动赋值
		DefaultInputStream &operator=(DefaultInputStream &&) = delete;

		/// @brief 下标访问运算符
		/// @param szIndex 索引位置
		/// @return 对应位置的常量引用
		/// @note 这个接口一般用于随机访问流中的数据，不改变当前读取位置，调用者保证访问范围合法
		const ValueType &operator[](size_t szIndex) const noexcept
		{
			return tData[szIndex];
		}

		/// @brief 获取下一个字节并推进读取位置
		/// @return 下一个字节的常量引用
		/// @note 这个接口一般用于逐个从流中读取数据
		const ValueType &GetNext() noexcept
		{
			return tData[szIndex++];
		}

		/// @brief 从流中读取一段数据
		/// @param pDest 指向要读取数据的目标缓冲区的指针
		/// @param szSize 要读取的数据大小（字节数）
		/// @note 这个接口一般用于批量从流中读取数据
		void GetRange(void *pDest, size_t szSize) noexcept
		{
			memcpy(pDest, &tData[szIndex], szSize);
			szIndex += szSize;
		}

		/// @brief 回退一个字节的读取
		/// @note 如果当前已在流的起始位置，则不会进行任何操作
		void UnGet() noexcept
		{
			if (szIndex != 0)
			{
				--szIndex;
			}
		}

		/// @brief 获取当前读取位置的指针
		/// @return 指向当前读取位置数据的指针
		/// @note 这个接口一般用于直接访问当前及后续的数据而不拷贝
		const ValueType *CurData() const noexcept
		{
			return &(tData[szIndex]);
		}

		/// @brief 向后推进读取
		/// @param szSize 要推进的字节数
		/// @return 推进后的新读取位置
		/// @note 这个接口一般与CurData合并使用，通过CurData读取一段数据后，调用此接口移动当前读取位置
		size_t AddIndex(size_t szSize) noexcept
		{
			return szIndex += szSize;
		}

		/// @brief 向前撤销读取
		/// @param szSize 要撤销的字节数
		/// @return 撤销后的新读取位置
		/// @note 这个接口一般用于在某些情况下撤销一部分的读取
		size_t SubIndex(size_t szSize) noexcept
		{
			return szIndex -= szSize;
		}

		/// @brief 检查是否已到达流末尾
		/// @return 如果已到达或超过流末尾则返回true，否则返回false
		bool IsEnd() const noexcept
		{
			return szIndex >= tData.size();
		}

		/// @brief 获取流的总大小
		/// @return 流的总大小，以字节数计
		size_t Size() const noexcept
		{
			return tData.size();
		}

		/// @brief 检查是否还有足够的数据可供读取
		/// @param szSize 需要读取的数据大小
		/// @return 如果剩余数据足够则返回true，否则返回false
		bool HasAvailData(size_t szSize) const noexcept
		{
			return (tData.size() - szIndex) >= szSize;
		}

		/// @brief 重置流读取位置到起始处
		void Reset() noexcept
		{
			szIndex = 0;
		}

		/// @brief 获取底层数据的起始指针
		/// @return 指向底层数据起始位置的常量指针
		const ValueType *BaseData() const noexcept
		{
			return tData.data();
		}

		/// @brief 获取当前读取位置（只读）
		/// @return 当前读取位置索引
		size_t Index() const noexcept
		{
			return szIndex;
		}

		/// @brief 获取当前读取位置（可写）
		/// @return 当前读取位置索引的引用
		/// @note 这个接口允许直接修改读取位置，调用者保证修改后的索引范围合法
		size_t &Index() noexcept
		{
			return szIndex;
		}
	};

	/// @brief 默认输出流类，用于将数据写入到标准库容器中
	/// @tparam T 数据容器类型，必须满足以下要求：
	/// - value_type的大小必须为1字节
	/// - value_type必须是可平凡复制的类型
	/// @note 这个类用于标准库的顺序容器，非标准库容器顺序请使用其它的自定义流对象，而非使用此对象，
	/// 因为此对象对标准库容器的部分实现存在假设，其它非标准库容器极有可能不兼容导致未定义行为。
	/// 可以注意到部分接口在类中并未使用，这是未来扩展时可能用到的，如果自定义流对象，则可以省略部分未使用的接口。
	template <typename T = std::vector<uint8_t>>
	class DefaultOutputStream
	{
	private:
		T &tData;

	public:
		/// @brief 容器类型
		using StreamType = T;
		/// @brief 容器值类型
		using ValueType = typename T::value_type;

		//静态断言确保字节流与可平凡拷贝
		static_assert(sizeof(ValueType) == 1, "Error ValueType Size");
		static_assert(std::is_trivially_copyable_v<ValueType>, "ValueType Must Be Trivially Copyable");

		/// @brief 构造函数
		/// @param _tData 输出数据容器的引用
		/// @param szStartIdx 起始索引，容器会调整大小到此索引位置
		/// @note 索引位置后的数据都会被删除，然后从索引当前位置开始写入
		DefaultOutputStream(T &_tData, size_t szStartIdx = 0) :tData(_tData)//引用天生无法使用临时值构造，无需担心临时值构造导致的悬空引用
		{
			tData.resize(szStartIdx);
		}

		/// @brief 默认析构函数
		~DefaultOutputStream(void) = default;
		/// @brief 禁止拷贝构造
		DefaultOutputStream(const DefaultOutputStream &) = delete;
		/// @brief 禁止移动构造
		DefaultOutputStream(DefaultOutputStream &&) = delete;
		/// @brief 禁止拷贝赋值
		DefaultOutputStream &operator=(const DefaultOutputStream &) = delete;
		/// @brief 禁止移动赋值
		DefaultOutputStream &operator=(DefaultOutputStream &&) = delete;

		/// @brief 下标访问运算符
		/// @param szIndex 索引位置
		/// @return 对应位置的常量引用
		/// @note 这个接口一般用于随机访问流中的数据，而不修改流，调用者保证访问范围合法
		const ValueType &operator[](size_t szIndex) const noexcept
		{
			return tData[szIndex];
		}

		/// @brief 向流中写入写入单个值
		/// @tparam V 元素类型，必须可构造为ValueType
		/// @param c 要写入的元素
		/// @note 这个接口一般用于逐个向流中写入数据
		template<typename V>
		requires(std::is_constructible_v<ValueType, V &&>)
		void PutOnce(V &&c)
		{
			tData.push_back(std::forward<V>(c));
		}

		/// @brief 向流中写入一段数据
		/// @param pData 指向要写入数据的缓冲区的指针
		/// @param szSize 要写入的数据大小（字节数）
		/// @note 这个接口一般用于批量向流中写入数据
		void PutRange(const ValueType *pData, size_t szSize)
		{
			//tData.insert(tData.end(), &pData[0], &pData[szSize]);

			//使用更高效的实现，而不是迭代器写入
			size_t szCurSize = tData.size();
			tData.resize(szCurSize + szSize);
			memcpy(&tData.data()[szCurSize], &pData[0], szSize);
		}

		/// @brief 预分配额外容量
		/// @param szAddSize 要额外分配的容量大小（字节数）
		/// @note 这个接口一般是用于性能优化的，提前要求流预留空间以便后续的写入。
		/// 请不要假设在所有写入操作之前都会进行此调用，这个接口只有部分情况会用到。
		void AddReserve(size_t szAddSize)
		{
			tData.reserve(tData.size() + szAddSize);
		}

		/// @brief 删除（撤销）最后一个写入的字节
		void UnPut(void) noexcept
		{
			tData.pop_back();
		}

		/// @brief 获取当前字节流中已有的数据大小
		/// @return 数据大小，以字节数计
		size_t Size(void) const noexcept
		{
			return tData.size();
		}

		/// @brief 重置流，清空所有数据
		void Reset(void) noexcept
		{
			tData.clear();
		}

		/// @brief 获取底层数据的常量引用
		/// @return 底层数据容器的常量引用
		const T &Data(void) const noexcept
		{
			return tData;
		}

		/// @brief 获取底层数据的非常量引用
		/// @return 底层数据容器的非常量引用
		T &Data(void) noexcept
		{
			return tData;
		}
	};


public:
	/// @brief 从任意顺序容器写出字节流数据到指定文件名的文件中
	/// @tparam T 任意顺序容器类型
	/// @param pathFileName 目标文件名
	/// @param tData 顺序容器的引用
	/// @return 写出是否成功
	/// @note 如果文件已存在则直接清空并覆盖，未存在则创建文件。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
	template<typename T = std::vector<uint8_t>>
	requires (sizeof(typename T::value_type) == 1 && std::is_trivially_copyable_v<typename T::value_type>)
	static bool WriteFile(const std::filesystem::path &pathFileName, const T &tData)
	{
		std::fstream fWrite;
		fWrite.open(pathFileName, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
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

	/// @brief 从指定文件名的文件中读取字节流数据到任意顺序容器中
	/// @tparam T 任意顺序容器类型
	/// @param pathFileName 目标文件名
	/// @param[out] tData 顺序容器的引用
	/// @return 读取是否成功
	/// @note 如果文件不存在，则失败。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
	template<typename T = std::vector<uint8_t>>
	requires (sizeof(typename T::value_type) == 1 && std::is_trivially_copyable_v<typename T::value_type>)
	static bool ReadFile(const std::filesystem::path &pathFileName, T &tData)
	{
		std::fstream fRead;
		fRead.open(pathFileName, std::ios_base::binary | std::ios_base::in);
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

	/// @brief 判断指定文件名的文件是否存在
	/// @param pathFileName 目标文件名
	/// @return 文件是否存在
	/// @note 如果判断出现错误，也返回不存在。只有明确返回存在的文件存在，
	/// 否则文件可能不存在，也可能存在但是因为其它原因无法获取。
	static bool IsFileExist(const std::filesystem::path &pathFileName)
	{
		std::error_code ec;//判断这东西是不是true确定有没有error
		bool bExists = std::filesystem::exists(pathFileName, ec);

		return !ec && bExists;//没有错误并且存在
	}

#ifdef CJF2_NBT_CPP_USE_ZLIB

	/// @brief 通过字节流开始的两个字节判断是否可能是Zlib压缩
	/// @param u8DataFirst 字节流的第一个字节
	/// @param u8DataSecond 字节流的第二个字节
	/// @return 是否可能是Zlib压缩
	/// @note 仅用于可能性判断，具体是否Zlib压缩需要靠解压例程决定，仅作为快速判断的辅助函数。
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

	/// @brief 通过字节流开始的两个字节判断是否可能是Gzip压缩
	/// @param u8DataFirst 字节流的第一个字节
	/// @param u8DataSecond 字节流的第二个字节
	/// @return 是否可能是Gzip压缩
	/// @note 仅用于可能性判断，具体是否Gzip压缩需要靠解压例程决定，仅作为快速判断的辅助函数。
	static bool IsGzip(uint8_t u8DataFirst, uint8_t u8DataSecond)
	{
		return u8DataFirst == (uint8_t)0x1F && u8DataSecond == (uint8_t)0x8B;
	}

	/// @brief 判断一个顺序容器存储的字节流是否可能存在压缩
	/// @tparam T 任意顺序容器类型
	/// @param tData 顺序容器类型的引用
	/// @return 是否可能存在压缩
	/// @note 仅用于可能性判断，具体是否压缩需要靠解压例程决定，仅作为快速判断的辅助函数。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
	template<typename T>
	requires (sizeof(typename T::value_type) == 1 && std::is_trivially_copyable_v<typename T::value_type>)
	static bool IsDataZipped(const T &tData)
	{
		if (tData.size() <= 2)
		{
			return false;
		}

		uint8_t u8DataFirst = (uint8_t)tData[0];
		uint8_t u8DataSecond = (uint8_t)tData[1];

		return IsZlib(u8DataFirst, u8DataSecond) || IsGzip(u8DataFirst, u8DataSecond);
	}

	/// @brief 解压数据，自动判断Zlib或Gzip并解压，如果失败则抛出异常
	/// @tparam I 输入的顺序容器类型
	/// @tparam O 输出的顺序容器类型
	/// @param[out] oData 输入的顺序容器引用
	/// @param iData 输出的顺序容器引用
	/// @note oData和iData不能引用相同对象，否则错误。如果输入为空，则输出也为空。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
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

	/// @brief 压缩数据，默认压缩为Gzip，也就是NBT格式的标准压缩类型，如果失败则抛出异常
	/// @tparam I 输入的顺序容器类型
	/// @tparam O 输出的顺序容器类型
	/// @param[out] oData 输入的顺序容器引用
	/// @param iData 输出的顺序容器引用
	/// @param iLevel 压缩等级
	/// @note oData和iData不能引用相同对象，否则错误。如果输入为空，则输出也为空。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
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

	/// @brief 解压数据，但是不抛出异常，而是通过funcErrInfo打印异常信息并返回成功与否
	/// @tparam I 输入的顺序容器类型
	/// @tparam O 输出的顺序容器类型
	/// @tparam ErrInfoFunc 打印异常信息的仿函数类型
	/// @param[out] oData 输入的顺序容器引用
	/// @param iData 输出的顺序容器引用
	/// @param funcErrInfo 打印异常信息的仿函数
	/// @return 操作是否成功
	/// @note funcErrInfo默认实现为NBT_Print并输出到标准异常stderr，用户可以自定义。
	/// 类似于NBT_Print的仿函数类型并替换输出例程，具体情况请参照NBT_Print类的说明。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
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

	/// @brief 压缩数据，但是不抛出异常，而是通过funcErrInfo打印异常信息并返回成功与否
	/// @tparam I 输入的顺序容器类型
	/// @tparam O 输出的顺序容器类型
	/// @tparam ErrInfoFunc 打印异常信息的仿函数类型
	/// @param[out] oData 输入的顺序容器引用
	/// @param iData 输出的顺序容器引用
	/// @param iLevel 压缩等级
	/// @param funcErrInfo 打印异常信息的仿函数
	/// @return 操作是否成功
	/// @note funcErrInfo默认实现为NBT_Print并输出到标准异常stderr，用户可以自定义。
	/// 类似于NBT_Print的仿函数类型并替换输出例程，具体情况请参照NBT_Print类的说明。
	/// 顺序容器必须存储字节流，内部的值类型大小必须为1，且必须可平凡拷贝。
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
