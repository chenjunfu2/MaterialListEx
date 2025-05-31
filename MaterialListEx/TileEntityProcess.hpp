#pragma once

#include "ItemProcess.hpp"
#include "MUTF8_Tool.hpp"
#include "NBT_Node.hpp"

#include <unordered_map>

class TileEntityProcess {
public:
  TileEntityProcess() = delete;
  ~TileEntityProcess() = delete;

  struct TileEntityContainerStats {
    const NBT_Node::NBT_String *psTileEntityName{};
    const NBT_Node *pItems{};
  };

  using TileEntityContainerStatsList = std::vector<TileEntityContainerStats>;

private:
  // 处理多种集合数据情况并映射到TileEntityContainerStats
  static bool
  MapTileEntityContainerStats(const NBT_Node::NBT_Compound &teCompound,
                              TileEntityContainerStats &teStats) {
    /*
            有好几种情况：item-Compound、Items-List、特殊名称-Compound
    */

    // 尝试寻找Items（普通多格容器）
    if (const auto pItems = teCompound.Search(MU8STR("Items"));
        pItems != NULL && pItems->IsList()) {
      teStats.pItems = pItems;
      return true;
    }

    // 尝试处理特殊方块
    // 用于映射特殊的方块实体容器里物品的名字
    const static std::unordered_map<NBT_Node::NBT_String, NBT_Node::NBT_String>
        mapContainerTagName = {
            {MU8STR("minecraft:jukebox"), MU8STR("RecordItem")},
            {MU8STR("minecraft:lectern"), MU8STR("Book")},
            {MU8STR("minecraft:brushable_block"), MU8STR("item")},
        };

    if (teStats.psTileEntityName == NULL) {
      // TODO:如果没有方块实体id，则通过方块->方块实体映射表查找，而不是返回失败
      return false;
    }

    // 查找方块实体id是否在map中
    const auto findIt = mapContainerTagName.find(*teStats.psTileEntityName);
    if (findIt ==
        mapContainerTagName.end()) // 不在，不是可以存放物品的容器，跳过
    {
      return false; // 跳过此方块实体
    }

    // 通过映射名查找对应物品存储位置
    const auto pSearch = teCompound.Search(findIt->second);
    if (pSearch == NULL) // 查找失败
    {
      return false; // 跳过此方块实体
    }

    // 放入结构内
    teStats.pItems = pSearch;
    return true;
  }

public:
  static TileEntityContainerStatsList
  GetTileEntityContainerStats(const NBT_Node::NBT_Compound &RgCompound) {
    // 获取方块实体列表
    const auto &listTileEntity = RgCompound.GetList(MU8STR("TileEntities"));
    TileEntityContainerStatsList listTileEntityStats{};
    listTileEntityStats.reserve(listTileEntity.size()); // 提前扩容

    for (const auto &it : listTileEntity) {
      const auto &cur = GetCompound(it);

      TileEntityContainerStats teStats{cur.HasString(MU8STR("id"))};
      if (MapTileEntityContainerStats(cur, teStats)) {
        listTileEntityStats.emplace_back(std::move(teStats));
      }
    }

    return listTileEntityStats;
  }

  static ItemProcess::ItemStackList TileEntityContainerStatsToItemStack(
      const TileEntityContainerStats &stContainerStats) {
    ItemProcess::ItemStackList listItemStack{};

    auto tag = stContainerStats.pItems->GetTag();
    if (tag == NBT_Node::TAG_Compound) // 只有一格物品
    {
      if (stContainerStats.pItems->GetCompound().empty()) {
        return listItemStack; // 空，直接返回
      }
      listItemStack.push_back(ItemProcess::ItemCompoundToItemStack(
          stContainerStats.pItems->GetCompound()));
    } else if (tag == NBT_Node::TAG_List) // 多格物品列表
    {
      const auto &tmp = stContainerStats.pItems->GetList();
      for (const auto &it : tmp) {
        if (it.GetCompound().empty()) {
          continue; // 空，处理下一个
        }
        listItemStack.push_back(
            ItemProcess::ItemCompoundToItemStack(it.GetCompound()));
      }
    }

    return listItemStack;
  }
};