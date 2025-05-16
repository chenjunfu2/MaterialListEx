#include "RegionProcess.h"

#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.size());//��ǰ����
	for (const auto &[RgName, RgVal] : cpRegions)//����ѡ��
	{
		RegionStats rgsData{ RgName };
		auto &RgCompound = GetCompound(RgVal);

		//���鴦��
		{
			auto &current = rgsData.mslBlock;
			auto listBlockStats = BlockProcess::GetBlockStats(RgCompound);//��ȡ����ͳ���б�
			for (const auto &itBlock : listBlockStats)
			{
				//ÿ������ת������Ʒ����ͨ��map����ͳ��ͬ����Ʒ
				auto istItemList = BlockProcess::BlockStatsToItemStack(itBlock);
				for (const auto &itItem : istItemList)
				{
					Item tmp{ std::move(itItem.sItemName) };//ת������Ȩ
					current.mapItemCounter[std::move(tmp)] += itItem.u64Counter;//���key�����ڣ����Զ��������ұ�֤valueΪ0
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
				auto ret = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				for (auto &itItem : ret)
				{
					Item tmp{ std::move(itItem.sItemName), std::move(itItem.cpdItemTag) };//ת������Ȩ
					current.mapItemCounter[std::move(tmp)] += (uint64_t)(uint8_t)itItem.byteItemCount;//��ת����unsigned��Ȼ���ٽ�����չ
				}
			}

			//ִ������
			current.SortElement();
		}

		//ʵ�崦��


		//ʵ����������


		//TODO��ʵ����Ʒ������

		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}