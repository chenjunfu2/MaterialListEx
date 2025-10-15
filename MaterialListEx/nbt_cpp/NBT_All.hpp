#pragma once

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
#define USE_GZIPZLIB//安装gzip与zlib库
#define USE_XXHASH//安装xxhash库

解锁的功能有：
nbt解压
nbt哈希
*/