#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;


	struct TileEntityStats
	{
		const NBT_Node::NBT_String *psTileEntityName{};
		union
		{
			const NBT_Node::NBT_List *plistItems{};
			const NBT_Node::NBT_Compound *pcpdItem;
		};
		NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
	};

	static std::vector<TileEntityStats> GetTileEntityStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//获取方块实体列表
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//提前扩容

		for (const auto &it : listTileEntity)
		{
			/*
			有好几种情况：Item-Compound、Items-List、特殊定制的名称-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityStats teStats{ &teCurrent.GetString(MU8STR("id")) };

			//先尝试处理特殊方块
			struct DataInfo
			{
				NBT_Node::NBT_String sTagName{};
				NBT_Node::NBT_TAG enType{ NBT_Node::TAG_End };
			};

			const static std::unordered_map<NBT_Node::NBT_String, DataInfo> mapTileTntityDataName =
			{
				{MU8STR("minecraft:jukebox"),{MU8STR("RecordItem"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:lectern"),{MU8STR("Book"),NBT_Node::TAG_Compound}},
			};

			//从映射名中寻找
			const auto findIt = mapTileTntityDataName.find(*teStats.psTileEntityName);
			if (findIt != mapTileTntityDataName.end())
			{
				//通过映射名查找对应物品存储位置
				const auto pSearch = teCurrent.Search(findIt->second.sTagName);
				if (pSearch == NULL)//查找失败
				{
					continue;//跳过此方块实体
				}
				
				//先设置类型
				teStats.enType = findIt->second.enType;
				switch (teStats.enType)//通过类型从变体内获取
				{
				case NBT_Node::TAG_Compound:
					{
						teStats.pcpdItem = &pSearch->GetCompound();
					}
					break;
				case NBT_Node::TAG_List:
					{
						teStats.plistItems = &pSearch->GetList();
					}
					break;
				default://未知类型
					{
						continue;//跳过此方块实体
					}
					break;
				}
			}
			else if (//尝试寻找Items（普通多格容器）
				const auto pItems = teCurrent.Search(MU8STR("Items"));
				pItems != NULL && pItems->IsList())
			{
				teStats.plistItems = &pItems->GetList();
				teStats.enType = NBT_Node::TAG_List;
			}
			else if (//尝试寻找Item（特殊单格容器）
				const auto pItem = teCurrent.Search(MU8STR("Item"));
				pItem != NULL && pItem->IsCompound())
			{
				teStats.pcpdItem = &pItem->GetCompound();
				teStats.enType = NBT_Node::TAG_Compound;
			}
			else
			{//完全无法找到
				continue;//跳过此方块实体
			}

			//执行到这里这说明操作完成
			vtTileEntityStats.emplace_back(std::move(teStats));
		}//for

		return vtTileEntityStats;
	}//func



};