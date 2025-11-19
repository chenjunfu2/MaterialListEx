#pragma once

/// @file
/// @brief 此头文件包含所有可选独立模块，其它模块大部分为库内依赖，不独立使用。
/// 如果需要单独包部分含可选模块，可以在此文件内预览需要的模块头文件。
#include "NBT_Node.hpp"
#include "NBT_Node_View.hpp"
#include "NBT_Helper.hpp"
#include "NBT_Reader.hpp"
#include "NBT_Writer.hpp"
#include "NBT_IO.hpp"

/*
此头文件包含所有公开可选NBT模块
部分模块允许通过宏定义解锁更多功能
前提是安装了宏定义需求的库

目前的可选接口有：
#define CJF2_NBT_CPP_USE_ZLIB//安装zlib库的情况下
#define CJF2_NBT_CPP_USE_XXHASH//安装xxhash库的情况下

解锁的功能有：
NBT_IO中的nbt压缩
NBT_Helper中的nbt哈希

说明：
vcpkg安装本库会自动在vcpkg_config.h头文件中
处理定义，否则需要手动处理，具体请参考文档
*/