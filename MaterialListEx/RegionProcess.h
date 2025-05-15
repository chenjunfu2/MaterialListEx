#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include <xxhash.h>

#include "NBT_Node.hpp"
#include "NBT_Helper.hpp"


template<typename Map>
struct MapSortList
{
	using MapPair = Map::value_type;

	//map用于查重统计，不需要真的用key来查找，所以key是什么无所谓
	Map mapItemCounter{};//创建方块状态到物品映射map
	//通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> vecSortItem{};

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//数量相等情况下按key排序
		{
			return l.first < r.first;//升序
		}
		else//否则val排序
		{
			return l.second > r.second;//降序
		}
	}
	
	void SortElement(void)
	{
		//提前扩容减少插入开销
		vecSortItem.reserve(mapItemCounter.size());
		vecSortItem.assign(mapItemCounter.begin(), mapItemCounter.end());//迭代器范围插入
		//对物品按数量进行排序
		std::sort(vecSortItem.begin(), vecSortItem.end(), SortCmp);
	}
};

struct TileEntityKey
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

	static size_t Hash(const TileEntityKey &self)
	{
		return self.u64Hash;
	}
	
	static bool Equal(const TileEntityKey &_l, const TileEntityKey &_r)
	{
		return _l.u64Hash == _r.u64Hash && _l.sName == _r.sName && _l.cpdTag == _r.cpdTag;
	}

	inline std::partial_ordering operator<=>(const TileEntityKey &_r) const
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


struct RegionStats
{
	NBT_Node::NBT_String sRegionName{};
	MapSortList<std::unordered_map<NBT_Node::NBT_String, uint64_t>> mslBlock{ .mapItemCounter{128} };
	//MapSortList<std::map<NBT_Node::NBT_String, uint64_t>> mslBlock{};
	MapSortList<std::unordered_map<TileEntityKey, uint64_t, decltype(&TileEntityKey::Hash), decltype(&TileEntityKey::Equal)>> mslTileEntityContainer{ {128 ,&TileEntityKey::Hash, &TileEntityKey::Equal} };
	//MapSortList<std::map<TileEntityKey, uint64_t>> mslTileEntityContainer{};
	//MapSortList mslEntity{};
	//MapSortList mslEntityContainer{};
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);

/*
TODO:
解析药箭、药水、附魔、谜之炖菜等已知可读NBT信息进行格式化输出
*/