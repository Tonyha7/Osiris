@echo off
REM =====================================================
REM Osiris 项目编译脚本 - 带动态特征码注入
REM 每次编译生成不同的DLL特征码
REM =====================================================

setlocal enabledelayedexpansion

REM =====================================================
REM 在这里执行Python脚本生成动态特征码
REM =====================================================
echo.
echo [*] 开始生成动态特征码...
echo.

REM 检查Python是否可用
python --version >nul 2>&1
if errorlevel 1 (
    echo [!] 错误: 未找到Python环境
    echo [!] 请确保Python已安装并添加到PATH环境变量中
    exit /b 1
)

REM 执行签名生成脚本
python generate_signature.py
if errorlevel 1 (
    echo [!] 错误: 动态特征码生成失败
    exit /b 1
)

echo.
echo [*] 特征码注入完成，开始编译...
echo.

REM =====================================================
REM 执行MSBuild编译命令
REM =====================================================
msbuild Osiris.sln /p:Platform=x64 /p:Configuration=Release

REM 检查编译结果
if errorlevel 1 (
    echo.
    echo [!] 编译失败！
    exit /b 1
) else (
    echo.
    echo [+] 编译成功！
    echo [+] DLL已生成并注入了动态特征码
)

endlocal
