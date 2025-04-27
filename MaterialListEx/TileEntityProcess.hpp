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
		//��ȡ����ʵ���б�
		const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
		std::vector<TileEntityStats> vtTileEntityStats{};
		vtTileEntityStats.reserve(listTileEntity.size());//��ǰ����

		for (const auto &it : listTileEntity)
		{
			/*
			�кü��������Item-Compound��Items-List�����ⶨ�Ƶ�����-Compound
			*/

			const auto &teCurrent = GetCompound(it);
			TileEntityStats teStats{ &teCurrent.GetString(MU8STR("id")) };

			//�ȳ��Դ������ⷽ��
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

			//��ӳ������Ѱ��
			const auto findIt = mapTileTntityDataName.find(*teStats.psTileEntityName);
			if (findIt != mapTileTntityDataName.end())
			{
				//ͨ��ӳ�������Ҷ�Ӧ��Ʒ�洢λ��
				const auto pSearch = teCurrent.Search(findIt->second.sTagName);
				if (pSearch == NULL)//����ʧ��
				{
					continue;//�����˷���ʵ��
				}
				
				//����������
				teStats.enType = findIt->second.enType;
				switch (teStats.enType)//ͨ�����ʹӱ����ڻ�ȡ
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
				default://δ֪����
					{
						continue;//�����˷���ʵ��
					}
					break;
				}
			}
			else if (//����Ѱ��Items����ͨ���������
				const auto pItems = teCurrent.Search(MU8STR("Items"));
				pItems != NULL && pItems->IsList())
			{
				teStats.plistItems = &pItems->GetList();
				teStats.enType = NBT_Node::TAG_List;
			}
			else if (//����Ѱ��Item�����ⵥ��������
				const auto pItem = teCurrent.Search(MU8STR("Item"));
				pItem != NULL && pItem->IsCompound())
			{
				teStats.pcpdItem = &pItem->GetCompound();
				teStats.enType = NBT_Node::TAG_Compound;
			}
			else
			{//��ȫ�޷��ҵ�
				continue;//�����˷���ʵ��
			}

			//ִ�е�������˵���������
			vtTileEntityStats.emplace_back(std::move(teStats));
		}//for

		return vtTileEntityStats;
	}//func



};