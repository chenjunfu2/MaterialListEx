@echo off
setlocal enabledelayedexpansion

:: 获取传入的程序名
set "program=%1"

:: 如果没有传入程序名，则提示并退出
if "%program%"=="" (
    echo 请提供测试程序名作为参数!
    exit /b 1
)

:: 计算总文件数
set count=0
for %%f in (*.litematic) do (
    set /a count+=1
)

echo 总共找到 %count% 个 .litematic 文件
echo.

:: 初始化测试索引
set index=0

:: 遍历当前文件夹下的所有 .litematic 文件
for %%f in (*.litematic) do (
    set /a index+=1
    echo 测试 #!index!: %%f
    "%program%" "%%f"
    echo.
)

endlocal
@echo on
