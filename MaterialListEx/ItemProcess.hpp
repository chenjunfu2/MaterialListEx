#pragma once

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"

#include <xxhash.h>
#include <compare>

struct ItemInfo
{
	NBT_Type::String sName{};
	NBT_Type::Compound cpdTag{};
	uint64_t u64Hash{ DataHash() };//初始化顺序严格按照声明顺序，此处无问题

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0xDE35'B92A'7F41'806C;

		if (cpdTag.Empty())//tag为空只计算名称
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
	static size_t Hash(const ItemInfo &self) noexcept
	{
		return self.u64Hash;
	}

	static bool Equal(const ItemInfo &_l, const ItemInfo &_r) noexcept
	{
		return	_l.u64Hash	==	_r.u64Hash	&&
				_l.sName	==	_r.sName	&&
				_l.cpdTag	==	_r.cpdTag;//开销最大放最后
	}

	inline std::partial_ordering operator<=>(const ItemInfo &_r) const noexcept
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
		return cpdTag <=> _r.cpdTag;//开销最大放最后
	}
};

struct NoTagItemInfo
{
	NBT_Type::String sName{};
	uint64_t u64Hash{ DataHash() };

public:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0x83B0'1A83'062C'4F5D;

		return XXH64(sName.data(), sName.size(), HASH_SEED);
	}

