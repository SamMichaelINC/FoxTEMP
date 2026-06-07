@echo off
chcp 65001 >nul

:MENU
cls
echo ===================================================
echo 🦊 FOXFW MASTER COMPILATION OPERATOR AUTOMATION
echo ===================================================
echo.
echo  [F] Clean Workspace + Flash Live USB (flash_usb_full)
echo  [U] Clean Workspace + Generate DFU Updater Package
echo  [N] Native Fast Compilation Pass Only (just fbt)
echo  [E] Close Terminal operator console window
echo.
echo ===================================================
echo.

:: Capture character keys instantly without pressing Enter
choice /c FUNE /n /m "Select your compilation route: "

if errorlevel 4 goto EXATOR
if errorlevel 3 goto JUSTFBT
if errorlevel 2 goto UPDPKG
if errorlevel 1 goto LVEUSB

:LVEUSB
cls
echo ===================================================
echo 🚀 CLEANING WORKSPACE AND LAUNCHING LIVE USB FLASH...
echo ===================================================
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist .sconsign.dblite del /f /q .sconsign.dblite
call fbt flash_usb_full COMPACT=1 DEBUG=0 FORCE=1
echo.
echo Press any key to return to Master Menu...
pause >nul
goto MENU

:UPDPKG
cls
echo ===================================================
echo 🚀 CLEANING WORKSPACE AND GENERATING UPDATER PACKAGE...
echo ===================================================
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist
if exist .sconsign.dblite del /f /q .sconsign.dblite
call fbt updater_package COMPACT=1 DEBUG=0 FORCE=1
echo.
echo Press any key to return to Master Menu...
pause >nul
goto MENU

:JUSTFBT
cls
echo ===================================================
echo 🚀 LAUNCHING NATIVE FAST COMPILATION PASS ONLY...
echo ===================================================
:: Fast Pass: Leaves the drive caches intact to build instantly!
call fbt COMPACT=1 DEBUG=0 FORCE=1
echo.
echo Press any key to return to Master Menu...
pause >nul
goto MENU

:EXATOR
cls
echo 📄 Closing operator console window...
exit
