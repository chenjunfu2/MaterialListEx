#pragma once
#include <filesystem>
#include <stdlib.h>

std::filesystem::path GetCurrentModulePath()//��ȡ����·�����ƣ�Ȼ����е�����������ȡ��·��
{
	std::filesystem::path sPath{ _pgmptr };//��msvc���е�_pgmptr���ǵ�ǰ����·��
	return sPath.parent_path();
}