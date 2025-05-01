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
		//��ȡ����ʵ���б�
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityContainerStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//��ǰ����

		for (const auto &it : listTileEntity)
		{
			/*
				�кü��������item-Compound��Items-List����������-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityContainerStats teStats{ &teCurrent.GetString(MU8STR("id")) };


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

			//����ӳ������ķ���ʵ����������Ʒ������
			const static std::unordered_map<NBT_Node::NBT_String, DataInfo> mapContainerItemName =
			{
				{MU8STR("minecraft:jukebox"),{MU8STR("RecordItem"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:lectern"),{MU8STR("Book"),NBT_Node::TAG_Compound}},
				{MU8STR("minecraft:brushable_block"),{MU8STR("item"),NBT_Node::TAG_Compound}},
			};

			//���ҷ���ʵ��id�Ƿ���map��
			const auto findIt = mapContainerItemName.find(*teStats.psTileEntityName);
			if (findIt == mapContainerItemName.end())//���ڣ����ǿ��Դ����Ʒ������������
			{
				continue;//�����˷���ʵ��
			}
			
			//ͨ��ӳ�������Ҷ�Ӧ��Ʒ�洢λ��
			const auto pSearch = teCurrent.Search(findIt->second.sTagName);
			if (pSearch == NULL)//����ʧ��
			{
				continue;//�����˷���ʵ��
			}

			//����ṹ��
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
		NBT_Node cpdItemTag{};//��Ʒ��ǩ
		NBT_Node::NBT_Byte byteItemCount = 0;//��Ʒ������
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

		//��ȡÿ����Ʒ�ļ��ϲ���������ݲ��뵽ItemStack
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
		
		if (stContainerStats.enType == NBT_Node::TAG_Compound)//ֻ��һ����Ʒ
		{
			EmplaceBackItem(stContainerStats.pItems->GetCompound());
		}
		else if(stContainerStats.enType == NBT_Node::TAG_List)//�����Ʒ�б�
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