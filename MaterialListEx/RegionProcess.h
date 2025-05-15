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

	//map���ڲ���ͳ�ƣ�����Ҫ�����key�����ң�����key��ʲô����ν
	Map mapItemCounter{};//��������״̬����Ʒӳ��map
	//ͨ��vector����map��Ԫ�����򣨴洢pair���ã�std::reference_wrapper<>
	std::vector<std::reference_wrapper<MapPair>> vecSortItem{};

	static bool SortCmp(MapPair &l, MapPair &r)
	{
		if (l.second == r.second)//�����������°�key����
		{
			return l.first < r.first;//����
		}
		else//����val����
		{
			return l.second > r.second;//����
		}
	}
	
	void SortElement(void)
	{
		//��ǰ���ݼ��ٲ��뿪��
		vecSortItem.reserve(mapItemCounter.size());
		vecSortItem.assign(mapItemCounter.begin(), mapItemCounter.end());//��������Χ����
		//����Ʒ��������������
		std::sort(vecSortItem.begin(), vecSortItem.end(), SortCmp);
	}
};

struct TileEntityKey
{
	NBT_Node::NBT_String sName{};
	NBT_Node::NBT_Compound cpdTag{};
	uint64_t u64Hash{ DataHash() };//��ʼ��˳���ϸ�������˳�򣬴˴�������
private:
	uint64_t DataHash(void)
	{
		static_assert(std::is_same_v<XXH64_hash_t, uint64_t>, "Hash type does not match the required type.");

		constexpr static XXH64_hash_t HASH_SEED = 0xDE35B92A7F41706C;

		if (cpdTag.empty())//tagΪ��ֻ��������
		{
			return XXH64(sName.data(), sName.size(), HASH_SEED);
		}
		else//��Ϊ�������tag
		{
			return NBT_Helper::Hash(cpdTag, HASH_SEED,
				[this](XXH64_state_t *pHashState) -> void//��tag֮ǰ��������
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
		//�Ȱ��չ�ϣ��
		if (auto tmp = (u64Hash <=> _r.u64Hash); tmp != 0)
		{
			return tmp;
		}

		//����������
		if (auto tmp = (sName <=> _r.sName); tmp != 0)
		{
			return tmp;
		}

		//����ͬ�����Tag��
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
����ҩ����ҩˮ����ħ����֮���˵���֪�ɶ�NBT��Ϣ���и�ʽ�����
*/