//示例配置文件，实际安装情况由vcpkg根据可选库生成具体内容
//示例配置文件内默认包含所有可选库

//use zlib
#ifndef __USE_ZLIB__
#define __USE_ZLIB__
#else
#undef __USE_ZLIB__
#endif

//use xxhash
#ifndef __USE_XXHASH__
#define __USE_XXHASH__
#else
#undef __USE_XXHASH__
#endif