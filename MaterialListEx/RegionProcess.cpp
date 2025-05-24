#include "RegionProcess.h"

#include "BlockProcess.hpp"
#include "TileEntityProcess.hpp"
#include "EntityProcess.hpp"

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