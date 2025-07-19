#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "ItemProcess.hpp"

#include <unordered_map>

class TileEntityProcess
{
public:
	TileEntityProcess() = delete;
	~TileEntityProcess() = delete;

	struct TileEntityContainerStats
	{
		const NBT_Node::NBT_String *psTileEntityName{};
		const NBT_Node *pItems{};
	};

	using TileEntityContainerStatsList = std::vector<TileEntityContainerStats>;
	
private:
	//������ּ������������ӳ�䵽TileEntityContainerStats
	static bool MapTileEntityContainerStats(const NBT_Node::NBT_Compound &teCompound, TileEntityContainerStats &teStats)
	{
		/*
			�кü��������item-Compound��Items-List����������-Compound
		*/

		//����Ѱ��Items����ͨ���������
		if (const auto pItems = teCompound.Search(MU8STR("Items"));
			pItems != NULL && pItems->IsList())
		{
			teStats.pItems = pItems;
			return true;
		}

		//���Դ������ⷽ��
		//����ӳ������ķ���ʵ����������Ʒ������
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapContainerTagName =
		{
			{MU8STR("minecraft:jukebox"),MU8STR("RecordItem")},
			{MU8STR("minecraft:lectern"),MU8STR("Book")},
			{MU8STR("minecraft:brushable_block"),MU8STR("item")},
		};

		if (teStats.psTileEntityName == NULL)//�������Ϊ�գ�������
		{
			return false;
		}

		//���ҷ���ʵ��id�Ƿ���map��
		const auto findIt = mapContainerTagName.find(*teStats.psTileEntityName);
		if (findIt == mapContainerTagName.end())//���ڣ����ǿ��Դ����Ʒ������������
		{
			return false;//�����˷���ʵ��
		}

		//ͨ��ӳ�������Ҷ�Ӧ��Ʒ�洢λ��
		const auto pSearch = teCompound.Search(findIt->second);
		if (pSearch == NULL)//����ʧ��
		{
			return false;//�����˷���ʵ��
		}

		//����ṹ��
		teStats.pItems = pSearch;
		return true;
	}

public:
	static TileEntityContainerStatsList GetTileEntityContainerStats(const NBT_Node::NBT_Compound &RgCompound)
	{
		//��ȡ����ʵ���б�
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		TileEntityContainerStatsList listTileEntityStats{};
		listTileEntityStats.reserve(listTileEntity.size());//��ǰ����

		for (const auto &it : listTileEntity)
		{
			const auto &cur = GetCompound(it);

			//TODO:���cur.HasString(MU8STR("id"))û�з���ʵ��id��
			//��ͨ������->����ʵ��ӳ�����ң��Ҳ�����ΪNULL
			//���ҳɹ���ʧ�ܺ��ٴ���teStats
			//�ر�ģ�ֻ���ҿ��Դ����Ʒ�ķ���ʵ�����������ඪ��
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
		if (tag == NBT_Node::TAG_Compound)//ֻ��һ����Ʒ
		{
			if (stContainerStats.pItems->GetCompound().empty())
			{
				return listItemStack;//�գ�ֱ�ӷ���
			}
			listItemStack.push_back(ItemProcess::ItemCompoundToItemStack(stContainerStats.pItems->GetCompound()));
		}
		else if (tag == NBT_Node::TAG_List)//�����Ʒ�б�
		{
			const auto &tmp = stContainerStats.pItems->GetList();
			for (const auto &it : tmp)
			{
				if (it.GetCompound().empty())
				{
					continue;//�գ�������һ��
				}
				listItemStack.push_back(ItemProcess::ItemCompoundToItemStack(it.GetCompound()));
			}
		}

		return listItemStack;
	}
};