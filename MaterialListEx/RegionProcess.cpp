#include "RegionProcess.h"

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.size());//��ǰ����
	for (const auto &[RgName, RgVal] : cpRegions)//����ѡ��
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//���鵽��Ʒ����
		{
			auto &current = rgsData.mslBlockItem;
			auto listBlockStats = BlockProcess::GetBlockStats(RgCompound);//��ȡ����ͳ���б�
			for (const auto &itBlock : listBlockStats)
			{
				//ÿ������ת������Ʒ����ͨ��map����ͳ��ͬ����Ʒ
				auto tmp = BlockProcess::BlockStatsToItemStack(itBlock);
				for (auto &itItem : tmp)
				{
					current.map[{ std::move(itItem.sItemName) }] += itItem.u64Counter;//���key�����ڣ����Զ��������ұ�֤valueΪ0
				}
			}

			//ִ������
			current.SortElement();
		}

		//����ʵ����������
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto listTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : listTEContainerStats)
			{
				//ת��ÿ������ʵ��
				auto tmp = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				tmp = ItemProcess::ItemStackListUnpackContainer(std::move(tmp));//����ڲ�������
				for (auto &itItem : tmp)
				{
					current.map[{ std::move(itItem.sItemName), std::move(itItem.cpdItemTag) }] += itItem.u64ItemCount;
				}
			}

			//ִ������
			current.SortElement();
		}

		//ʵ�崦��
		{
			//��������Щ�����ڶ�ȡ��������Ҫ������
			//��һ���Ƚ��ʵ�屾���ڶ�������������������ٽ����Ʒ��
			//��Ϊ���齫����ͷ���ʵ��ֿ���ţ���ʵ���ʵ��tag����һ��ģ�
			//�ֿ����������鷳��ֻ���������齫������ܲ���Ķ��������ˣ�
			//ע��ʵ��ǳ����⣬�ܶ�ʱ���ȡ��ʽ���������ڴ���Ʒ������
			//û�취ֱ��ת������Ʒ�����Ե�����һ��ʵ���
			auto &current = rgsData.mslEntity;
			auto &curContainer = rgsData.mslEntityContainer;
			auto &curInventory = rgsData.mslEntityInventory;

			auto listEntityStats = EntityProcess::GetEntityStats(RgCompound);
			for (const auto &it : listEntityStats)
			{
				//ת��ʵ��
				auto tmpEntity = EntityProcess::EntityStatsToEntity(it);
				current.map[std::move(tmpEntity)] += 1;//ÿ������+1����

				//ת��ʵ����Ʒ��������
				auto tmpSlot = EntityProcess::EntityStatsToEntitySlot(it);

				//���ڲ�����Ʒ�������н��
				tmpSlot.listContainer = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listContainer));
				tmpSlot.listInventory = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listInventory));

				//�������ϲ���ͬ��Ʒ
				for (auto &itItem : tmpSlot.listContainer)
				{
					curContainer.map[{std::move(itItem.sItemName), std::move(itItem.cpdItemTag)}] += itItem.u64ItemCount;
				}
				for (auto &itItem : tmpSlot.listInventory)
				{
					curInventory.map[{std::move(itItem.sItemName), std::move(itItem.cpdItemTag)}] += itItem.u64ItemCount;
				}
			}

			//ִ������
			current.SortElement();
			curContainer.SortElement();
			curInventory.SortElement();
		}

		//���漸��û�д������������飬���ڿ��Ʊ�����������������ɶ���

		//������ÿ��region�����region�б�
		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}