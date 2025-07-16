#pragma once

#include "NBT_Node.hpp"
#include "MUTF8_Tool.hpp"
#include "Windows_ANSI.hpp"
#include "Calc_Tool.hpp"
#include "ItemProcess.hpp"

#include <stdint.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>


//跟物品类似，同样起个别名
using BlockInfo = ItemInfo;

class BlockProcess
{
public:
	BlockProcess() = delete;
	~BlockProcess() = delete;

	//过滤可获取的方块，不可获取的方块直接删除，半砖等特殊多物品方块转换数量
	//(应对花盆、含水方块、炼药锅、蜡烛蛋糕等组合物品)
	struct BlockStats
	{
		const NBT_Node::NBT_String *psBlockName{};
		const NBT_Node::NBT_Compound *pcpdProperties{};
		uint64_t u64Counter = 0;//方块计数器
	};

	using BlockStatsList = std::vector<BlockStats>;

public:
	//使用指针指向nRoot内节点，请不要在使用nRoot作为参数调用此函数获取返回值的情况下，销毁nRoot
	static BlockStatsList GetBlockStats(const NBT_Node::NBT_Compound &RgCompound)//block list
	{
		/*----------------区域大小计算、调色板获取----------------*/
		//获取区域偏移
		const auto &Position = RgCompound.GetCompound(MU8STR("Position"));
		const BlockPos reginoPos =
		{
			.x = Position.GetInt(MU8STR("x")),
			.y = Position.GetInt(MU8STR("y")),
			.z = Position.GetInt(MU8STR("z")),
		};
		//获取区域大小（可能为负，结合区域偏移计算实际大小）
		const auto &Size = RgCompound.GetCompound(MU8STR("Size"));
		const BlockPos regionSize =
		{
			.x = Size.GetInt(MU8STR("x")),
			.y = Size.GetInt(MU8STR("y")),
			.z = Size.GetInt(MU8STR("z")),
		};
		//计算区域实际大小
		const BlockPos posEndRel = getRelativeEndPositionFromAreaSize(regionSize).add(reginoPos);
		const BlockPos posMin = getMinCorner(reginoPos, posEndRel);
		const BlockPos posMax = getMaxCorner(reginoPos, posEndRel);
		const BlockPos size = posMax.sub(posMin).add({ 1,1,1 });

		//printf("RegionSize: [%d, %d, %d]\n", size.x, size.y, size.z);

		//获取调色板（方块种类）
		const auto &BlockStatePalette = RgCompound.GetList(MU8STR("BlockStatePalette"));
		const uint32_t bitsPerBitMapElement = Max(2U, (uint32_t)sizeof(uint32_t) * 8 - numberOfLeadingZeros(BlockStatePalette.size() - 1));//计算位图中一个元素占用的bit大小
		const uint32_t bitMaskOfElement = (1 << bitsPerBitMapElement) - 1;//获取遮罩位，用于取bitmap内部内容
		//printf("BlockStatePaletteSize: [%zu]\nbitsPerBitMapElement: [%d]\n", BlockStatePalette.size(), bitsPerBitMapElement);
		/*------------------------------------------------*/

		/*----------------根据方块位图访问调色板，获取不同状态方块的计数----------------*/
		//创建方块统计vector
		BlockStatsList listBlockStats;
		listBlockStats.reserve(BlockStatePalette.size());//提前分配

		//遍历BlockStatePalette方块调色板，并从中创建等效下标的方块统计vector
		for (const auto &it : BlockStatePalette)
		{
			const auto &itBlockCompound = GetCompound(it);

			BlockStats bsTemp{};
			bsTemp.psBlockName = &itBlockCompound.GetString(MU8STR("Name"));
			const auto find = itBlockCompound.HasCompound(MU8STR("Properties"));//检查方块是否有额外属性
			if (find != NULL)
			{
				bsTemp.pcpdProperties = find;
			}
			else
			{
				bsTemp.pcpdProperties = NULL;
			}

			listBlockStats.emplace_back(std::move(bsTemp));
		}

		//获取Long方块状态位图数组（用于作为下标访问调色板）
		const auto &BlockStates = RgCompound.GetLongArray(MU8STR("BlockStates"));
		const uint64_t RegionFullSize = (uint64_t)size.x * (uint64_t)size.y * (uint64_t)size.z;
		if (BlockStates.size() * 64 / bitsPerBitMapElement < RegionFullSize)
		{
			//printf("BlockStates Too Small!\n");
			return listBlockStats;
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

			++listBlockStats[paletteIndex].u64Counter;
		}
		/*------------------------------------------------*/

		//计数完成返回
		return listBlockStats;
	}

