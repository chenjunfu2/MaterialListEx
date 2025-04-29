#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;


	struct TileEntityItemStats
	{
		const NBT_Node::NBT_String *psTileEntityName{};
		struct
		{
			const NBT_Node *pItems{};
			NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
		};
	};

	static std::vector<TileEntityItemStats> GetTileEntityItemStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//获取方块实体列表
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityItemStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//提前扩容

		for (const auto &it : listTileEntity)
		{
			/*
				有好几种情况：item-Compound、Items-List、特殊名称-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityItemStats teStats{ &teCurrent.GetString(MU8STR("id")) };


			//尝试寻找Items（普通多格容器）
			if (const auto pItems = teCurrent.Search(MU8STR("Items"));
				pItems != NULL && pItems->IsList())
			{
				teStats.pItems = pItems;
				teStats.enType = NBT_Node::TAG_List;
				//插入并跳过
				vtTileEntityStats.emplace_back(std::move(teStats));
				continue;
			}

			//尝试处理特殊方块
			struct DataInfo
			{
				NBT_Node::NBT_String sTagName{};
				NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
			};

			const static std::unordered_map<NBT_Node::NBT_String, DataInfo> mapTileTntityDataName =
			{
				{MU8STR("minecraft:jukebox"),{MU8STR("RecordItem"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:lectern"),{MU8STR("Book"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:brushable_block"),{MU8STR("item"),NBT_Node::TAG_Compound}},
			};

			//查找方块实体id是否在map中
			const auto findIt = mapTileTntityDataName.find(*teStats.psTileEntityName);
			if (findIt == mapTileTntityDataName.end())//不在，不是可以存放物品的容器，跳过
			{
				continue;//跳过此方块实体
			}
			
			//通过映射名查找对应物品存储位置
			const auto pSearch = teCurrent.Search(findIt->second.sTagName);
			if (pSearch == NULL)//查找失败
			{
				continue;//跳过此方块实体
			}

			teStats.enType = findIt->second.enType;
			teStats.pItems = pSearch;

			//执行到这里这说明操作完成
			vtTileEntityStats.emplace_back(std::move(teStats));
		}//for

		return vtTileEntityStats;
	}//func

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		NBT_Node::NBT_Compound pcpdItemTag{};//物品标签
		uint64_t u64Counter = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;

	static ItemStackList TileEntityItemStatsToItemStack(const TileEntityItemStats &stItemStats)
	{
		if (stItemStats.enType == NBT_Node::TAG_End)
		{
			return {};
		}
		if (stItemStats.pItems == NULL)
		{
			return {};
		}

		ItemStackList vtItemStackList{};
		if (stItemStats.enType == NBT_Node::TAG_Compound)//只有1个物品
		{





		}
		else if(stItemStats.enType == NBT_Node::TAG_List)//物品列表
		{





		}
		//else {}

		return vtItemStackList;
	}


};