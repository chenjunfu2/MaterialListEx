@echo off
setlocal

:: ��ȡ����ĳ�����
set "program=%1"

:: ���û�д��������������ʾ���˳�
if "%program%"=="" (
    echo ���ṩ���Գ�������Ϊ����!
    exit /b 1
)

:: ������ǰ�ļ����µ����� .litematic �ļ�
for %%f in (*.litematic) do (
    "%program%" "%%f"
)

endlocal
@echo on
