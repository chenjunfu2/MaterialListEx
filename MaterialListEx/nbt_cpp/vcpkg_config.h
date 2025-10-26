//示例配置文件，实际安装情况由vcpkg根据可选库生成具体内容
//示例配置文件内默认包含所有可选库

//use zlib
#ifndef USE_ZLIB
#define USE_ZLIB
#else
#undef USE_ZLIB
#endif

//use xxhash
#ifndef USE_XXHASH
#define USE_XXHASH
#else
#undef USE_XXHASH
#endif