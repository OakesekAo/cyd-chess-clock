@echo off
REM ESP32 Chess Clock - Build All Variants
REM This script builds firmware for all 3 display variants

setlocal enabledelayedexpansion

set BOOT_APP0=%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\3.0.4\tools\partitions\boot_app0.bin
set FQBN=esp32:esp32:esp32
set SKETCH=chess-clock
set INO_FILE=chess-clock\chess-clock.ino
set ARDUINO_LIBS=%USERPROFILE%\Documents\Arduino\libraries

echo ============================================
echo ESP32 Chess Clock - Building All Variants
echo ============================================
echo.

REM Check arduino-cli is available
where arduino-cli >nul 2>&1
if errorlevel 1 (
    echo ERROR: arduino-cli not found in PATH
    echo Install with: winget install ArduinoSA.CLI
    exit /b 1
)

REM Copy TFT_eSPI User_Setup.h to Arduino libraries folder
echo Copying TFT_eSPI User_Setup.h to Arduino libraries...
if exist "%ARDUINO_LIBS%\TFT_eSPI" (
    copy /Y "TFT_eSPI\User_Setup.h" "%ARDUINO_LIBS%\TFT_eSPI\User_Setup.h"
    echo   User_Setup.h copied to %ARDUINO_LIBS%\TFT_eSPI\
) else (
    echo   WARNING: TFT_eSPI library not found at %ARDUINO_LIBS%\TFT_eSPI
)

REM Copy LVGL lv_conf.h to Arduino libraries folder  
echo Copying LVGL lv_conf.h to Arduino libraries...
if exist "%ARDUINO_LIBS%\lvgl" (
    copy /Y "lvgl\src\lv_conf.h" "%ARDUINO_LIBS%\lvgl\src\lv_conf.h"
    copy /Y "lvgl\src\lv_conf.h" "%ARDUINO_LIBS%\lv_conf.h"
    echo   lv_conf.h copied to %ARDUINO_LIBS%\lvgl\ and %ARDUINO_LIBS%\
) else (
    echo   WARNING: lvgl library not found at %ARDUINO_LIBS%\lvgl
)
echo.

REM Clean build folder
if exist build rmdir /s /q build
mkdir build

REM Build each variant
call :build_variant 24_ILI9341
call :build_variant 24_ST7789  
call :build_variant 28_ILI9341

REM Reset to default variant
echo.
echo Resetting source to default variant (28_ILI9341)...
powershell -Command "(Get-Content '%INO_FILE%') -replace '#define CHESS_CLOCK_24_ILI9341', '// #define CHESS_CLOCK_24_ILI9341' -replace '#define CHESS_CLOCK_24_ST7789', '// #define CHESS_CLOCK_24_ST7789' -replace '// #define CHESS_CLOCK_28_ILI9341', '#define CHESS_CLOCK_28_ILI9341' | Set-Content '%INO_FILE%'"

echo.
echo ============================================
echo All builds complete!
echo ============================================
echo.
echo Output files:
for %%V in (24_ILI9341 24_ST7789 28_ILI9341) do (
    echo   build\%%V\
    dir /b "build\%%V\*.bin" 2>nul
)
echo.
echo To upload: Create a GitHub Release and attach all .bin files
echo.

endlocal
exit /b 0

:build_variant
set VARIANT=%1
echo.
echo Building %VARIANT%...
echo ----------------------------------------

mkdir build\%VARIANT%

REM Modify source to enable this variant
powershell -Command "(Get-Content '%INO_FILE%') -replace '#define CHESS_CLOCK_24_ILI9341', '// #define CHESS_CLOCK_24_ILI9341' -replace '#define CHESS_CLOCK_24_ST7789', '// #define CHESS_CLOCK_24_ST7789' -replace '#define CHESS_CLOCK_28_ILI9341', '// #define CHESS_CLOCK_28_ILI9341' -replace '// #define CHESS_CLOCK_%VARIANT%', '#define CHESS_CLOCK_%VARIANT%' | Set-Content '%INO_FILE%'"

REM Compile
arduino-cli compile --fqbn %FQBN% %SKETCH% --output-dir build\%VARIANT%

if errorlevel 1 (
    echo ERROR: Compilation failed for %VARIANT%
    exit /b 1
)

REM Copy boot_app0.bin
copy "%BOOT_APP0%" "build\%VARIANT%\boot_app0-%VARIANT%.bin" >nul

REM Rename output files
ren "build\%VARIANT%\chess-clock.ino.bin" "firmware-%VARIANT%.bin"
ren "build\%VARIANT%\chess-clock.ino.bootloader.bin" "bootloader-%VARIANT%.bin"
ren "build\%VARIANT%\chess-clock.ino.partitions.bin" "partitions-%VARIANT%.bin"

REM Clean up intermediate files
del "build\%VARIANT%\chess-clock.ino.elf" 2>nul
del "build\%VARIANT%\chess-clock.ino.map" 2>nul
del "build\%VARIANT%\chess-clock.ino.merged.bin" 2>nul

echo %VARIANT% complete!
exit /b 0
