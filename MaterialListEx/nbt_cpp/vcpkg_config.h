#pragma once

/// @file
/// @brief 根据已安装的可选依赖提供定义
/// @note 这是一个示例配置文件，默认定义了所有的可选库
/// @note 如果用户自行通过代码文件安装，则可根据依赖安装情况注释掉未安装依赖的相关定义
/// @note 如果用户通过vcpkg安装此库，则请不要修改安装目录下的此文件，vcpkg会自动根据可选库生成具体内容

//use zlib
#define CJF2_NBT_CPP_USE_ZLIB ///< 安装zlib库则定义，否则请注释

//use xxhash
#define CJF2_NBT_CPP_USE_XXHASH ///< 安装xxhash库则定义，否则请注释