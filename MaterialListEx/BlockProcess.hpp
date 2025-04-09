#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"

#include <stdint.h>
#include <vector>
#include <map>
#include <regex>
#include <unordered_set>

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
		uint64_t u64Counter = 0;//方块计数器
	};

	struct RegionsStatistics
	{
		NBT_Node::NBT_String sRegionName{};
		std::vector<BlockStatistics> bsList{};
	};

	static std::vector<RegionsStatistics> GetBlockStatistics(const NBT_Node &nRoot)//block list
	{
		//获取regions，也就是区域，一个投影可能有多个区域（选区）
		const auto &Regions = nRoot.Compound(MU8STR("Regions"));
		//创建区域统计vector
		std::vector<RegionsStatistics> vtRegionsStatistics;
		vtRegionsStatistics.reserve(Regions.size());//提前分配
		for (const auto &[RgName, RgVal] : Regions)//遍历选区
		{

			/*----------------区域大小计算、调色板获取----------------*/
			//获取区域偏移
			const auto &Position = RgVal.Get(MU8STR("Position"));
			const BlockPos reginoPos =
			{
				.x = Position.Int(MU8STR("x")),
				.y = Position.Int(MU8STR("y")),
				.z = Position.Int(MU8STR("z")),
			};
			//获取区域大小（可能为负，结合区域偏移计算实际大小）
			const auto &Size = RgVal.Get(MU8STR("Size"));
			const BlockPos regionSize =
			{
				.x = Size.Int(MU8STR("x")),
				.y = Size.Int(MU8STR("y")),
				.z = Size.Int(MU8STR("z")),
			};
			//计算区域实际大小
			const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
			const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
			const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
			const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

			//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

			//获取调色板（方块种类）
			const auto &BlockStatePalette = RgVal.List(MU8STR("BlockStatePalette"));
			const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//计算位图中一个元素占用的bit大小
			const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//获取遮罩位，用于取bitmap内部内容
			//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
			/*------------------------------------------------*/

			/*----------------根据方块位图访问调色板，获取不同状态方块的计数----------------*/
			//创建方块统计vector
			std::vector<BlockStatistics> vtBlockStatistics;
			vtBlockStatistics.reserve(BlockStatePalette.size());//提前分配

			//遍历BlockStatePalette方块调色板，并从中创建等效下标的方块统计vector
			for (const auto &it : BlockStatePalette)
			{
				BlockStatistics bsTemp{};
				bsTemp.sBlockName =it.String(MU8STR("Name"));
				const auto find = it.HasCompound(MU8STR("Properties"));//检查方块是否有额外属性
				if (find != NULL)
				{
					bsTemp.cpdProperties = *find;

					/*
					blockName += '[';
					const auto &Properties = (*find).second.Compound();//取出compound
					for (const auto &[ppState, ppVal] : Properties)//遍历所有额外属性并拼接到blockName后方
					{
						blockName += ppState;
						blockName += '=';
						blockName += ppVal.String();
						blockName += ',';
					}

					//如果不是空列表，替换最后一个,为]
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

			//获取Long方块状态位图数组（用于作为下标访问调色板）
			const auto &BlockStates = RgVal.Long_Array(MU8STR("BlockStates"));
			const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
			if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
			{
				//printf("BlockStates Too Small!\n");
				vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));//全0计数
				continue;//尝试获取下一个选区
			}

			//存储格式：(y * sizeLayer) + z * sizeX + x = Long array index, sizeLayer = sizeX * sizeZ
			//遍历方块位图，设置调色板列表对应方块的计数器
			for (uint64_t startOffset = 0; startOffset < (RegionFullSize * (uint64_t)bitsPerBitMapElement); startOffset += (uint64_t)bitsPerBitMapElement)//startOffset从第二次开始递增
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
			/*------------------------------------------------*/

			//计数完成，插入RegionsStatistics
			vtRegionsStatistics.emplace_back(RgName,std::move(vtBlockStatistics));
		}

		return vtRegionsStatistics;
	}

	//过滤可获取的方块，不可获取的方块直接删除，半砖等特殊多物品方块转换数量
	struct ItemStack
	{
		NBT_Node::NBT_String sItemName{};//物品名
		uint64_t u64Counter = 0;//物品计数器
	};

	using ItemStackList = std::vector<ItemStack>;//(应对花盆、含水方块、炼药锅、蜡烛蛋糕等组合物品)

	static ItemStackList BlockStatisticsToItemStack(const BlockStatistics &stBlocks)
	{
		//处理方块到物品转换
		ItemStackList stItemsList;

		//	{MU8STR("minecraft:farmland"),1},//dirt
		//
		//	{MU8STR("minecraft:lava"),2},//lava.level == 0 ? minecraft:lava_bucket : minecraft:empty
		//	{MU8STR("minecraft:water"),2},//water.level == 0 ? minecraft:water_bucket : minecraft:empty

		
		/*
		无物品形式方块\多格方块处理（门、床、活塞）（注：多格植物额外处理）\
		墙上的方块\
		不同种类花盆\炼药锅（不同内容物不同id）\带蜡烛的蛋糕转换为蛋糕+蜡烛\
		绊线到线\气泡柱转换为水\耕地、土径转为泥土\
		水、岩浆等流体处理\含水方块处理（转换为水桶）\
		特殊植物（海带有植株跟上部，大型垂滴有颈部和叶部，紫菘花有植株和头部（特殊处理，植株物品形式也有plant后缀），
			发光浆果植株（洞穴藤蔓转换）有植株和根部，垂泪藤有植株和根部部的区别，缠怨藤也有植株和根部）
		同一格内多个物品数量的方块转换（半砖\雪\海龟蛋\海泡菜\蜡烛\樱花簇）\
		特殊植物（瓶子草荚果和植株，火把花种子和植株）小型垂滴叶、高草、大型撅、多格花、瓶子草植株（特殊处理，物品形式也有plant后缀）\
		同一格内多面生长方块转换（藤蔓，发光地衣，幽匿脉络）
		*/
		//注意，使用了短路求值原理，只要其中一个函数成功返回，则剩下全部跳过，并且bRetCvrt为true
		bool bRetCvrt =
			CvrtUnItemedBlocks(stBlocks, stItemsList) ||//无物品形式方块处理
			CvrtMultiBlock(stBlocks, stItemsList) || //多格方块处理
			CvrtWallVariantBlocks(stBlocks, stItemsList) ||//墙上方块处理
			CvrtFlowerPot(stBlocks, stItemsList) ||//花盆处理



			false;//此处false只是为了统一格式：让函数调用后统一加||不报错，不改变最终执行结果

		//特殊方块、复合方块，多格方块，含水方块转换，所有的转换只会转换特殊状态，如果本身就和物品名形式一致则跳过，所以函数最后如果所有转换都没生效则为正常物品
		//如果前面全部没进入处理，则为普通方块，直接返回原始值
		if (!bRetCvrt)
		{
			stItemsList.emplace_back(stBlocks.sBlockName, 1);
		}

		return stItemsList;
	}

private:

#define FIND(name)\
const std::string target = MU8STR(name);\
size_t szPos = stBlocks.sBlockName.find(target);\
if (szPos != std::string::npos)

	static inline bool CvrtUnItemedBlocks(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//直接匹配所有不可获取的方块并返回空，注意是mc中所有无物品形式的，而非生存不可获取的
		//注意不匹配门、床、活塞等多格方块的另一半，而由他们对应的函数自行处理
		std::unordered_set<NBT_Node::NBT_String> UnItemedBlocks =
		{
			//空气类
			MU8STR("minecraft:air"),
			MU8STR("minecraft:void_air"),
			MU8STR("minecraft:cave_air"),
			//传送门类
			MU8STR("minecraft:nether_portal"),
			MU8STR("minecraft:end_portal"),
			MU8STR("minecraft:end_gateway"),
			//火类
			MU8STR("minecraft:fire"),
			MU8STR("minecraft:minecraft:soul_fire"),
			//移动中的活塞
			MU8STR("minecraft:moving_piston"),
		};

		//如果count返回不是0则代表存在，直接返回true即可，stItems初始化即为全空全0
		return UnItemedBlocks.count(stBlocks.sBlockName) != 0;
	}

	static inline bool CvrtMultiBlock(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//门只有下半掉落，上半直接转为空
		{
			FIND("_door")
			{
				//读取方块state判断是门的哪一部分
				const auto &half = stBlocks.cpdProperties.at(MU8STR("half")).String();

				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
				}
				//else if (half == MU8STR("upper")){}
				//else{}

				return true;
			}
		}

		//床只有头掉落，脚直接转为空
		{
			FIND("_bed")
			{
				//读取方块state判断是床的哪一部分
				const auto &part = stBlocks.cpdProperties.at(MU8STR("part")).String();

				if (part == MU8STR("head"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
				}
				//else if (part == MU8STR("foot")){}
				//else{}

				return true;
			}
		}

		//活塞只有身子掉落，头直接转为空，注意移动中的活塞不可获取，已被其它例程排除
		{
			FIND("piston")
			{
				//判断是不是piston_head
				if (stBlocks.sBlockName != MU8STR("minecraft:piston_head"))
				{
					stItemsList.emplace_back(stBlocks.sBlockName, 1);
				}
				//else{}

				return true;
			}
		}

		return false;
	}

	//注意只处理墙上的形式，如果是普通形式根本不需要处理
	static inline bool CvrtWallVariantBlocks(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//理论上所有带墙上额外方块id的方块都会有wall限定词，且不以wall结尾
		//去掉wall转换为item形式，经过查看，不存在诸如wall_结尾的方块名，也不存在wall_wall_之类的连续情况
		//直接匹配wall_并删除，即可正确处理诸如wall_torch或者cyan_wall_banner为物品形式

		FIND("wall_")
		{
			auto TmpName = stBlocks.sBlockName;
			TmpName.replace(szPos, target.length(), "");//删去wall_
			stItemsList.emplace_back(std::move(TmpName), 1);
			return true;
		}

		return false;
	}

	//注意，只处理放过花的花盆，否则跳过（如果本身叫flower_pot那就根本不需要转换）
	static inline bool CvrtFlowerPot(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//放了花的花盆以potted_开头，后跟花名
		//否则叫flower_pot
		FIND("potted_")
		{
			//有的情况下转化为flower_pot+扩展花物品名的形式
			stItemsList.emplace_back(MU8STR("minecraft:flower_pot"), 1);
			auto TmpName = stBlocks.sBlockName;
			TmpName.replace(szPos, target.length(), "");//删去potted_
			stItemsList.emplace_back(std::move(TmpName), 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtCauldron(const BlockStatistics &stBlocks, ItemStackList &stItemsList)
	{
		//炼药锅：含水、岩浆、粉雪
		//其中含水如果不满则转换到水瓶
		FIND("cauldron")
		{
			if (stBlocks.sBlockName == MU8STR("minecraft:cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:water_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				

			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:lava_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				//扩展部分为岩浆桶
				stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), 1);
			}
			else if (stBlocks.sBlockName == MU8STR("minecraft:powder_snow_cauldron"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:cauldron"), 1);
				//扩展部分为粉雪桶
				stItemsList.emplace_back(MU8STR("minecraft:powder_snow_bucket"), 1);
			}
			//else{}

			return true;
		}

		return false;
	}


#undef FIND
	//class
};