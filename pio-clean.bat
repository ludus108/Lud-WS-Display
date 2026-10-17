@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul
title LUD-WS - PlatformIO Cleanup

:: ============================================================
::  CONFIGURAZIONE
:: ============================================================
:: Cartella del progetto (default: cartella dello script)
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

:: Cartella globale PlatformIO
set "PLATFORMIO_HOME=%USERPROFILE%\.platformio"

:: Cartella di backup per le patch manuali
set "BACKUP_DIR=%PROJECT_DIR%\_pio_patch_backup"
:: ============================================================

:: --- Verifica: deve esserci .pio ---
if not exist "%PROJECT_DIR%\.pio" (
    echo.
    echo  [ERRORE] Cartella .pio non trovata in:
    echo          %PROJECT_DIR%
    echo.
    echo  Metti questo script nella cartella che contiene platformio.ini,
    echo  oppure lancialo da li'.
    echo.
    pause
    exit /b 1
)

:MENU
cls
echo.
echo  ============================================================
echo   LUD-WS - PlatformIO Cleanup
echo  ============================================================
echo.
echo   Progetto:   %PROJECT_DIR%
echo   PlatformIO: %PLATFORMIO_HOME%
echo.
call :SHOW_SIZES
echo.
echo  ------------------------------------------------------------
echo   [1]  Veloce      .pio\build             (sicuro)
echo   [2]  Media       + idedata.json         (sicuro)
echo   [3]  Globale     .platformio\.cache     (sicuro, ~GB)
echo   [4]  Completa    + libdeps              (perde patch manuali)
echo   [5]  Info        mostra dimensioni
echo   [0]  Esci
echo  ------------------------------------------------------------
set /p "SCELTA=Scelta: "

if "%SCELTA%"=="1" goto CLEAN_BUILD
if "%SCELTA%"=="2" goto CLEAN_BUILD_IDE
if "%SCELTA%"=="3" goto CLEAN_CACHE
if "%SCELTA%"=="4" goto CLEAN_FULL
if "%SCELTA%"=="5" goto INFO
if "%SCELTA%"=="0" exit /b 0
goto MENU


:: ============================================================
:: AZIONI
:: ============================================================

:CLEAN_BUILD
echo.
echo  [*] Pulizia .pio\build ...
if exist "%PROJECT_DIR%\.pio\build" (
    rmdir /s /q "%PROJECT_DIR%\.pio\build"
    echo  [OK] Rimossa .pio\build
) else (
    echo  [--] .pio\build non esiste
)
echo.
call :SHOW_SIZES
echo.
pause
goto MENU


:CLEAN_BUILD_IDE
echo.
echo  [*] Pulizia .pio\build + idedata.json ...
if exist "%PROJECT_DIR%\.pio\build" (
    rmdir /s /q "%PROJECT_DIR%\.pio\build"
    echo  [OK] Rimossa .pio\build
)
if exist "%PROJECT_DIR%\.pio\idedata.json" (
    del /q "%PROJECT_DIR%\.pio\idedata.json"
    echo  [OK] Rimosso .pio\idedata.json
)
echo.
call :SHOW_SIZES
echo.
pause
goto MENU


:CLEAN_CACHE
echo.
echo  [*] Pulizia cache globale .platformio\.cache ...
if exist "%PLATFORMIO_HOME%\.cache" (
    rmdir /s /q "%PLATFORMIO_HOME%\.cache"
    echo  [OK] Rimossa .platformio\.cache
) else (
    echo  [--] .platformio\.cache non esiste
)
echo.
call :SHOW_SIZES
echo.
pause
goto MENU


:CLEAN_FULL
echo.
echo  ============================================================
echo   ATTENZIONE
echo  ============================================================
echo   Questa operazione rimuove anche .pio\libdeps\, che contiene
echo   le librerie scaricate E LE PATCH MANUALI che hai applicato
echo   a ESP32_Display_Panel.
echo.
echo   Prima di procedere verra' fatto un backup automatico delle
echo   patch in:  %BACKUP_DIR%
echo.
set /p "CONF=Procedere? [s/N]: "
if /i not "%CONF%"=="s" goto MENU