	static ItemProcess::NoTagItemList BlockStatsToItemStack(const BlockStats &stBlocks)
	{
		//处理方块到物品转换
		ItemProcess::NoTagItemList stItemsList{};

		//检查方块名
		if (stBlocks.psBlockName == NULL)
		{
			return stItemsList;
		}
		
		/*
		无物品形式方块\两格方块处理（门、床、活塞）（注：多格植物额外处理）\
		墙上的方块\
		不同种类花盆（杜鹃id有例外要注意）\炼药锅（不同内容物不同id）\带蜡烛的蛋糕转换为蛋糕+蜡烛\
		绊线到线\气泡柱转换为水\耕地、土径转为泥土\
		水、岩浆等流体处理\半砖处理\
		同一格内多个物品数量的方块转换（雪\海龟蛋\海泡菜\蜡烛\樱花簇）\
		同一格内多面生长方块转换（藤蔓，发光地衣，幽匿脉络）\
		多格植株植物处理（海带有植株跟上部，大型垂滴有颈部和叶部，(紫菘花无需特殊处理，直接跳过走默认例程（紫菘植株也直接转化为物品（反正生存不可获取，但是创造能拿）)
			发光浆果植株（洞穴藤蔓转换）有植株和根部，垂泪藤有植株和根部部的区别，缠怨藤也有植株和根部），竹笋和竹子转换，
			注意海带极其特殊，不带含水标签且必含水，不过根据实际情况，可以直接忽略海带本身自带的水，想了想算了还是多加一个水桶吧，方便统计\
		两格植物处理，小型垂滴叶、高草、海草、大型撅、多格花、瓶子草植株\

		作物处理：马铃薯、胡萝卜、甜菜根、小麦、西瓜南瓜的种子，和他们的作物形式转换，西瓜、南瓜茎有结果的形态和普通形态
			额外：火把花作物转换到火把花种子，瓶子草作物（下半部分）转换到瓶子草荚果\

		普通方块与含水方块处理（含水则转换为水桶）\
		*/
		//注意，使用了短路求值原理，只要其中一个函数成功返回，则剩下全部跳过，并且bRetCvrt为true
		bool bRetCvrt =
			CvrtUnItemedBlocks(stBlocks, stItemsList) ||		//无物品形式方块处理
			CvrtDoublePartBlocks(stBlocks, stItemsList) ||		//两格方块处理
			CvrtWallVariantBlocks(stBlocks, stItemsList) ||		//墙上方块处理
			CvrtFlowerPot(stBlocks, stItemsList) ||				//花盆处理
			CvrtCauldron(stBlocks, stItemsList) ||				//炼药锅处理
			CvrtCandleCake(stBlocks, stItemsList) ||			//蜡烛蛋糕处理
			CvrtAliasBlocks(stBlocks, stItemsList) ||			//别名方块处理
			CvrtFluid(stBlocks, stItemsList) ||					//流体处理
			CvrtSlabBlocks(stBlocks, stItemsList) ||			//半砖处理
			CvrtClusterBlocks(stBlocks, stItemsList) ||			//复合方块处理
			CvrtPolyAttachBlocks(stBlocks, stItemsList) ||		//多面附着方块处理
			CvrtMultiPartPlant(stBlocks, stItemsList) ||		//多格植株处理
			CvrtDoublePartPlant(stBlocks, stItemsList) ||		//两格植株处理
			CvrtCropPlant(stBlocks, stItemsList) ||				//作物植株处理

			false;//此处false只是为了统一格式：让函数调用后统一加||不报错，不改变最终执行结果

		//特殊方块、复合方块，多格方块，含水方块转换，所有的转换只会转换特殊状态，如果本身就和物品名形式一致则跳过，所以函数最后如果所有转换都没生效则为正常物品
		//如果前面全部没进入处理，则为普通方块，直接返回原始值
		//注意存在一种物品，有标签，不在过滤表内，所以此处再次处理
		if (!bRetCvrt)
		{
			CvrtNormalBlock(stBlocks, stItemsList);
		}

		//有标签方块进行含水方块处理（任何方块都有可能是，全部过一遍）
		CvrtWaterLoggedBlock(stBlocks, stItemsList);

		return stItemsList;
	}

