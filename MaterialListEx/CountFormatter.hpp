#include <exception>
#include <type_traits>
#include <string>
#include <stdint.h>
#include <stddef.h>//size_t
#include <unordered_map>
#include <vector>

#include <NBT_Node.hpp>

#include "FileSystemHelper.hpp"

class CountFormatter
{
public:
	struct ItemLevel
	{
	public:
		union
		{
			struct
			{
				uint64_t u64LargeChestShulkerBox;
				uint64_t u64ChestShulkerBox;
				uint64_t u64ShulkerBox;
				uint64_t u64SetItem;
				uint64_t u64Item;
			};
			uint64_t u64Data[5] = { 0 };
		};
		static inline constexpr const size_t szDatacount = sizeof(u64Data) / sizeof(u64Data[0]);

		static inline constexpr const char *const cItemLevel[szDatacount] =
		{
			"大箱盒","箱盒","盒","组","个",
		};

	public:
		std::string ToString(void)
		{
			std::string strRet;
			strRet.reserve(36);//预分配

			//挨个输出非0项
			bool bOut = false;
			for (int i = 0; i < szDatacount; ++i)
			{
				if (u64Data[i] == 0)
				{
					continue;
				}

				if (bOut)
				{
					strRet += " + ";
				}
				strRet += std::to_string(u64Data[i]);//转换数值到string
				strRet += cItemLevel[i];//加上单位
				bOut = true;
			}

			if (!bOut)//没一个非0（没有输出过）
			{
				strRet += '0';//输出0个
				strRet += cItemLevel[szDatacount - 1];//单位“个”
			}

			return strRet;
		}
	};

private:
	struct ContainerSlotCount
	{
	public:
		size_t szChestSlotCount;
		size_t szShulkerBoxSlotCount;
		size_t szLargeChestSlotCount;

	public:
		void SetDefault(void)
		{
			szChestSlotCount = 27;
			szShulkerBoxSlotCount = szChestSlotCount;
			szLargeChestSlotCount = szChestSlotCount * 2;
		}
	};

	struct ItemCount
	{
	public:
		size_t szSetItemCount;
		size_t szShulkerBoxItemCount;
		size_t szChestShulkerBoxItemCount;
		size_t szLargeChestShulkerBoxItemCount;

	public:
		void SetItemStackCount(size_t szItemStackCount, const ContainerSlotCount & csc)
		{
			szSetItemCount = szItemStackCount;
			szShulkerBoxItemCount = csc.szShulkerBoxSlotCount * szItemStackCount;
			szChestShulkerBoxItemCount = csc.szChestSlotCount * csc.szShulkerBoxSlotCount * szItemStackCount;
			szLargeChestShulkerBoxItemCount = csc.szLargeChestSlotCount * csc.szShulkerBoxSlotCount * szItemStackCount;
		}

		static size_t DefaultItemStackCount(void)
		{
			return 64;
		}
	};

private:
	ContainerSlotCount cscDefault;
	ItemCount icDefault;
	std::unordered_map<std::string, size_t> mapItemCount;
	std::vector<ItemCount> listItemCount;

private:
	const ItemCount &GetItemCount(const std::string &strItemName)
	{
		if (auto it = mapItemCount.find(strItemName); it != mapItemCount.end())
		{
			return listItemCount[it->second];
		}
		else
		{
			return icDefault;
		}
	}


public:
	CountFormatter():
		cscDefault({}),
		icDefault({}),
		mapItemCount({}),
		listItemCount({})
	{
		SetDefault();
	}

	~CountFormatter(void) = default;

