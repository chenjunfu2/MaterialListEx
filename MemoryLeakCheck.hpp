#pragma once

#ifdef _DEBUG
#define new new( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

struct _SetCheckStatus
{
public:
	_SetCheckStatus(void)
	{
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	}
	~_SetCheckStatus(void)
	{
		//_CrtDumpMemoryLeaks();程序结尾调用，有_CRTDBG_LEAK_CHECK_DF标志会自动调用可以省略，因为需要在所有析构函数后进行检查
	}
}g_SetCheckStatus{};
#endif