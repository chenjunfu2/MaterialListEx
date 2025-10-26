#pragma once

#include <xxhash.h>
#include <type_traits>

class NBT_Hash
{
public:
	using HASH_T = XXH64_hash_t;
	static_assert(std::is_same_v<HASH_T, uint64_t>, "Hash type does not match the required type.");

private:
	using STATE_T = XXH64_state_t;
	STATE_T *pHashState = nullptr;

public:
	NBT_Hash(const HASH_T &tHashSeed) :pHashState(XXH64_createState())
	{
		XXH64_reset(pHashState, tHashSeed);
	}

	~NBT_Hash(void)
	{
		if (pHashState != nullptr)
		{
			XXH64_freeState(pHashState);
			pHashState = nullptr;
		}
	}

	NBT_Hash(const NBT_Hash &_Copy) = delete;
	NBT_Hash(NBT_Hash &&_Move) noexcept :pHashState(_Move.pHashState)
	{
		_Move.pHashState = nullptr;
	}

	NBT_Hash &operator=(const NBT_Hash &) = delete;
	NBT_Hash &operator=(NBT_Hash &&_Move) noexcept
	{
		pHashState = _Move.pHashState;
		_Move.pHashState = nullptr;
		return *this;
	}

	HASH_T Digest(void)
	{
		return XXH64_digest(pHashState);
	}

	void Update(const void *pData, size_t szSize)
	{
		XXH64_update(pHashState, pData, szSize);
	}

	template<typename T>
	requires(std::is_trivially_copyable_v<T>)
	void Update(const T &tData)
	{
		Update(&tData, sizeof(tData));
	}

	template<typename T, size_t N>
	requires(std::is_trivially_copyable_v<T>)
	void Update(const T(&tDataArr)[N])
	{
		Update(&tDataArr, sizeof(tDataArr));
	}

public:
	static HASH_T Hash(const void *pData, size_t szSize, HASH_T tHashSeed)
	{
		return XXH64(pData, szSize, tHashSeed);
	}

	template<typename T>
	requires(std::is_trivially_copyable_v<T>)
	static HASH_T Hash(const T &tData, HASH_T tHashSeed)
	{
		return Hash(&tData, sizeof(tData), tHashSeed);
	}

	template<typename T, size_t N>
	requires(std::is_trivially_copyable_v<T>)
	static HASH_T Hash(const T(&tDataArr)[N], HASH_T tHashSeed)
	{
		return Hash(&tDataArr, sizeof(tDataArr), tHashSeed);
	}
};