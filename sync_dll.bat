@echo off
rem PVZOverlay_pending.dll -> PVZOverlay.dll auto-sync: overwrite once the game lock releases (up to 2h)
cd /d "%~dp0"
for /l %%i in (1,1,240) do (
  if not exist build\PVZOverlay_pending.dll exit /b 0
  copy /Y build\PVZOverlay_pending.dll build\PVZOverlay.dll >nul 2>&1
  if not errorlevel 1 (
    del /f /q build\PVZOverlay_pending.dll >nul 2>&1
    echo [%date% %time%] PVZOverlay_pending.dll synced to PVZOverlay.dll >> build\sync.log
    exit /b 0
  )
  timeout /t 30 /nobreak >nul
)
