#pragma once

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"


#include <xxhash.h>

struct Item
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};
	uint64_t u64Hash{ DataHash() };//初始化顺序严格按照声明顺序，此处无问题
private:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0xDE35B92A7F41706C;

		if (cpdTag.empty())//tag为空只计算名称
		{
			return XXH64(sName.data(), sName.size(), HASH_SEED);
		}
		else//不为空则计算tag
		{
			return NBT_Helper::Hash(cpdTag, HASH_SEED,
				[this](XXH64_state_t *pHashState) -> void//在tag之前加入名称
				{
					XXH64_update(pHashState, this->sName.data(), this->sName.size());
				});
		}
	}
public:

	static size_t Hash(const Item &self)
	{
		return self.u64Hash;
	}

	static bool Equal(const Item &_l, const Item &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName && _l.cpdTag == _r.cpdTag;
	}

	inline std::partial_ordering operator<=>(const Item &_r) const
	{
		//先按照哈希序
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//后按照名称序
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		//都相同最后按照Tag序
		return cpdTag <=> _r.cpdTag;
	}
};



class ItemProcess
{









};