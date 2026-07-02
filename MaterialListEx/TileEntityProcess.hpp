#pragma once

#include <NBT_Node.hpp>
#include "ItemProcess.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;

	struct TileEntityContainerStats
	{
		const NBT_Type::String *psTileEntityName{};
		const NBT_Node *pItems{};
	};

	using TileEntityContainerStatsList = std::vector<TileEntityContainerStats>;
	
private:
	//处理多种集合数据情况并映射到TileEntityContainerStats
	static bool MapTileEntityContainerStats(const NBT_Type::Compound &teCompound, TileEntityContainerStats &teStats)
	{
		/*
			有好几种情况：item-Compound、Items-List、特殊名称-Compound
		*/

		//尝试寻找Items（普通多格容器）
		if (const auto pItems = teCompound.Has(MU8STR("Items"));
			pItems != NULL && pItems->IsList())
		{
			teStats.pItems = pItems;
			return true;
		}

		//尝试处理特殊方块
		//用于映射特殊的方块实体容器里物品的名字
		const static std::vector<NBT_Type::String> listContainerTagName =
		{
			MU8STR("item"),
			MU8STR("RecordItem"),
			MU8STR("Book"),
		};

		//遍历所有可能的容器物品栏名称
		const NBT_Node *pContainerTag = NULL;
		for (auto &strTagName : listContainerTagName)
		{
			//通过遍历每个可能的名称查找对应物品存储位置
			pContainerTag = teCompound.Has(strTagName);
			if (pContainerTag != NULL)//查找成功
			{
				break;
			}
		}

		if (pContainerTag == NULL)
		{
			return false;//查找失败，不存在
		}

		//放入结构内
		teStats.pItems = pContainerTag;
		return true;
	}

public:
	static TileEntityContainerStatsList GetTileEntityContainerStats(const NBT_Type::Compound &RgCompound)
	{
		//尝试获取方块实体列表
		const auto pListTileEntity = RgCompound.HasList(MU8STR("TileEntities"));
		if (pListTileEntity == NULL)
		{
			return TileEntityContainerStatsList{};
		}

		const auto &listTileEntity = *pListTileEntity;
		TileEntityContainerStatsList listTileEntityStats{};
		listTileEntityStats.reserve(listTileEntity.Size());//提前扩容

		for (const auto &it : listTileEntity)
		{
			const auto &cur = GetCompound(it);

			//TODO:如果cur.HasString(MU8STR("id"))没有方块实体id，
			//则通过方块->方块实体映射表查找，找不到则为NULL
			//查找成功或失败后再创建teStats
			//特别的，只查找可以存放物品的方块实体容器，其余丢弃
			TileEntityContainerStats teStats{ cur.HasString(MU8STR("id")) };
			if (MapTileEntityContainerStats(cur, teStats))
			{
				listTileEntityStats.emplace_back(std::move(teStats));
			}
		}

		return listTileEntityStats;
	}

	static ItemProcess::ItemStackList TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats)
	{
		ItemProcess::ItemStackList listItemStack{};

		auto tag = stContainerStats.pItems->GetTag();
		if (tag == NBT_TAG::Compound)//只有一格物品
		{
			if (stContainerStats.pItems->GetCompound().Empty())
			{
				return listItemStack;//空，直接返回
			}
			listItemStack.push_back(ItemProcess::ItemCompoundToItemStack(stContainerStats.pItems->GetCompound()));
		}
		else if (tag == NBT_TAG::List)//多格物品列表
		{
			const auto &tmp = stContainerStats.pItems->GetList();
			for (const auto &it : tmp)
			{
				if (it.GetCompound().Empty())
				{
					continue;//空，处理下一个
				}
				listItemStack.push_back(ItemProcess::ItemCompoundToItemStack(it.GetCompound()));
			}
		}

		return listItemStack;
	}
};