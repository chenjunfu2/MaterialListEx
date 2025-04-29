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
		//��ȡ����ʵ���б�
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityItemStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//��ǰ����

		for (const auto &it : listTileEntity)
		{
			/*
				�кü��������item-Compound��Items-List����������-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityItemStats teStats{ &teCurrent.GetString(MU8STR("id")) };


			//����Ѱ��Items����ͨ���������
			if (const auto pItems = teCurrent.Search(MU8STR("Items"));
				pItems != NULL && pItems->IsList())
			{
				teStats.pItems = pItems;
				teStats.enType = NBT_Node::TAG_List;
				//���벢����
				vtTileEntityStats.emplace_back(std::move(teStats));
				continue;
			}

			//���Դ������ⷽ��
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

			//���ҷ���ʵ��id�Ƿ���map��
			const auto findIt = mapTileTntityDataName.find(*teStats.psTileEntityName);
			if (findIt == mapTileTntityDataName.end())//���ڣ����ǿ��Դ����Ʒ������������
			{
				continue;//�����˷���ʵ��
			}
			
			//ͨ��ӳ�������Ҷ�Ӧ��Ʒ�洢λ��
			const auto pSearch = teCurrent.Search(findIt->second.sTagName);
			if (pSearch == NULL)//����ʧ��
			{
				continue;//�����˷���ʵ��
			}

			teStats.enType = findIt->second.enType;
			teStats.pItems = pSearch;

			//ִ�е�������˵���������
			vtTileEntityStats.emplace_back(std::move(teStats));
		}//for

		return vtTileEntityStats;
	}//func

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		NBT_Node::NBT_Compound pcpdItemTag{};//��Ʒ��ǩ
		uint64_t u64Counter = 0;//��Ʒ������
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
		if (stItemStats.enType == NBT_Node::TAG_Compound)//ֻ��1����Ʒ
		{





		}
		else if(stItemStats.enType == NBT_Node::TAG_List)//��Ʒ�б�
		{





		}
		//else {}

		return vtItemStackList;
	}


};