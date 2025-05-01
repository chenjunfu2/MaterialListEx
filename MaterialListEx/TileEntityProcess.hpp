#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;


	struct TileEntityContainerStats
	{
		const NBT_Node::NBT_String *psTileEntityName{};
		struct
		{
			const NBT_Node *pItems{};
			NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
		};
	};

	static std::vector<TileEntityContainerStats> GetTileEntityContainerStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//获取方块实体列表
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityContainerStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//提前扩容

		for (const auto &it : listTileEntity)
		{
			/*
				有好几种情况：item-Compound、Items-List、特殊名称-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityContainerStats teStats{ &teCurrent.GetString(MU8STR("id")) };


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

			//用于映射特殊的方块实体容器里物品的名字
			const static std::unordered_map<NBT_Node::NBT_String, DataInfo> mapContainerItemName =
			{
				{MU8STR("minecraft:jukebox"),{MU8STR("RecordItem"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:lectern"),{MU8STR("Book"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:brushable_block"),{MU8STR("item"),NBT_Node::TAG_Compound}},
			};

			//查找方块实体id是否在map中
			const auto findIt = mapContainerItemName.find(*teStats.psTileEntityName);
			if (findIt == mapContainerItemName.end())//不在，不是可以存放物品的容器，跳过
			{
				continue;//跳过此方块实体
			}
			
			//通过映射名查找对应物品存储位置
			const auto pSearch = teCurrent.Search(findIt->second.sTagName);
			if (pSearch == NULL)//查找失败
			{
				continue;//跳过此方块实体
			}

			//放入结构内
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
		NBT_Node cpdItemTag{};//物品标签
		NBT_Node::NBT_Byte byteItemCount = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;

	static ItemStackList TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats)
	{
		if (stContainerStats.enType == NBT_Node::TAG_End)
		{
			return {};
		}
		if (stContainerStats.pItems == NULL)
		{
			return {};
		}

		//读取每个物品的集合并解包出内容插入到ItemStack
		ItemStackList vtItemStackList{};
		auto EmplaceBackItem = [&vtItemStackList](const NBT_Node::NBT_Compound &cpdItem) -> void
		{
			const auto pcpdTag = cpdItem.Search(MU8STR("tag"));
			ItemStack tmp =
			{
				.sItemName = cpdItem.GetString(MU8STR("id")),
				.cpdItemTag = ((pcpdTag != NULL) ? *pcpdTag : NBT_Node{}),
				.byteItemCount = cpdItem.GetByte("Count"),
			};

			vtItemStackList.emplace_back(std::move(tmp));
		};
		
		if (stContainerStats.enType == NBT_Node::TAG_Compound)//只有一格物品
		{
			EmplaceBackItem(stContainerStats.pItems->GetCompound());
		}
		else if(stContainerStats.enType == NBT_Node::TAG_List)//多格物品列表
		{
			for (const auto &it : stContainerStats.pItems->GetList())
			{
				EmplaceBackItem(it.GetCompound());
			}
		}
		//else {}

		return vtItemStackList;
	}
};