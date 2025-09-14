#include "RegionProcess.h"

//�ϲ�����ѡ��
RegionStats MergeRegionStatsList(const RegionStatsList &listRegionStats)
{
	RegionStats allRegionStats{};
	for (const auto &it : listRegionStats)//��������ѡ���ϲ���allRegionStats��
	{
		allRegionStats.Merge(it);
	}

	//ȫ������
	allRegionStats.SortElement();
	
	return allRegionStats;
}

RegionStatsList RegionProcess(const NBT_Type::Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.Size());//��ǰ����
	for (const auto &[RgName, RgVal] : cpRegions)//����ѡ��
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//���鵽��Ʒ����
		{
			//auto &current = rgsData.mslBlock;
			auto &curBlockItem = rgsData.mslBlockItem;
			auto listBlockStats = BlockProcess::GetBlockStats(RgCompound);//��ȡ����ͳ���б�
			for (const auto &itBlock : listBlockStats)
			{
				//ת������
				//auto tmpBlock = BlockProcess::BlockStatsToBlockInfo(itBlock);
				//current.map[std::move(tmpBlock)] += itBlock.u64Counter;//�H������Ҫ�Ӽ������������Ǽ�һ����Ϊǰ�涼ͳ������

				//ÿ������ת������Ʒ����ͨ��map����ͳ��ͬ����Ʒ
				auto tmp = BlockProcess::BlockStatsToItemStack(itBlock);
				for (auto &itItem : tmp)
				{
					curBlockItem.map[{ std::move(itItem.sItemName) }] += itItem.u64Counter;//���key�����ڣ����Զ��������ұ�֤valueΪ0
				}
			}

			//ִ������
			//current.SortElement();
			curBlockItem.SortElement();
		}

		//����ʵ����������
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto &curInfoTEC = rgsData.mmslParentInfoTEC;

			auto listTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : listTEContainerStats)//����ÿ������ʵ��
			{
				//ת������ʵ��
				auto tmp = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				tmp = ItemProcess::ItemStackListUnpackContainer(std::move(tmp));//�����Ʒ�ڲ�������

				//��ȡ��������
				auto sParentName = it.psTileEntityName == NULL ? NBT_Type::String{} : *it.psTileEntityName;
				auto &mpInfoTEC = curInfoTEC[sParentName];

				//��������ת�������Ʒ���ϲ���ͬ
				for (auto &itItem : tmp)
				{
					mpInfoTEC.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					current.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
			}

			//ִ������
			current.SortElement();
			curInfoTEC.SortElement();
		}

		//ʵ�崦��
		{
			//��������Щ�����ڶ�ȡ��������Ҫ������
			//��һ���Ƚ��ʵ�屾���ڶ�������������������ٽ����Ʒ��
			//��Ϊ���齫����ͷ���ʵ��ֿ���ţ���ʵ���ʵ��tag����һ��ģ�
			//�ֿ����������鷳��ֻ���������齫������ܲ���Ķ��������ˣ�
			//ע��ʵ��ǳ����⣬�ܶ�ʱ���ȡ��ʽ���������ڴ���Ʒ������
			//û�취ֱ��ת������Ʒ�����Ե�����һ��ʵ���
			//ʵ��ת����Ʒ�����б�Ҫ�ģ����紬����ʲô������ʵ��nbt��������ʵ��������boat
			auto &current = rgsData.mslEntity;
			auto &curContainer = rgsData.mslEntityContainer;
			auto &curInventory = rgsData.mslEntityInventory;
			auto &curInfoEC = rgsData.mmslParentInfoEC;
			auto &curInfoEI = rgsData.mmslParentInfoEI;

			auto listEntityStats = EntityProcess::GetEntityStats(RgCompound);
			for (const auto &it : listEntityStats)
			{
				//ת��ʵ��
				auto tmpEntity = EntityProcess::EntityStatsToEntityInfo(it);
				current.map[std::move(tmpEntity)] += 1;//ÿ������+1���ɣ���Ϊÿ�δ����ʵ��ֻ��һ��

				//ת��ʵ����Ʒ��������
				auto tmpSlot = EntityProcess::EntityStatsToEntitySlot(it);

				//���ڲ�����Ʒ�������н��
				tmpSlot.listContainer = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listContainer));
				tmpSlot.listInventory = ItemProcess::ItemStackListUnpackContainer(std::move(tmpSlot.listInventory));

				//��ȡʵ�����Ʋ���ȡ����������Ʒ��Ϣӳ��
				auto sParentName = it.psEntityName == NULL ? NBT_Type::String{} : *it.psEntityName;
				auto &mpInfoEC = curInfoEC[sParentName];
				auto &mpInfoEI = curInfoEI[sParentName];

				//�������ϲ���ͬ��Ʒ
				for (auto &itItem : tmpSlot.listContainer)
				{
					mpInfoEC.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					curContainer.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
				for (auto &itItem : tmpSlot.listInventory)
				{
					mpInfoEI.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
					curInventory.map[{ itItem.sItemName, itItem.cpdItemTag }] += itItem.u64ItemCount;
				}
			}

			//ִ������
			current.SortElement();
			curContainer.SortElement();
			curInventory.SortElement();
			curInfoEC.SortElement();
			curInfoEI.SortElement();
		}

		//���漸��û�д������������飬���ڿ��Ʊ�����������������ɶ���

		//������ÿ��region�����region�б�
		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}