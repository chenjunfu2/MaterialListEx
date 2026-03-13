# MaterialListEx
投影材料列表扩展版，除了支持输出方块材料列表外，还额外支持输出实体材料列表、容器内物品材料列表等  

## 使用方法
从Release处下载最新构建的exe、语言json、方块堆叠列表json，将他们放在同一个文件夹，把需要转换的投影（可多个）拖动到exe上松手，即可转换完成   
[配置文件说明](config/README.md)


## 注意事项
目前暂时只支持1.20.1数据版本  
如果需要更高版本，可以尝试使用[版本降低转换器](https://github.com/chenjunfu2/Litematic_V7_To_V6)进行数据格式转换  
语言文件可以直接从MC资源文件内提取后替换，这样就能解析最新版本的名称  

## 其它
该项目使用了`zlib`、`nlohmann-json`、`xxhash`库，相关许可证在对应库目录下可以找到  
该项目使用的NBT库已独立发行：[NBT_CPP](https://github.com/chenjunfu2/NBT_CPP/)  

## 跨平台
**Release仅提供Windows x64构建版本，Linux用户请自行构建**  
**其他平台暂时不考虑兼容，如有需求请自行尝试**  
  
`Linux`请使用`Clang`或`GCC`通过`CMakeLists.txt`构建  
`Windows`可使用`MSVC`通过`*.sln`构建，或使用与`Linux`相同的构建方式  
  

## Star History
[![Star History Chart](https://api.star-history.com/svg?repos=chenjunfu2/MaterialListEx&type=date&legend=top-left)](https://www.star-history.com/#chenjunfu2/MaterialListEx&type=date&legend=top-left)
