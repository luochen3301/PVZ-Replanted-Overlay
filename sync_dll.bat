@echo off
rem PVZModMenu_pending.dll -> PVZModMenu.dll auto-sync: overwrite once the game lock releases (up to 2h)
cd /d "%~dp0"
for /l %%i in (1,1,240) do (
  if not exist build\PVZModMenu_pending.dll exit /b 0
  copy /Y build\PVZModMenu_pending.dll build\PVZModMenu.dll >nul 2>&1
  if not errorlevel 1 (
    del /f /q build\PVZModMenu_pending.dll >nul 2>&1
    echo [%date% %time%] PVZModMenu_pending.dll synced to PVZModMenu.dll >> build\sync.log
    exit /b 0
  )
  timeout /t 30 /nobreak >nul
)