call :BACKUP_PATCHES

echo.
echo  [*] Pulizia .pio\build, idedata, libdeps ...
if exist "%PROJECT_DIR%\.pio\build"    rmdir /s /q "%PROJECT_DIR%\.pio\build"
if exist "%PROJECT_DIR%\.pio\libdeps"  rmdir /s /q "%PROJECT_DIR%\.pio\libdeps"
if exist "%PROJECT_DIR%\.pio\idedata.json" del /q "%PROJECT_DIR%\.pio\idedata.json"
echo  [OK] Build e libdeps rimosse.

echo.
echo  [*] Pulizia cache globale .platformio\.cache ...
if exist "%PLATFORMIO_HOME%\.cache" rmdir /s /q "%PLATFORMIO_HOME%\.cache"
echo  [OK] Cache globale rimossa.

echo.
call :SHOW_SIZES
echo.
echo  Ricompila con:
echo    platformio run
echo.
pause
goto MENU


:INFO
echo.
call :SHOW_SIZES
echo.
pause
goto MENU


:: ============================================================
:: SOTTOPROGRAMMI
:: ============================================================

:SHOW_SIZES
set "SZ_PIO=0"
set "SZ_BUILD=0"
set "SZ_LIBDEPS=0"
set "SZ_CACHE=0"

for /f "usebackq tokens=1,2 delims==" %%A in (`powershell -NoProfile -Command ^
  "$p=@{PIO='%PROJECT_DIR%\.pio';BUILD='%PROJECT_DIR%\.pio\build';LIBDEPS='%PROJECT_DIR%\.pio\libdeps';CACHE='%PLATFORMIO_HOME%\.cache'}; foreach($k in 'PIO','BUILD','LIBDEPS','CACHE'){ $v=0; $path=$p[$k]; if(Test-Path $path){ $s=(Get-ChildItem -LiteralPath $path -Recurse -File -ErrorAction SilentlyContinue ^| Measure-Object Length -Sum).Sum; if($s){$v=[math]::Round($s/1MB,1)} }; Write-Output \"$k=$v\" }"`) do (
    if "%%A"=="PIO"     set "SZ_PIO=%%B"
    if "%%A"=="BUILD"   set "SZ_BUILD=%%B"
    if "%%A"=="LIBDEPS" set "SZ_LIBDEPS=%%B"
    if "%%A"=="CACHE"   set "SZ_CACHE=%%B"
)

echo   .pio (totale):       !SZ_PIO! MB
echo     .pio\build:        !SZ_BUILD! MB
echo     .pio\libdeps:      !SZ_LIBDEPS! MB
echo   .platformio\.cache:  !SZ_CACHE! MB
goto :eof


:BACKUP_PATCHES
if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"
set "PANEL_HOST=%PROJECT_DIR%\.pio\libdeps\display\ESP32_Display_Panel\src\drivers\host"
if exist "%PANEL_HOST%\esp_panel_host_i2c.hpp"  copy /y "%PANEL_HOST%\esp_panel_host_i2c.hpp"  "%BACKUP_DIR%\" >nul
if exist "%PANEL_HOST%\esp_panel_host_spi.hpp"  copy /y "%PANEL_HOST%\esp_panel_host_spi.hpp"  "%BACKUP_DIR%\" >nul
if exist "%PANEL_HOST%\esp_panel_host_qspi.hpp" copy /y "%PANEL_HOST%\esp_panel_host_qspi.hpp" "%BACKUP_DIR%\" >nul
if exist "%PANEL_HOST%\esp_panel_host_rgb.hpp"  copy /y "%PANEL_HOST%\esp_panel_host_rgb.hpp"  "%BACKUP_DIR%\" >nul
echo  [OK] Backup patch in: %BACKUP_DIR%
goto :eof