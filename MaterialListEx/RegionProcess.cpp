#include "RegionProcess.h"

#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include <algorithm>

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions)
{
	RegionStatsList listRegionStats;
	listRegionStats.reserve(cpRegions.size());//��ǰ����
	for (const auto &[RgName, RgVal] : cpRegions)//����ѡ��
	{
		RegionStats rgsData{ &RgName };
		auto &RgCompound = GetCompound(RgVal);

		//���鴦��
		{
			auto &current = rgsData.mslBlock;
			auto vtBlockStats = BlockProcess::GetBlockStats(RgCompound);//��ȡ����ͳ���б�
			for (const auto &itBlock : vtBlockStats)
			{
				//ÿ������ת������Ʒ����ͨ��map����ͳ��ͬ����Ʒ
				auto istItemList = BlockProcess::BlockStatsToItemStack(itBlock);
				for (const auto &itItem : istItemList)
				{
					current.mapItemCounter[itItem.sItemName] += itItem.u64Counter;//���key�����ڣ����Զ��������ұ�֤valueΪ0
				}
			}

			//��ǰ���ݼ��ٲ��뿪��
			current.vecSortItem.reserve(current.mapItemCounter.size());
			current.vecSortItem.assign(current.mapItemCounter.begin(), current.mapItemCounter.end());//��������Χ����
			//����Ʒ��������������
			std::sort(current.vecSortItem.begin(), current.vecSortItem.end(), current.SortCmp);
		}

		//����ʵ����������
		{
			auto &current = rgsData.mslTileEntityContainer;
			auto vtTEContainerStats = TileEntityProcess::GetTileEntityContainerStats(RgCompound);
			for (const auto &it : vtTEContainerStats)
			{
				auto ret = TileEntityProcess::TileEntityContainerStatsToItemStack(it);
				for (const auto &itItem : ret)
				{
					//current.mapItemCounter[itItem.sItemName] += (uint64_t)(uint8_t)itItem.byteItemCount;//��ת����unsigned��Ȼ���ٽ�����չ
				}

				//��ǰ���ݼ��ٲ��뿪��
				current.vecSortItem.reserve(current.mapItemCounter.size());
				current.vecSortItem.assign(current.mapItemCounter.begin(), current.mapItemCounter.end());//��������Χ����
				//����Ʒ��������������
				std::sort(current.vecSortItem.begin(), current.vecSortItem.end(), current.SortCmp);
			}
		}

		//ʵ�崦��


		//ʵ����������


		//TODO��ʵ����Ʒ������

		listRegionStats.emplace_back(std::move(rgsData));
	}

	return listRegionStats;
}