	bool ReadCountFormatterFile(const std::string &strFileName)
	{
		Json json;
		if (!ReadJsonFile(strFileName, json))
		{
			return false;
		}

#define TRY_READ_FIELD(json_obj, expected_type, key, field)\
do\
{\
	auto _it = json_obj.find(key);\
	if (_it == json_obj.end())\
	{\
		break;\
	}\
	if (_it->type() != (expected_type))\
	{\
		break;\
	}\
	(field) = _it->get<std::remove_reference_t<decltype(field)>>();\
} while (false);

#define THROW_IF_ZERO(name, field)\
do\
{\
	if ((field) == 0)\
	{\
		throw std::runtime_error(name "is 0");\
	}\
} while (false);

		cscDefault.SetDefault();
		try
		{
			if (!json.is_object())
			{
				throw std::runtime_error("Json root not object");
			}

			//读取成员信息
			TRY_READ_FIELD(json, Json::value_t::number_unsigned, "ChestSlotCount", cscDefault.szChestSlotCount);
			THROW_IF_ZERO("ChestSlotCount", cscDefault.szChestSlotCount);
			
			TRY_READ_FIELD(json, Json::value_t::number_unsigned, "ShulkerBoxSlotCount", cscDefault.szShulkerBoxSlotCount);
			THROW_IF_ZERO("ShulkerBoxSlotCount", cscDefault.szShulkerBoxSlotCount);
			
			TRY_READ_FIELD(json, Json::value_t::number_unsigned, "LargeChestSlotCount", cscDefault.szLargeChestSlotCount);
			THROW_IF_ZERO("LargeChestSlotCount", cscDefault.szLargeChestSlotCount);
			
			size_t szDefaultItemStackCount = icDefault.DefaultItemStackCount();
			TRY_READ_FIELD(json, Json::value_t::number_unsigned, "DefaultItemStackCount", szDefaultItemStackCount);
			THROW_IF_ZERO("DefaultItemStackCount", szDefaultItemStackCount);

			icDefault.SetItemStackCount(szDefaultItemStackCount, cscDefault);


			//读取列表信息
			auto itStack = json.find("Stack");
			if (itStack == json.end())
			{
				return true;
			}

			if (!itStack->is_array())
			{
				throw std::runtime_error("\"Stack\" not array");
			}

			std::unordered_map<size_t, size_t> mapCountPos;
			for (auto &obj : *itStack)
			{
				if (!obj.is_object())
				{
					throw std::runtime_error("\"Stack\" not object array");
				}

				auto itCount = obj.find("Count");
				if (itCount == obj.end() && !itCount->is_number_unsigned())
				{
					throw std::runtime_error("object don't have unsigned number \"Count\"");
				}
				size_t szCount = itCount->get<size_t>();

				//筛去重复值
				size_t szCurPos = listItemCount.size();
				auto emp_ret = mapCountPos.try_emplace(szCount, szCurPos);
				if (emp_ret.second)//插入成功，新值更新。否则失败跳过，值不会被覆盖
				{
					listItemCount.push_back({ szCount });//构造新大小
				}
				else//获取阻止插入的值的坐标
				{
					szCurPos = emp_ret.first->second;//设置为已有的值的坐标
				}

				//查找物品列表
				auto itItems = obj.find("Items");
				if (itItems == obj.end() && !itItems->is_array())
				{
					throw std::runtime_error("object don't have array \"Items\"");
				}

				for (auto &name : *itItems)
				{
					if (!name.is_string())
					{
						throw std::runtime_error("\"Items\" not string array");
					}

					mapItemCount.emplace(name.get<std::string>(), szCurPos);//设置为vector的下标
				}
			}

			return true;
		}
		catch (const Json::exception &e)
		{
			printf("Json exception: %s\n", e.what());
		}
		catch (const std::runtime_error &e)
		{
			printf("Handle exception: %s\n", e.what());
		}

#undef TRY_READ_FIELD

		printf("Use Default Value!\n");
		SetDefault();

		return false;
	}

	void SetDefault(void)
	{
		cscDefault.SetDefault();
		icDefault.SetItemStackCount(icDefault.DefaultItemStackCount(), cscDefault);
	}


	ItemLevel CalculateLevels(const std::string &strItemName, uint64_t u64Count)
	{
		const auto &icCurrent = GetItemCount(strItemName);
		ItemLevel level = { 0 };

		//挨个计算
		if (icCurrent.szLargeChestShulkerBoxItemCount != 1 && u64Count >= icCurrent.szLargeChestShulkerBoxItemCount)
		{
			level.u64LargeChestShulkerBox = (u64Count / icCurrent.szLargeChestShulkerBoxItemCount);//如果1大箱盒只有1个，那么跳过设置
		}
		if (icCurrent.szChestShulkerBoxItemCount != 1 && u64Count >= icCurrent.szChestShulkerBoxItemCount)//如果1箱盒只有1个，那么跳过设置
		{
			level.u64ChestShulkerBox = (u64Count / icCurrent.szChestShulkerBoxItemCount) % (cscDefault.szLargeChestSlotCount / cscDefault.szChestSlotCount);
		}
		if (icCurrent.szShulkerBoxItemCount != 1 && u64Count >= icCurrent.szShulkerBoxItemCount)//如果1盒只有1个，那么跳过设置
		{
			level.u64ShulkerBox = (u64Count / icCurrent.szShulkerBoxItemCount) % cscDefault.szShulkerBoxSlotCount;
		}
		if (icCurrent.szSetItemCount != 1 && u64Count >= icCurrent.szSetItemCount)//如果1组只有1个，那么跳过设置
		{
			level.u64SetItem = (u64Count / icCurrent.szSetItemCount) % cscDefault.szShulkerBoxSlotCount;
		}
		//if (u64Count > 0)//无需判断
		{
			level.u64Item = u64Count % icCurrent.szSetItemCount;
		}

		return level;
	}
};

