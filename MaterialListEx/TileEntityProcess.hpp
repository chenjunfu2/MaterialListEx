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

	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		NBT_Node::NBT_Compound cpdItemTag{};//��Ʒ��ǩ
		NBT_Node::NBT_Byte byteItemCount = 0;//��Ʒ������
	};

	using TileEntityContainerStatsList = std::vector<TileEntityContainerStats>;
	using ItemStackList = std::vector<ItemStack>;
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

			TileEntityContainerStats teStats{ &cur.GetString(MU8STR("id")) };
			if (MapTileEntityContainerStats(cur, teStats))
			{
				listTileEntityStats.emplace_back(std::move(teStats));
			}
		}

		return listTileEntityStats;
	}

	static ItemStackList TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats, size_t szStackDepth = 64)
	{
		ItemStackList ret;
		_TileEntityContainerStatsToItemStack(stContainerStats, ret, szStackDepth);
		return ret;
	}

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
			teStats.enType = NBT_Node::TAG_List;
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

		if (teStats.psTileEntityName == NULL)
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
		teStats.enType = pSearch->GetTag();
		return true;
	}

	static void AddItems(const NBT_Node::NBT_Compound &cpdItem, ItemStackList &vtItemStackList, size_t szStackDepth)
	{
		if (szStackDepth <= 0)//�ݹ�ջ��ȼ��
		{
			return;//������ֱ�ӷ���
		}

		//�Ȳ���
		vtItemStackList.emplace_back
		(
			cpdItem.GetString(MU8STR("id")),
			NBT_Node::NBT_Compound{},//�˴������գ���ֹ���Ƕ�������Ĵ�������
			cpdItem.GetByte("Count")
		);

		//���pcpdTagΪNULL����������ֱ�ӷ���
		const auto pcpdTag = cpdItem.Search(MU8STR("tag"));
		if (pcpdTag == NULL)
		{
			return;
		}

		//Ȼ���ȡβ������Ϊ��ǰ����
		auto &current = vtItemStackList.back();//��Ϊ�п��ܽ���д�룬��Ҫ����Ϊ��const
		const auto &cpdTag = pcpdTag->GetCompound();//�����ã�Ȼ��������������������򲻿��������򿽱�Tag�Ա㴦��������Ʒ����ҩˮ����ҩˮ����ħ���������nbt�仯����Ʒ��

		//�����Ʒ�Ƿ��з���ʵ��tag��C��*���齫����������ʵ���TileEntity����ô�����ֽ�BlockEntity����������ˣ�
		//��ǰ��д����״̬ת�����鷳��Ҫ�������ֲ�ͳһ��һ����ˮ���ܸ�����������д��������İ���ס�ˣ���������*���齫��
		const auto pBlockEntityTag = cpdTag.Search(MU8STR("BlockEntityTag"));
		if (pBlockEntityTag != NULL)//������ʵ��
		{
			const auto &cpdBlockEntityTag = pBlockEntityTag->GetCompound();

			//ӳ����Ҫ��Ʒid����ȡһ�£����ˣ�������������BlockEntityTag��û����Ʒid�ģ�
			const auto pId = cpdBlockEntityTag.Search(MU8STR("id"));
			TileEntityContainerStats teStats{ (pId != NULL) ? &pId->GetString() : &current.sItemName };//û��id�������ֱ��ʹ����Ʒ��

			//����BlockEntityTag��ת����������Ʒͳ����Ϣ
			if (MapTileEntityContainerStats(cpdBlockEntityTag, teStats))//����ӳ��
			{
				//�ɹ���ݹ�������
				_TileEntityContainerStatsToItemStack(teStats, vtItemStackList, szStackDepth);//ѭ���ݹ�
			}
		}
		//���Բ����ɴ�������BlockEntityTag�ڲ�Ƕ��Items�ģ���ֱ��Items�ģ��������⴦��û�����齫���ҷ��ˣ�
		else if (current.sItemName == MU8STR("minecraft:bundle"))//û��BlockEntityTag�����Դ������ɴ�
		{
			const auto pItems = cpdTag.Search(MU8STR("Items"));
			TileEntityContainerStats teStats{ NULL,pItems,NBT_Node::TAG_List };//�ݹ鴦����Ҫid���һᴦ��pItems����ΪNULL�����
			_TileEntityContainerStatsToItemStack(teStats, vtItemStackList, szStackDepth);//ѭ���ݹ�
		}
		else//������涼û���������ڷ�����������Tag�Ա��������������������������Tag����
		{
			current.cpdItemTag = cpdTag;//����
		}
	};

	static void _TileEntityContainerStatsToItemStack(const TileEntityContainerStats &stContainerStats, ItemStackList &vtItemStackList, size_t szStackDepth)
	{
		if (stContainerStats.pItems == NULL)
		{
			return;
		}

		//��ȡÿ����Ʒ�ļ��ϲ���������ݲ��뵽ItemStack

		if (stContainerStats.enType == NBT_Node::TAG_Compound)//ֻ��һ����Ʒ
		{
			AddItems(stContainerStats.pItems->GetCompound(), vtItemStackList, szStackDepth - 1);
		}
		else if (stContainerStats.enType == NBT_Node::TAG_List)//�����Ʒ�б�
		{
			const auto &tmp = stContainerStats.pItems->GetList();
			for (const auto &it : tmp)
			{
				AddItems(it.GetCompound(), vtItemStackList, szStackDepth - 1);
			}
		}
		//else {}
	}

};