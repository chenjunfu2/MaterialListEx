﻿#include <iostream>
#include <filesystem>
#include <cstdlib>

std::string GetCurrentModulePath()//获取程序路径名称，然后裁切掉程序名，获取父路径
{
	std::filesystem::path sPath{ _pgmptr };//你msvc特有的_pgmptr就是当前程序路径
	return sPath.parent_path().string();
}


int main(void)
{
	printf("current:[%s]\n", GetCurrentModulePath().c_str());
	system("pause");
	return 0;
}