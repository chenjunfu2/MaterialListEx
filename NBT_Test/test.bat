@echo off
setlocal

:: 获取传入的程序名
set "program=%1"

:: 如果没有传入程序名，则提示并退出
if "%program%"=="" (
    echo 请提供测试程序名作为参数!
    exit /b 1
)

:: 遍历当前文件夹下的所有 .litematic 文件
for %%f in (*.litematic) do (
    "%program%" "%%f"
)

endlocal
@echo on
