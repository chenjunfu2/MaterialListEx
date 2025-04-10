import json

def filter_item_minecraft(input_path, output_path):
    try:
        # 读取原始JSON文件
        with open(input_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # 筛选以"item.minecraft."开头的键值对
         # 筛选并替换键名
        filtered_data = {}
        for key, value in data.items():
            if key.startswith('item.minecraft.'):
                new_key = key.replace('item.minecraft.', 'minecraft:', 1)
                filtered_data[new_key] = value
            elif key.startswith('block.minecraft.'):
                new_key = key.replace('block.minecraft.', 'minecraft:', 1)
                filtered_data[new_key] = value
        
        # 写入到新JSON文件
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(filtered_data, f, ensure_ascii=False, indent=4)
        
        print(f"成功筛选出 {len(filtered_data)} 个条目，已保存至 {output_path}")
    
    except FileNotFoundError:
        print(f"错误：输入文件 {input_path} 不存在")
    except json.JSONDecodeError:
        print(f"错误：{input_path} 不是有效的JSON文件")
    except Exception as e:
        print(f"发生未知错误：{str(e)}")

# 使用示例（修改以下路径）
input_file = "zh_cn.json"   # 原始JSON文件路径
output_file = "zh_cn__.json" # 输出JSON文件路径

filter_item_minecraft(input_file, output_file)