# 这里存储应用程序需要用到的配置  
- `zh_cn.json` 用于键值翻译  
- `item_stack_count.json` 用于计算物品堆叠上限  

# 来源说明  

## `zh_cn.json`  
在`.minecraft\assets\indexes\`文件夹内，  
找到对应的`版本序号.json`（[版本序号查询](https://zh.minecraft.wiki/w/%E6%95%A3%E5%88%97%E8%B5%84%E6%BA%90%E6%96%87%E4%BB%B6?variant=zh-cn#%E8%B5%84%E6%BA%90%E7%B4%A2%E5%BC%95)），  
然后在内部搜索对应`语言id.json`，比如`zh_cn.json`，获得后面`hash`里的哈希值，  
去`.minecraft\assets\objects\`文件夹内，  
按照哈希值开头的两个值确定文件夹名称，  
再进入文件夹内找到与哈希值一致的文件名，  
拷贝出来并修改文件名为对应`语言id.json`，比如`zh_cn.json`，以获得目标文件
  
## `item_stack_count.json`
编写MC Mod（具体教程请自行查找，我这里使用[`Fabric template mod generator`](https://fabricmc.net/develop/template/)生成的1.20.1项目，使用Yarn映射）  
  
**首先是Mixin**  
`ItemsMixin.java`  
```Java
package chenjunfu2.itemstackcount.mixin;

import net.minecraft.item.Item;
import net.minecraft.item.Items;
import net.minecraft.registry.RegistryKey;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

import static chenjunfu2.itemstackcount.ItemStackCount.itemRegistry;

@Mixin(Items.class)
abstract class ItemsMixin
{
	@Inject(method = "register(Lnet/minecraft/registry/RegistryKey;Lnet/minecraft/item/Item;)Lnet/minecraft/item/Item;", at = @At(value = "HEAD"))
	private static void RegisterInject(RegistryKey<Item> key, Item item, CallbackInfoReturnable<Item> cir)
	{
		itemRegistry.put(key.getValue().toString(), item);//注册物品的时候自己保留一份注册表
	}
}
```
  
**然后是主类**  
`item_stack_count.java`  
```Java
package chenjunfu2.itemstackcount;

import com.google.gson.GsonBuilder;
import net.fabricmc.api.ModInitializer;

import net.minecraft.item.Item;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

public class ItemStackCount implements ModInitializer {
	public static final String MOD_ID = "itemstackcount";
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
	
	public static HashMap<String, Item> itemRegistry = new HashMap<>();
	
	@Override
	public void onInitialize()
	{
		LOGGER.info("itemRegistry init!");//执行到此注册表已初始化完成
		
		//根据物品堆叠上限数量进行分组与排序
		Map<Integer, List<String>> itemStackCount = new HashMap<>();
		for (var itemName : itemRegistry.keySet())
		{
			var itemCount = itemRegistry.get(itemName).getMaxCount();
			itemStackCount.computeIfAbsent(itemCount, k -> new ArrayList<>()).add(itemName);
		}
		List<Integer> sortedKeys = new ArrayList<>(itemStackCount.keySet());
		Collections.sort(sortedKeys);
		
		//初始化有序格式并序列化为Json
		List<Map<String, Object>> jsonList = new ArrayList<>();
		for (var itemCount : sortedKeys)
		{
			Map<String, Object> entry = new LinkedHashMap<>();//有序Map
			entry.put("Count", itemCount);
			entry.put("Items", itemStackCount.get(itemCount));
			jsonList.add(entry);
		}
		String json = new GsonBuilder().setPrettyPrinting().create().toJson(jsonList);
		
		//写入Json文件
		try {
			Path gameDir = Paths.get("").toAbsolutePath();
			Path outputFile = gameDir.resolve("item_stack_count.json");
			
			Files.writeString(outputFile, json);
			LOGGER.info("Json file written: {}", outputFile);
		} catch (IOException e) {
			LOGGER.error("Json file write fail: {}", e.toString());
		}
	}
}
```
  
**最后添加Mixin到配置**  
`itemstackcount.mixins.json`  
```Json
{
  "required": true,
  "package": "chenjunfu2.itemstackcount.mixin",
  "compatibilityLevel": "JAVA_17",
  "mixins": [
    "ItemsMixin"
  ],
  "injectors": {
    "defaultRequire": 1
  },
  "overwrites": {
    "requireAnnotations": true
  }
}
```
  
直接运行项目即可在run文件夹下获得`item_stack_count.json`  
