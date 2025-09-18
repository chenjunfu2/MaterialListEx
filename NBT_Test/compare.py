import sys
import hashlib

def calculate_file_hash(file_path):
    """计算文件的MD5哈希值"""
    hash_md5 = hashlib.md5()
    try:
        with open(file_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
        return hash_md5.hexdigest()
    except Exception as e:
        print(f"错误: 无法读取文件 {file_path}: {str(e)}")
        return None

def compare_files(file_paths):
    """比较多个文件的哈希值"""
    # 计算每个文件的哈希值
    file_hashes = {}
    for file_path in file_paths:
        file_hash = calculate_file_hash(file_path)
        if file_hash is not None:
            if file_hash not in file_hashes:
                file_hashes[file_hash] = []
            file_hashes[file_hash].append(file_path)
    
    # 分析结果
    if len(file_hashes) == 0:
        print("没有有效的文件可供比较")
        return
    
    if len(file_hashes) == 1:
        print(f"所有文件完全相同!")
        for file_path in file_hashes[list(file_hashes.keys())[0]]:
            print(f"  - {file_path}")
    else:
        print(f"发现 {len(file_hashes)} 组不同的文件:")
        for i, (file_hash, files) in enumerate(file_hashes.items(), 1):
            print(f"\n第 {i} 组，共{len(files)}个，哈希值: {file_hash}")
            for file_path in files:
                print(f"  - {file_path}")

def main():
    if len(sys.argv) < 2:
        input("\n请将需要比较的多个文件拖拽到本脚本上")
        return
    
    file_paths = sys.argv[1:]
    print(f"正在比较以下{len(file_paths)}个文件:")
    for path in file_paths:
        print(f"  - {path}")
    
    print("\n" + "="*50)
    compare_files(file_paths)
    print("\n" + "="*50)
    input("\n按回车键退出...")

if __name__ == "__main__":
    main()