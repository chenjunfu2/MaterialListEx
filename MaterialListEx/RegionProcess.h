#pragma once

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

#include "BlockProcess.hpp"
#include "EntityProcess.hpp"
#include "ItemProcess.hpp"
#include "NBT_Node.hpp"

template <typename Map> struct MapSortList {
  using MapPair = Map::value_type;

  // map用于查重统计，不需要真的用key来查找，所以key是什么无所谓
  Map map{}; // 创建方块状态到物品映射map
  // 通过vector创建map的元素排序（存储pair引用）std::reference_wrapper<>
  std::vector<std::reference_wrapper<MapPair>> listSort{};

  static bool SortCmp(MapPair &l, MapPair &r) {
    if (l.second == r.second) // 数量相等情况下按key排序
    {
      return l.first < r.first; // 升序
    } else                      // 否则val排序
    {
      return l.second > r.second; // 降序
    }
  }

  void SortElement(void) {
    // 提前扩容减少插入开销
    listSort.reserve(map.size());
    listSort.assign(map.begin(), map.end()); // 迭代器范围插入
    // 对物品按数量进行排序
    std::sort(listSort.begin(), listSort.end(), SortCmp);
  }
};

struct RegionStats {
  NBT_Node::NBT_String sRegionName{};

  // 简化声明
#define MAPSORTLIST(key, val, size, name)                                      \
  MapSortList<std::unordered_map<key, val, decltype(&key::Hash),               \
                                 decltype(&key::Equal)>>                       \
      name {                                                                   \
    .map { size, &key::Hash, &key::Equal }                                     \
  }

  // 方块（原本形式）
  MAPSORTLIST(BlockInfo, uint64_t, 128, mslBlock);

  // 方块（转换到物品形式）
  MAPSORTLIST(NoTagItemInfo, uint64_t, 128, mslBlockItem);

  // 方块实体（原本形式）先不做
  // MAPSORTLIST(TileEntityInfo, uint64_t, 128, mslTileEntity);

  // 方块实体容器
  MAPSORTLIST(ItemInfo, uint64_t, 128, mslTileEntityContainer);

  // 实体（原本形式）
  MAPSORTLIST(EntityInfo, uint64_t, 128, mslEntity);

  // 实体（转换到物品形式）先不做
  // MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityItem);
  // 如果做这个，那么要在实体容器内排除掉落物，然后把掉落物转换到这里面

  // 实体容器
  MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityContainer);

  // 实体物品栏
  MAPSORTLIST(ItemInfo, uint64_t, 128, mslEntityInventory);

#undef MAPSORTLIST
};

using RegionStatsList = std::vector<RegionStats>;

RegionStatsList RegionProcess(const NBT_Node::NBT_Compound &cpRegions);
