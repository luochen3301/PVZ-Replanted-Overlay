@echo off
rem Lawnbox_pending.dll -> Lawnbox.dll auto-sync: overwrite once the game lock releases (up to 2h)
cd /d "%~dp0"
for /l %%i in (1,1,240) do (
  if not exist build\Lawnbox_pending.dll exit /b 0
  copy /Y build\Lawnbox_pending.dll build\Lawnbox.dll >nul 2>&1
  if not errorlevel 1 (
    del /f /q build\Lawnbox_pending.dll >nul 2>&1
    echo [%date% %time%] Lawnbox_pending.dll synced to Lawnbox.dll >> build\sync.log
    exit /b 0
  )
  timeout /t 30 /nobreak >nul
)
