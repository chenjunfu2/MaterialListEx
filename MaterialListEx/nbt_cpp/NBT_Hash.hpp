#pragma once

#include <xxhash.h>
#include <type_traits>

/// @file
/// @brief 哈希工具集

/// @brief 一个封装xxhash调用的用于计算哈希的辅助类
/// @note 该类仅在安装xxhash的情况下有效
class NBT_Hash
{
public:
	/// @brief 类型别名，xxhash类型封装，类型必须是uint64_t
	using HASH_T = XXH64_hash_t;
	static_assert(std::is_same_v<HASH_T, uint64_t>, "Hash type does not match the required type.");

private:
	using STATE_T = XXH64_state_t;
	STATE_T *pHashState = nullptr;

	void Clear(void)
	{
		if (pHashState != nullptr)
		{
			XXH64_freeState(pHashState);
			pHashState = nullptr;
		}
	}

public:
	/// @brief 通过哈希种子构造并初始化xxhash对象
	/// @param tHashSeed 哈希种子
	NBT_Hash(const HASH_T &tHashSeed) :pHashState(XXH64_createState())
	{
		XXH64_reset(pHashState, tHashSeed);
	}

	/// @brief 析构并释放xxhash对象，然后置空
	~NBT_Hash(void)
	{
		Clear();
	}

	/// @brief 禁止拷贝构造
	NBT_Hash(const NBT_Hash &_Copy) = delete;
	/// @brief 移动构造
	/// @param _Move 右值对象
	NBT_Hash(NBT_Hash &&_Move) noexcept :pHashState(_Move.pHashState)
	{
		_Move.pHashState = nullptr;
	}

	/// @brief 禁止拷贝赋值
	NBT_Hash &operator=(const NBT_Hash &) = delete;
	/// @brief 移动赋值
	/// @param _Move 右值对象
	/// @return 对象的引用
	NBT_Hash &operator=(NBT_Hash &&_Move) noexcept
	{
		Clear();

		pHashState = _Move.pHashState;
		_Move.pHashState = nullptr;

		return *this;
	}

	/// @brief 根据之前已添加的数据，获取当前的哈希值
	/// @return 哈希值
	HASH_T Digest(void)
	{
		return XXH64_digest(pHashState);
	}

	/// @brief 添加指针指向的数据更新哈希
	/// @param pData 数据指针
	/// @param szSize 数据大小（字节）
	/// @note 通过指针访问指定字节数并更新哈希
	void Update(const void *pData, size_t szSize)
	{
		XXH64_update(pHashState, pData, szSize);
	}

	/// @brief 通过可平凡复制类型更新哈希
	/// @tparam T 可平凡复制类型
	/// @param tData 类型的引用
	/// @note 自动计算类型字节数并更新哈希
	template<typename T>
	requires(std::is_trivially_copyable_v<T>)
	void Update(const T &tData)
	{
		Update(&tData, sizeof(tData));
	}

	/// @brief 通过可平凡复制类型的数组更新哈希
	/// @tparam T 可平凡复制类型
	/// @param tDataArr 可平凡复制类型的数组引用
	/// @note 自动计算数组字节数并更新哈希
	template<typename T, size_t N>
	requires(std::is_trivially_copyable_v<T>)
	void Update(const T(&tDataArr)[N])
	{
		Update(&tDataArr, sizeof(tDataArr));
	}

public:
	/// @brief 直接通过指针指向的数据获取哈希
	/// @param pData 数据指针
	/// @param szSize 数据大小（字节）
	/// @param tHashSeed 哈希种子
	/// @return 计算的哈希值
	/// @note 直接计算并返回，无状态，静态函数
	static HASH_T Hash(const void *pData, size_t szSize, HASH_T tHashSeed)
	{
		return XXH64(pData, szSize, tHashSeed);
	}

	/// @brief 直接通过可平凡复制类型获取哈希
	/// @tparam T 可平凡复制类型
	/// @param tData 可平凡复制类型的引用
	/// @param tHashSeed 哈希种子
	/// @return 计算的哈希值
	/// @note 自动获取类型字节数并直接计算后返回，无状态，静态函数
	template<typename T>
	requires(std::is_trivially_copyable_v<T>)
	static HASH_T Hash(const T &tData, HASH_T tHashSeed)
	{
		return Hash(&tData, sizeof(tData), tHashSeed);
	}

	/// @brief 直接通过可平凡复制类型的数组获取哈希
	/// @tparam T 可平凡复制类型
	/// @param tDataArr 可平凡复制类型的数组引用
	/// @param tHashSeed 哈希种子
	/// @return 计算的哈希值
	/// @note 自动获取数组字节数并直接计算后返回，无状态，静态函数
	template<typename T, size_t N>
	requires(std::is_trivially_copyable_v<T>)
	static HASH_T Hash(const T(&tDataArr)[N], HASH_T tHashSeed)
	{
		return Hash(&tDataArr, sizeof(tDataArr), tHashSeed);
	}
};