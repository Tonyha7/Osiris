@echo off
REM =====================================================
REM Osiris 项目编译脚本 - 带动态特征码注入（高级版本）
REM 每次编译生成不同的DLL特征码
REM =====================================================

setlocal enabledelayedexpansion

REM 设置颜色和符号
set "SUCCESS=[+]"
set "INFO=[*]"
set "ERROR=[!]"
set "DEBUG=[DEBUG]"

REM 检查命令行参数
set "CMD=%1"
if "%CMD%"=="" (
    set "CMD=build"
)

REM 处理不同的命令
if /i "%CMD%"=="help" goto show_help
if /i "%CMD%"=="clean" goto clean_build
if /i "%CMD%"=="rebuild" goto rebuild
if /i "%CMD%"=="build" goto build
if /i "%CMD%"=="sig-only" goto sig_only

echo %ERROR% 未知命令: %CMD%
echo 请运行: build.bat help
exit /b 1

:show_help
echo.
echo =====================================================
echo Osiris 编译脚本 - 动态特征码版本
echo =====================================================
echo.
echo 用法: build.bat [命令]
echo.
echo 命令:
echo   build       编译项目（带动态特征码注入）[默认]
echo   clean       清理编译输出
echo   rebuild     清理并重新编译
echo   sig-only    仅生成特征码不编译
echo   help        显示此帮助信息
echo.
echo 示例:
echo   build.bat build
echo   build.bat clean
echo   build.bat rebuild
echo.
goto end

:clean_build
echo.
echo %INFO% 清理编译输出...
echo %INFO% 删除源版本中间文件...
for /d %%d in (Source\BuildSignature) do (
    if exist "%%d" (
        rmdir /s /q "%%d" 2>nul
        echo %SUCCESS% 删除: %%d
    )
)
echo %SUCCESS% 清理完成
goto end

:rebuild
echo.
echo %INFO% 开始重新编译（清理+编译）...
call :clean_build
call :build
goto end

:sig_only
echo.
echo %INFO% 仅生成动态特征码...
python generate_signature.py
if errorlevel 1 (
    echo %ERROR% 特征码生成失败
    exit /b 1
)
echo %SUCCESS% 特征码生成完成
goto end

:build
echo.
echo =====================================================
echo Osiris 项目编译（带动态特征码注入）
echo =====================================================
echo.

REM =====================================================
REM 在这里执行Python脚本生成动态特征码
REM =====================================================
echo %INFO% 检查依赖环境...

REM 检查Python是否可用
python --version >nul 2>&1
if errorlevel 1 (
    echo %ERROR% 未找到Python环境
    echo %INFO% 请确保Python已安装并添加到PATH环境变量中
    exit /b 1
)

echo %SUCCESS% Python环境检查通过
echo.
echo %INFO% 开始生成动态特征码...
echo.

REM 执行签名生成脚本
python generate_signature.py
if errorlevel 1 (
    echo.
    echo %ERROR% 动态特征码生成失败！
    exit /b 1
)

echo.
echo %INFO% 特征码注入完成，开始编译...
echo.

REM =====================================================
REM 执行MSBuild编译命令
REM =====================================================
REM 检查MSBuild是否可用
where msbuild >nul 2>&1
if errorlevel 1 (
    echo %ERROR% 未找到msbuild
    echo %INFO% 请确保已安装Visual Studio和C++开发工具
    exit /b 1
)

echo %INFO% 开始MSBuild编译...
echo.

msbuild Osiris.sln /p:Platform=x64 /p:Configuration=Release

REM 检查编译结果
if errorlevel 1 (
    echo.
    echo %ERROR% ============================================
    echo %ERROR% 编译失败！
    echo %ERROR% ============================================
    exit /b 1
) else (
    echo.
    echo %SUCCESS% ============================================
    echo %SUCCESS% 编译成功！
    echo %SUCCESS% DLL已生成并注入了动态特征码
    echo %SUCCESS% ============================================
    echo.
)

goto end

:end
endlocal
exit /b %errorlevel%
