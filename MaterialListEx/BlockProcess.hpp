#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"

#include <stdint.h>
#include <vector>
#include <map>

class BlockProcess
{
private:



public:
	BlockProcess() = delete;
	~BlockProcess() = delete;

	struct BlockStatistics
	{
		NBT_Node::NBT_String sBlockName{};
		NBT_Node::NBT_Compound cpdProperties{};
		uint64_t u64Counter = 0;//���������
	};

	struct RegionsStatistics
	{
		NBT_Node::NBT_String sRegionName{};
		std::vector<BlockStatistics> bsList{};
	};

	static std::vector<RegionsStatistics> GetBlockStatistics(NBT_Node nRoot)//block list
	{
		//��ȡregions��Ҳ��������һ��ͶӰ�����ж������ѡ����
		const auto &Regions = nRoot.Compound().at(MU8STR("Regions")).Compound();
		//��������ͳ��vector
		std::vector<RegionsStatistics> vtRegionsStatistics;
		vtRegionsStatistics.reserve(Regions.size());//��ǰ����
		for (const auto &[RgName, RgVal] : Regions)//����ѡ��
		{
			const auto &RgCompound = RgVal.Compound();

			/*----------------�����С���㡢��ɫ���ȡ----------------*/
			//��ȡ����ƫ��
			const auto &Position = RgCompound.at(MU8STR("Position")).Compound();
			const BlockPos reginoPos =
			{
				.x = Position.at(MU8STR("x")).Int(),
				.y = Position.at(MU8STR("y")).Int(),
				.z = Position.at(MU8STR("z")).Int(),
			};
			//��ȡ�����С������Ϊ�����������ƫ�Ƽ���ʵ�ʴ�С��
			const auto &Size = RgCompound.at(MU8STR("Size")).Compound();
			const BlockPos regionSize =
			{
				.x = Size.at(MU8STR("x")).Int(),
				.y = Size.at(MU8STR("y")).Int(),
				.z = Size.at(MU8STR("z")).Int(),
			};
			//��������ʵ�ʴ�С
			const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
			const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
			const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
			const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

			//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

			//��ȡ��ɫ�壨�������ࣩ
			const auto &BlockStatePalette = RgCompound.at(MU8STR("BlockStatePalette")).List();
			const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//����λͼ��һ��Ԫ��ռ�õ�bit��С
			const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//��ȡ����λ������ȡbitmap�ڲ�����
			//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
			/*------------------------------------------------*/


			/*----------------����ʵ���ȡ----------------*/
			//��ȡ����ʵ���б�
			const auto &TileEntities = RgCompound.at(MU8STR("TileEntities")).List();









			/*------------------------------------------------*/

			//��������ͳ��vector
			std::vector<BlockStatistics> vtBlockStatistics;
			vtBlockStatistics.reserve(BlockStatePalette.size());//��ǰ����

			//����BlockStatePalette�����ɫ�壬�����д�����Ч�±�ķ���ͳ��vector
			for (const auto &it : BlockStatePalette)
			{
				const auto &itCompound = it.Compound();

				BlockStatistics bsTemp{};
				bsTemp.sBlockName = itCompound.at(MU8STR("Name")).String();
				const auto find = itCompound.find(MU8STR("Properties"));//��鷽���Ƿ��ж�������
				if (find != itCompound.end())
				{
					bsTemp.cpdProperties = (*find).second.Compound();

					/*
					blockName += '[';
					const auto &Properties = (*find).second.Compound();//ȡ��compound
					for (const auto &[ppState, ppVal] : Properties)//�������ж������Բ�ƴ�ӵ�blockName��
					{
						blockName += ppState;
						blockName += '=';
						blockName += ppVal.String();
						blockName += ',';
					}

					//������ǿ��б��滻���һ��,Ϊ]
					if (blockName.ends_with(','))
					{
						blockName.back() = ']';
					}
					else
					{
						blockName += ']';
					}
					*/
				}

				vtBlockStatistics.emplace_back(std::move(bsTemp));
			}

			//��ȡLong����״̬λͼ���飨������Ϊ�±���ʵ�ɫ�壩
			const auto &BlockStates = RgCompound.at(MU8STR("BlockStates")).Long_Array();
			const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
			if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
			{
				//printf("BlockStates Too Small!\n");
				vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));//ȫ0����
				continue;//���Ի�ȡ��һ��ѡ��
			}

			//�洢��ʽ��(y * sizeLayer) + z * sizeX + x = Long array index, sizeLayer = sizeX * sizeZ
			//��������λͼ�����õ�ɫ���б��Ӧ����ļ�����
			for (uint64_t startOffset = 0; startOffset < (RegionFullSize * (uint64_t)bitsPerBitMapElement); startOffset += (uint64_t)bitsPerBitMapElement)//startOffset�ӵڶ��ο�ʼ����
			{
				const uint64_t startArrIndex = (uint64_t)(startOffset >> 6);//div 64
				const uint64_t startBitOffset = (uint64_t)(startOffset & 0x3F);//mod 64
				const uint64_t endArrIndex = (uint64_t)((startOffset + (uint64_t)bitsPerBitMapElement - 1ULL) >> 6);//div 64
				uint64_t paletteIndex = 0;

				if (startArrIndex == endArrIndex)
				{
					paletteIndex = ((uint64_t)BlockStates[startArrIndex] >> startBitOffset) & bitMaskOfElement;
				}
				else
				{
					const uint64_t endBitOffset = sizeof(uint64_t) * 8 - startBitOffset;
					paletteIndex = ((uint64_t)BlockStates[startArrIndex] >> startBitOffset | (uint64_t)BlockStates[endArrIndex] << endBitOffset) & bitMaskOfElement;
				}

				++vtBlockStatistics[paletteIndex].u64Counter;
			}

			//������ɣ�����RegionsStatistics
			vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));
		}

		return vtRegionsStatistics;
	}

	//���˿ɻ�ȡ�ķ��飬���ɻ�ȡ�ķ���ֱ��ɾ������ש���������Ʒ����ת������
	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//��Ʒ��
		uint64_t u64Counter = 0;//��Ʒ������
	};

	static ItemStack BlockStatisticsToItemStack(const BlockStatistics &stBlocks)
	{
		//�ȴ����鵽��Ʒת��

		//��ͨ����ת��
		static const std::map<NBT_Node::NBT_String, uint64_t> mapFilter =//uint64_t->ID
		{
			/*0 -> minecraft:empty*/
			{MU8STR("minecraft:air"),0 },//����air��Ӧ��Ʒempty
			{MU8STR("minecraft:void_air"),0 },
			{MU8STR("minecraft:cave_air"),0 },

			{MU8STR("minecraft:piston_head"),0},
			{MU8STR("minecraft:moving_piston"),0},
			{MU8STR("minecraft:nether_portal"),0},
			{MU8STR("minecraft:end_portal"),0},
			{MU8STR("minecraft:end_gateway"),0},

			{MU8STR("minecraft:farmland"),1},//dirt

			{MU8STR("minecraft:lava"),2},//lava.level == 0 ? minecraft:lava_bucket : minecraft:empty
			{MU8STR("minecraft:water"),2},//water.level == 0 ? minecraft:water_bucket : minecraft:empty
		};


		//���ⷽ��ת��
		/*
		��\��\��ֲ��\��ͬ���໨��
		*/






		//��������ת��
		/*
		��ש\ѩ\���군\���ݲ�\����\ӣ��
		*/

		//���ⷽ������ת��
		/*
		�����������飬���������������
		*/




		return {};
	}


};