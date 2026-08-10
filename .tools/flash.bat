@echo off
setlocal enabledelayedexpansion

:: Usage: flash.bat [JLinkRoot] [ELF] [Device] [Interface] [Speed]

set "JLROOT=%~1"
set "ELF_IN=%~2"
set "DEV=%~3"
set "ITF=%~4"
set "SPEED=%~5"

:: --- 从 .ioc 读取芯片型号 ---
if "%DEV%"=="" (
    set "DEV="
    for %%F in ("%~dp0..\*.ioc") do (
        for /f "tokens=2 delims==" %%A in ('findstr /i "^Mcu" "%%F" 2^>nul') do (
            :: STM32F407ZGTx -> STM32F407ZG
            for /f "tokens=1,2 delims=." %%B in ("%%A") do set "DEV=%%B%%C"
        )
    )
)

:: --- 自动查找 ELF ---
set "PROJECT_NAME="
set "ELF="

if not "%ELF_IN%"=="" (
    if exist "%ELF_IN%" set "ELF=%ELF_IN%"
)

if "%ELF%"=="" (
    if exist "%~dp0..\build\Release\*.elf" (
        for %%F in ("%~dp0..\build\Release\*.elf") do (
            set "ELF=%%F"
            set "PROJECT_NAME=%%~nF"
            goto :found_elf
        )
    )
)
if "%ELF%"=="" (
    if exist "%~dp0..\build\Debug\*.elf" (
        for %%F in ("%~dp0..\build\Debug\*.elf") do (
            set "ELF=%%F"
            set "PROJECT_NAME=%%~nF"
            goto :found_elf
        )
    )
)
if "%ELF%"=="" (
    if exist "%~dp0..\build\*.elf" (
        for %%F in ("%~dp0..\build\*.elf") do (
            set "ELF=%%F"
            set "PROJECT_NAME=%%~nF"
            goto :found_elf
        )
    )
)

:found_elf
if "%ELF%"=="" (
    echo ERROR: No .elf file found
    echo Build the project first.
    exit /b 1
)

if "%PROJECT_NAME%"=="" (
    for %%F in ("%ELF%") do set "PROJECT_NAME=%%~nF"
)

:: --- 检查是否有 .ioc 文件，如果没读到芯片型号则报错 ---
if "%DEV%"=="" (
    echo ERROR: Could not read MCU from .ioc file
    echo Please specify device manually: flash.bat [JLinkRoot] [ELF] [Device]
    exit /b 1
)

:: --- Fallbacks for interface and speed ---
if "%ITF%"=="" set "ITF=SWD"
if "%SPEED%"=="" set "SPEED=4000"

:: --- 查找 JLink.exe ---
set "JL="

if not "%JLROOT%"=="" (
    if exist "%JLROOT%\JLink.exe" set "JL=%JLROOT%\JLink.exe"
)

if "%JL%"=="" (
    for /f "skip=2 tokens=2*" %%A in ('reg query "HKEY_CURRENT_USER\Software\Keil\ARM\JLink" /v "InstallPath" 2^>nul') do (
        if exist "%%B\JLink.exe" set "JL=%%B\JLink.exe"
    )
)

if "%JL%"=="" (
    for /f "skip=2 tokens=2*" %%A in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\SEGGER\J-Link" /v "InstallPath" 2^>nul') do (
        if exist "%%B\JLink.exe" set "JL=%%B\JLink.exe"
    )
)

if "%JL%"=="" (
    for /d %%D in ("C:\Program Files\SEGGER\JLink*") do (
        if exist "%%D\JLink.exe" set "JL=%%D\JLink.exe"
    )
)
if "%JL%"=="" (
    for /d %%D in ("C:\Program Files (x86)\SEGGER\JLink*") do (
        if exist "%%D\JLink.exe" set "JL=%%D\JLink.exe"
    )
)

if not exist "%JL%" (
    echo ERROR: JLink.exe not found
    exit /b 1
)

:: --- 显示信息 ---
echo ========================================
echo Project: %PROJECT_NAME%
echo Device:  %DEV%
echo ELF:     %ELF%
echo JLink:   %JL%
echo ========================================
echo.

:: --- ANSI color ---
for /f %%a in ('echo prompt $E^| cmd') do set "ESC=%%a"

:: --- 创建脚本 ---
set "SCRIPT=%TEMP%\jlink_%RANDOM%.jlink"
set "LOG=%TEMP%\jlink_flash_%RANDOM%.log"

(
echo r
echo h
echo loadfile "%ELF%"
echo r
echo g
echo exit
) > "%SCRIPT%"

:: --- 烧录 ---
echo Flashing "%ELF%" ...
echo.
"%JL%" -device %DEV% -if %ITF% -speed %SPEED% -autoconnect 1 -nogui 1 -CommanderScript "%SCRIPT%" > "%LOG%" 2>&1

:: --- 显示日志 ---
type "%LOG%"

:: --- 判断结果 ---
set "FLASH_OK=1"
findstr /i /c:"cannot connect" /c:"could not connect" /c:"error:" /c:"failed" /c:"timeout" /c:"unable to" "%LOG%" >nul && set "FLASH_OK=0"
findstr /c:"O.K." "%LOG%" >nul || set "FLASH_OK=0"

del "%SCRIPT%" 2>nul
del "%LOG%" 2>nul

echo.
if "%FLASH_OK%"=="1" (
    echo %ESC%[30;42m                                                            %ESC%[0m
    echo %ESC%[30;42m   ####   FLASH SUCCESS   ####   device=%DEV%                %ESC%[0m
    echo %ESC%[30;42m                                                            %ESC%[0m
    echo %ESC%[92m^>^>^> Download OK. Target reset and running.%ESC%[0m
    exit /b 0
) else (
    echo %ESC%[97;41m                                                            %ESC%[0m
    echo %ESC%[97;41m   ####   FLASH FAILED   ####   device=%DEV%                 %ESC%[0m
    echo %ESC%[97;41m                                                            %ESC%[0m
    echo %ESC%[91m^>^>^> See JLink output above.%ESC%[0m
    exit /b 1
)