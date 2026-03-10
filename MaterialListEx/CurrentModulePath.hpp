#pragma once

#include <filesystem>

namespace CURRENT_MODULE_PATH
{
	static inline std::filesystem::path pathCurrentModule = { "./" };
}

template<typename CHAR_T>
inline void SetCurrentModulePath(const CHAR_T *pCommandLine0) noexcept//程序启动设置程序路径
{
	CURRENT_MODULE_PATH::pathCurrentModule = std::filesystem::path{ pCommandLine0 }.parent_path();
}

inline const std::filesystem::path &GetCurrentModulePath(void) noexcept//获取程序路径名称，然后裁切掉程序名，获取父路径
{
	return CURRENT_MODULE_PATH::pathCurrentModule;
}
