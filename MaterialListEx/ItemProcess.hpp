#pragma once

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"

#include <xxhash.h>
#include <compare>

struct ItemInfo
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};
	uint64_t u64Hash{ DataHash() };//初始化顺序严格按照声明顺序，此处无问题

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0xDE35'B92A'7F41'806C;

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
	static size_t Hash(const ItemInfo &self)
	{
		return self.u64Hash;
	}

	static bool Equal(const ItemInfo &_l, const ItemInfo &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName && _l.cpdTag == _r.cpdTag;
	}

	inline std::partial_ordering operator<=>(const ItemInfo &_r) const
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

/*
物品需要解析药箭、药水、附魔、谜之炖菜、烟花火箭等级样式，旗帜样式等已知可读NBT信息进行格式化输出
决定把方块实体对于物品的处理逻辑移动到此，这样实体内物品处理也会走这里
突然想起来一个重大问题，如果带有容器信息的物品堆叠了，Count不为1，则里面所含有的所有东西的数量都得乘以它的堆叠倍率！

只对部分带有容器实体信息的实体递归解包：潜影盒，收纳袋

*/
class ItemProcess
{
public:
	ItemProcess() = delete;
	~ItemProcess() = delete;

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		NBT_Node::NBT_Compound cpdItemTag{};//物品标签
		NBT_Node::NBT_Byte byteItemCount = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;

	struct BlockItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		uint64_t u64Counter = 0;//物品计数器
	};

	using BlockItemStackList = std::vector<BlockItemStack>;

public:
	/*
	有一个重大问题，如果这个实体堆叠了，count不为1，并且这个实体是容器，则里面所含有的所有东西都得乘以它的堆叠倍率！
	*/
	ItemStackList ItemStackUnpackToItemList(const ItemStack &stItem)
	{





	}

	//TODO:从方块实体那边把逻辑拿来，然后修改区域处理，使得item处理在每个单独的转换后调用一次（方块可跳过，不存在tag）
};