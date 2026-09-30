@echo off
rem ROH_pending.dll -> ROH.dll 自动同步：游戏关掉（锁释放）后立即覆盖，最多等 2 小时
cd /d "%~dp0"
for /l %%i in (1,1,240) do (
  if not exist build\ROH_pending.dll exit /b 0
  copy /Y build\ROH_pending.dll build\ROH.dll >nul 2>&1
  if not errorlevel 1 (
    del /f /q build\ROH_pending.dll >nul 2>&1
    echo [%date% %time%] ROH_pending.dll synced to ROH.dll >> build\sync.log
    exit /b 0
  )
  timeout /t 30 /nobreak >nul
)