	//哈哈，这个跟entityinfo一样，也很简单捏
	static BlockInfo BlockStatsToBlockInfo(const BlockStats &stBlocks)
	{
		return BlockInfo
		{
			stBlocks.psBlockName == NULL ? NBT_Node::NBT_String{} : *stBlocks.psBlockName,
			stBlocks.pcpdProperties == NULL ? NBT_Node::NBT_Compound{} : *stBlocks.pcpdProperties
		};
	}
private:

	/*警告，所有使用pcpdProperties的地方都需要判断是否为NULL！*/

//所有使用下列宏的地方必须是常量字符串！
#define FIND(name)\
const static std::string target = MU8STR(name);\
size_t szPos = stBlocks.psBlockName->find(target);\
if (szPos != std::string::npos)

#define STARTSWITH(name)\
const static std::string target = MU8STR(name);\
if (stBlocks.psBlockName->starts_with(target))

#define ENDSWITH(name)\
const static std::string target = MU8STR(name);\
if (stBlocks.psBlockName->ends_with(target))

	static inline bool CvrtUnItemedBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//直接匹配所有不可获取的方块并返回空，注意是mc中所有无物品形式的，而非生存不可获取的
		//注意不匹配门、床、活塞等多格方块的另一半，而由他们对应的函数自行处理
		const static std::unordered_set<NBT_Node::NBT_String> setUnItemedBlocks =
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
			MU8STR("minecraft:soul_fire"),
			//移动中的活塞
			MU8STR("minecraft:moving_piston"),
		};

		//如果count返回不是0则代表存在，直接返回true即可，stItems初始化即为全空全0
		return setUnItemedBlocks.count(*stBlocks.psBlockName) != 0;
	}

	static inline bool CvrtDoublePartBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//门只有下半掉落，上半直接转为空
		{
			ENDSWITH("_door")
			{
				//读取方块state判断是门的哪一部分
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &half = stBlocks.pcpdProperties->GetString(MU8STR("half"));

				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
				}
				//else if (half == MU8STR("upper")){}
				//else{}

				return true;
			}
		}

		//床只有头掉落，脚直接转为空
		{
			ENDSWITH("_bed")
			{
				//读取方块state判断是床的哪一部分
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &part = stBlocks.pcpdProperties->GetString(MU8STR("part"));

				if (part == MU8STR("head"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
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
				if (*stBlocks.psBlockName != MU8STR("minecraft:piston_head"))
				{
					stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
				}
				//else{}

				return true;
			}
		}

		return false;
	}

	//注意只处理墙上的形式，如果是普通形式根本不需要处理
	static inline bool CvrtWallVariantBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//理论上所有带墙上额外方块id的方块都会有wall限定词，且不以wall结尾
		//去掉wall转换为item形式，经过查看，不存在诸如wall_结尾的方块名，也不存在wall_wall_之类的连续情况
		//直接匹配wall_并删除，即可正确处理诸如wall_torch或者cyan_wall_banner为物品形式

		FIND("wall_")
		{
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(szPos, target.length());//删去wall_
			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	//注意，只处理放过花的花盆，否则跳过（如果本身叫flower_pot那就根本不需要转换）
	static inline bool CvrtFlowerPot(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//放了花的花盆以potted_开头，后跟花名
		//否则叫flower_pot
		FIND("potted_")
		{
			//有的情况下转化为flower_pot+扩展花物品名的形式
			stItemsList.emplace_back(MU8STR("minecraft:flower_pot"), stBlocks.u64Counter * 1);
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(szPos, target.length());//删去potted_
			//检测末尾是否以_bush结尾
			ENDSWITH("_bush")
			{
				//杜鹃花额外处理
				//删去bush
				TmpName.erase(TmpName.length() - target.length());//删去末尾_bush
			}

			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtCauldron(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//炼药锅：含水、岩浆、粉雪
		//其中含水如果不满则转换到空瓶

		enum CauldronType
		{
			Cauldron = 0,
			Water_cauldron,
			Lava_cauldron,
			Powder_snow_cauldron,
		};

		const static std::unordered_map<NBT_Node::NBT_String, CauldronType> mapCauldron =
		{
			{MU8STR("minecraft:cauldron"),Cauldron},
			{MU8STR("minecraft:water_cauldron"),Water_cauldron},
			{MU8STR("minecraft:lava_cauldron"),Lava_cauldron},
			{MU8STR("minecraft:powder_snow_cauldron"),Powder_snow_cauldron},
		};

		auto it = mapCauldron.find(*stBlocks.psBlockName);
		if (it != mapCauldron.end())
		{
			switch (it->second)
			{
			case Cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
				}
				break;
			case Water_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//扩展部分为水桶
					stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
					//判断方块标签
					if (stBlocks.pcpdProperties == NULL)
					{
						return false;
					}
					const auto &level = stBlocks.pcpdProperties->GetString(MU8STR("level"));

					if (level == MU8STR("1"))
					{
						stItemsList.emplace_back(MU8STR("minecraft:glass_bottle"), stBlocks.u64Counter * 2);//2个空瓶移除水，剩余1
					}
					else if (level == MU8STR("2"))
					{
						stItemsList.emplace_back(MU8STR("minecraft:glass_bottle"), stBlocks.u64Counter * 1);//1个空瓶移除水，剩余2
					}
					//else if (level == MU8STR("3")){}//0个空瓶移除水，剩余3
					//else{}
				}
				break;
			case Lava_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//扩展部分为岩浆桶
					stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), stBlocks.u64Counter * 1);
				}
				break;
			case Powder_snow_cauldron:
				{
					stItemsList.emplace_back(MU8STR("minecraft:cauldron"), stBlocks.u64Counter * 1);
					//扩展部分为粉雪桶
					stItemsList.emplace_back(MU8STR("minecraft:powder_snow_bucket"), stBlocks.u64Counter * 1);
				}
				break;
			default:
				return false;
				break;
			}

			return true;
		}

		return false;
	}

	static inline bool CvrtCandleCake(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		ENDSWITH("_cake")
		{
			//转换为蜡烛+蛋糕
			stItemsList.emplace_back(MU8STR("minecraft:cake"), stBlocks.u64Counter * 1);
			auto TmpName = *stBlocks.psBlockName;
			TmpName.erase(TmpName.length() - target.length());//删去_cake
			stItemsList.emplace_back(std::move(TmpName), stBlocks.u64Counter * 1);

			return true;
		}

		return false;
	}

	static inline bool CvrtAliasBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//别名方块
	{
		//映射
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapAliasBlocks =
		{
			{MU8STR("minecraft:tripwire"),MU8STR("minecraft:string")},
			{MU8STR("minecraft:redstone_wire"),MU8STR("minecraft:redstone")},
			{MU8STR("minecraft:bubble_column"),MU8STR("minecraft:water_bucket")},
			//{MU8STR("minecraft:farmland"),MU8STR("minecraft:dirt")},
			//{MU8STR("minecraft:dirt_path"),MU8STR("minecraft:dirt")},
		};

		//查找
		const auto it = mapAliasBlocks.find(*stBlocks.psBlockName);
		if (it != mapAliasBlocks.end())//存在
		{
			stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);//插入映射
			return true;
		}

		return false;
	}

	static inline bool CvrtFluid(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//流体
	{
		if (*stBlocks.psBlockName == MU8STR("minecraft:water"))
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("level")) == MU8STR("0"))//是0就是水源
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}

			return true;
		}
		else if (*stBlocks.psBlockName == MU8STR("minecraft:lava"))
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("level")) == MU8STR("0"))//是0就是岩浆源
			{
				stItemsList.emplace_back(MU8STR("minecraft:lava_bucket"), stBlocks.u64Counter * 1);
			}

			return true;
		}
		//else{}

		return false;
	}

	static inline bool CvrtSlabBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		ENDSWITH("_slab")
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			if (stBlocks.pcpdProperties->GetString(MU8STR("type")) == MU8STR("double"))//转换为2，否则为1
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 2);
			}
			else
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
			}

			return true;
		}

		return false;
	}

	static inline bool CvrtClusterBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