public:
	static size_t Hash(const NoTagItemInfo &self) noexcept
	{
		return self.u64Hash;
	}

	static bool Equal(const NoTagItemInfo &_l, const NoTagItemInfo &_r) noexcept
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName;
	}

	inline std::strong_ordering operator<=>(const NoTagItemInfo &_r) const noexcept
	{
		//先按照哈希序
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//后按照名称序
		return sName <=> _r.sName;
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
		NBT_Type::String sItemName{};//物品名
		NBT_Type::Compound cpdItemTag{};//物品标签
		uint64_t u64ItemCount = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;

	struct NoTagItem
	{
		NBT_Type::String sItemName{};//物品名
		uint64_t u64Counter = 0;//物品计数器
	};

	using NoTagItemList = std::vector<NoTagItem>;

public:
	/*
	有一个重大问题，如果这个实体堆叠了，count不为1，并且这个实体是容器，则里面所含有的所有东西都得乘以它的堆叠倍率！
	*/
	//要求用户放弃原始listItemStack的所有权
	static ItemStackList ItemStackListUnpackContainer(ItemStackList &&listItemStack, size_t szStackDepth = 64)
	{
		ItemStackList ret;
		ret.reserve(listItemStack.size());//提前扩容（但是解包的情况下可能还会添加新的东西）
		for (auto &it : listItemStack)
		{
			ItemStackUnpackContainer(std::move(it), ret, szStackDepth);
		}
		return ret;
	}

	static ItemStack ItemCompoundToItemStack(const NBT_Type::Compound &cpdItem)
	{
		auto sName = cpdItem.HasString(MU8STR("id"));
		auto cpdTag = cpdItem.HasCompound(MU8STR("tag"));
		auto byteCount = cpdItem.HasByte(MU8STR("Count"));
		return ItemStack
		{
			sName == NULL ? NBT_Type::String{} : *sName,
			cpdTag == NULL ? NBT_Type::Compound{} : *cpdTag,
			byteCount == NULL ? uint64_t{} : (uint64_t)(uint8_t)*byteCount
		};
	}

	static ItemStack ItemCompoundToItemStack(NBT_Type::Compound &&cpdItem)
	{
		auto sName = cpdItem.HasString(MU8STR("id"));
		auto cpdTag = cpdItem.HasCompound(MU8STR("tag"));
		auto byteCount = cpdItem.HasByte(MU8STR("Count"));
		return ItemStack
		{
			sName == NULL ? NBT_Type::String{} : std::move(*sName),
			cpdTag == NULL ? NBT_Type::Compound{} : std::move(*cpdTag),
			byteCount == NULL ? uint64_t{} : (uint64_t)(uint8_t)*byteCount//内置类型直接拷贝，移动个P
		};
	}

	//TODO:从方块实体那边把逻辑拿来，然后修改区域处理，使得item处理在每个单独的转换后调用一次（方块可跳过，不存在tag）
private:
	/*
		只解包已知方块（各种潜影盒+收纳袋）除此之外全部排除
		为什么不解包带nbt的方块实体？因为这东西只能通过指令和创造获得，没办法跟
		正常容器一样放进去，并且很多方块实体存储的时候不带方块实体id，得根据名字转换
		非常麻烦，并且如果我这么做了，那么带nbt容器的实体（比如漏斗矿车这种）
		为了与方块实体一样也需要解，甚至刷怪蛋是容器实体的情况下也可以含物品，那是不是也要解？
		越来越复杂了，而这种东西甚至只能通过指令获得，完全没必要处理，直接保留原格式带nbt输出即可
	*/

	static void ItemStackUnpackContainer(ItemStack &&stItemStack, ItemStackList &listItemStack, size_t szStackDepth)
	{
		if (szStackDepth <= 0)
		{
			return;//栈深度超了，直接返回
		}

		if (stItemStack.cpdItemTag.Empty())//根本没有tag，下面不用判断了
		{
			goto push_return;
		}

		//有tag的情况下看看是不是需要解包的容器，是就递归解，否则返回
		//因为潜影盒有n多种颜色，都是前缀形式，所以判断后缀确定是不是潜影盒
		//为什么不判断方块实体id，不会有这么多变种？你问我我问谁，麻将抽象的很，有的方块实体根本不给你带id
		if (stItemStack.sItemName.ends_with(MU8STR("shulker_box")))
		{
			//查找tag内有没有BlockEntityTag，没有则插入返回
			auto pcpdBlockEntityTag = stItemStack.cpdItemTag.HasCompound(MU8STR("BlockEntityTag"));
			if (pcpdBlockEntityTag == NULL)
			{
				goto push_return;
			}

			//现在找内部的Items（潜影盒内部只能是这个，不用想了）
			auto pcpdItems = pcpdBlockEntityTag->Search(MU8STR("Items"));//这里不用HasXXX而是Search是为了不解包出类型，兼容TraversalItems参数
			TraversalItems(pcpdItems, listItemStack, stItemStack.u64ItemCount, szStackDepth);//获取当前容器的物品个数作为容器内所有物品的倍率
		}
		else if (stItemStack.sItemName == MU8STR("minecraft:bundle"))//收纳袋特殊处理
		{
			//直接找到Items
			auto plistItems = stItemStack.cpdItemTag.Search(MU8STR("Items"));//这里不用HasXXX而是Search是为了不解包出类型，兼容TraversalItems参数
			TraversalItems(plistItems, listItemStack, stItemStack.u64ItemCount, szStackDepth);
		}
		//else {} //上面几个都不是，无需解包，直接插入就行

		
	push_return:
		//不论如何都要进行插入，如果前面进行了解包那么插入的物品的nbt会变短，否则直接插入
		listItemStack.push_back(std::move(stItemStack));
		return;
	}

	static void TraversalItems(NBT_Node *pItems, ItemStackList &listItemStack, uint64_t u64Scale, size_t szStackDepth)
	{
		if (pItems == NULL)
		{
			return;//为空直接返回
		}

		//判断类型
		auto tag = pItems->GetTag();
		if (tag == NBT_TAG::Compound)
		{
			auto &tmp = pItems->GetCompound();
			ItemStack tmpItem = ItemCompoundToItemStack(std::move(tmp));
			pItems->GetCompound().Clear();//清空（内部数据已经转移所有权，不应该再使用）

			//对内部的物品乘以倍率缩放
			tmpItem.u64ItemCount *= u64Scale;
			ItemStackUnpackContainer(std::move(tmpItem), listItemStack, szStackDepth - 1);//递归 - 1
		}
		else if (tag == NBT_TAG::List)
		{
			auto &tmp = pItems->GetList();
			for (auto &it : tmp)//遍历list
			{
				ItemStack tmpItem = ItemCompoundToItemStack(std::move(it.GetCompound()));
				//这里先不清理
				//对内部的物品乘以倍率缩放
				tmpItem.u64ItemCount *= u64Scale;
				ItemStackUnpackContainer(std::move(tmpItem), listItemStack, szStackDepth - 1);//递归 - 1
			}
			//最后统一清空
			tmp.Clear();//清空（内部数据已经转移所有权，不应该再使用）
		}
		//else {}//忽略

		return;
	}
};