#define EMPLACE_CLUSTER_ITEMS(name)\
const auto &##name = stBlocks.pcpdProperties->GetString(MU8STR(#name));\
stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * std::stoll(##name));

		//先判断存不存在附加状态，下面所有代码路径都会使用，所以在开头排除
		if (stBlocks.pcpdProperties == NULL)
		{
			return false;
		}

		//蜡烛优先处理（多颜色方块）
		ENDSWITH("candle")//蜡烛极其特殊，存在无颜色标签的版本（与床之类不同）
		{
			//因为前面已经处理过带蜡烛的蛋糕，排除是带蜡烛蛋糕的情况，且candle是结尾，不存在蛋糕的情况
			EMPLACE_CLUSTER_ITEMS(candles);//转换到方块个数并插入
			return true;
		}


		enum ClusterBlock :uint64_t
		{
			Snow = 0, 
			Turtle_egg,
			Sea_pickle,
			Pink_petals,
		};

		const static std::unordered_map<NBT_Node::NBT_String, ClusterBlock> mapClusterBlocks =
		{
			{MU8STR("minecraft:snow"),Snow},
			{MU8STR("minecraft:turtle_egg"),Turtle_egg},
			{MU8STR("minecraft:sea_pickle"),Sea_pickle},
			{MU8STR("minecraft:pink_petals"),Pink_petals},
		};

		const auto it = mapClusterBlocks.find(*stBlocks.psBlockName);
		if (it != mapClusterBlocks.end())
		{

			switch (it->second)
			{
			case Snow://layers str
				{
					EMPLACE_CLUSTER_ITEMS(layers);
				}
				break;
			case Turtle_egg://eggs str
				{
					EMPLACE_CLUSTER_ITEMS(eggs);
				}
				break;
			case Sea_pickle://pickles str
				{
					EMPLACE_CLUSTER_ITEMS(pickles);
				}
				break;
			case Pink_petals://flower_amount str
				{
					EMPLACE_CLUSTER_ITEMS(flower_amount);
				}
				break;
			default:
				return false;
				break;
			}

			return true;
		}

		return false;
#undef EMPLACE_CLUSTER_ITEMS
	}

	static inline bool CvrtPolyAttachBlocks(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//多面附着方块
	{
		const static std::unordered_set<NBT_Node::NBT_String> setPolyAttachBlocks =
		{
			MU8STR("minecraft:vine"),
			MU8STR("minecraft:glow_lichen"),
			MU8STR("minecraft:sculk_vein"),
		};

		const constexpr static char *pSurfaceNameList[] =
		{
			"down",
			"east",
			"north",
			"south",
			"up",
			"west",
		};


		if (setPolyAttachBlocks.count(*stBlocks.psBlockName) != 0)
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}

			//遍历面列表，查询每个面信息，如果存在，判断是不是true，如果是，计数+1
			uint64_t u64Counter = 0;
			for (const auto &it : pSurfaceNameList)
			{
				const auto pSurface = stBlocks.pcpdProperties->HasString(MU8STR(it));
				if (pSurface != NULL)
				{
					if (*pSurface == MU8STR("true"))
					{
						++u64Counter;
					}
				}
			}

			//根据计数插入对应数量的多面附着块物品数
			stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * u64Counter);

			return true;
		}

		return false;
	}


	//所有此类转换函数都不处理普通单格植物
	static inline bool CvrtMultiPartPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//多格植株处理
	{
		//只转换植株其余非植株走普通方块normal处理
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapMultiPartPlant =
		{
			//洞穴藤蔓特殊处理，转换到发光浆果
			{MU8STR("minecraft:cave_vines_plant"),MU8STR("minecraft:glow_berries")},
			{MU8STR("minecraft:cave_vines"),MU8STR("minecraft:glow_berries")},
			//竹子特殊转换
			{MU8STR("minecraft:bamboo_sapling"),MU8STR("minecraft:bamboo")},
			//垂滴叶特殊转换
			{MU8STR(" minecraft:big_dripleaf_stem"),MU8STR("minecraft:big_dripleaf")},
			//海带
			{MU8STR("minecraft:kelp"),MU8STR("minecraft:kelp")},//添加海带到自身的映射的目的是为了处理含水情况
			{MU8STR("minecraft:kelp_plant"),MU8STR("minecraft:kelp")},
			//海草
			{MU8STR("minecraft:tall_seagrass"),MU8STR("minecraft:seagrass")},//高海草的每个部分都映射到一个海草
			//下界藤蔓
			{MU8STR("minecraft:weeping_vines_plant"),MU8STR("minecraft:weeping_vines")},
			{MU8STR("minecraft:twisting_vines_plant"),MU8STR("minecraft:twisting_vines")},
		};

		const auto it = mapMultiPartPlant.find(*stBlocks.psBlockName);
		if (it != mapMultiPartPlant.end())
		{
			stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);//插入映射
			//海带海草必含水，强制处理
			if (it->second == MU8STR("minecraft:kelp")||
				it->second == MU8STR("minecraft:seagrass"))
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}
			return true;
		}
		
		return false;
	}

	//所有此类转换函数都不处理普通单格植物
	static inline bool CvrtDoublePartPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//双格植株处理
	{
		//只有根部破坏掉落，头部直接忽略
		const static std::unordered_set<NBT_Node::NBT_String> setDoublePartPlant =
		{
			MU8STR("minecraft:small_dripleaf"),
			MU8STR("minecraft:tall_grass"),
			MU8STR("minecraft:large_fern"),
			MU8STR("minecraft:sunflower"),
			MU8STR("minecraft:lilac"),
			MU8STR("minecraft:rose_bush"),
			MU8STR("minecraft:peony"),
			MU8STR("minecraft:pitcher_plant"),
		};

		//判断是否存在于set中，是则处理上下部分
		if (setDoublePartPlant.count(*stBlocks.psBlockName) != 0)
		{
			if (stBlocks.pcpdProperties == NULL)
			{
				return false;
			}
			const auto &half = stBlocks.pcpdProperties->GetString("half");
			if (half == MU8STR("lower"))
			{
				stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
			}
			//else if (half == MU8STR("upper")) {}
			//else {}

			return true;
		}

		return false;
	}
	
	//所有此类转换函数都不处理普通单格植物
	static inline bool CvrtCropPlant(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)//作物植株处理
	{
		const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String> mapCropPlant =
		{
			{MU8STR("minecraft:pumpkin_stem"),MU8STR("minecraft:pumpkin_seeds")},
			{MU8STR("minecraft:attached_pumpkin_stem"),MU8STR("minecraft:pumpkin_seeds")},
			{MU8STR("minecraft:melon_stem"),MU8STR("minecraft:melon_seeds")},
			{MU8STR("minecraft:attached_melon_stem"),MU8STR("minecraft:melon_seeds")},
			{MU8STR("minecraft:beetroots"),MU8STR("minecraft:beetroot_seeds")},
			{MU8STR("minecraft:wheat"),MU8STR("minecraft:wheat_seeds")},
			{MU8STR("minecraft:carrots"),MU8STR("minecraft:carrot")},
			{MU8STR("minecraft:potatoes"),MU8STR("minecraft:potato")},
			{MU8STR("minecraft:torchflower_crop"),MU8STR("minecraft:torchflower_seeds")},
			{MU8STR("minecraft:pitcher_crop"),MU8STR("minecraft:pitcher_pod")},//需要特判处理
		};

		const auto it = mapCropPlant.find(*stBlocks.psBlockName);
		if (it != mapCropPlant.end())
		{
			if (it->second == MU8STR("minecraft:pitcher_pod"))//需要判断原方块的half
			{
				if (stBlocks.pcpdProperties == NULL)
				{
					return false;
				}
				const auto &half = stBlocks.pcpdProperties->GetString("half");
				if (half == MU8STR("lower"))
				{
					stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);
				}
				//else if (half == MU8STR("upper")) {}
				//else {}
			}
			else
			{
				//直接插入转换结果
				stItemsList.emplace_back(it->second, stBlocks.u64Counter * 1);
			}

			return true;
		}

		return false;
	}

	//注意该函数需要保证不进行任何block的compound判断
	static inline void CvrtNormalBlock(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		stItemsList.emplace_back(*stBlocks.psBlockName, stBlocks.u64Counter * 1);
	}

	static inline void CvrtWaterLoggedBlock(const BlockStats &stBlocks, ItemProcess::NoTagItemList &stItemsList)
	{
		//判断是不是含水方块，如果是，加一个水桶
		if (stBlocks.pcpdProperties == NULL)
		{
			return;//如果没有附加标签则无需判断
		}
		const auto it = stBlocks.pcpdProperties->HasString("waterlogged");
		if (it != NULL)
		{
			if (*it == MU8STR("true"))//含水
			{
				stItemsList.emplace_back(MU8STR("minecraft:water_bucket"), stBlocks.u64Counter * 1);
			}
			//else {}
		}
	}

#undef ENDSWITH
#undef STARTSWITH
#undef FIND
	//class
};