@echo off
setlocal
echo Start either Minecraftify Half-Life profile in Minecraft Launcher first.
set /p "MinecraftifyNode="<"%~dp0halflife-minecraftify\local\node-runtime.txt"
"%MinecraftifyNode%" "%~dp0halflife-minecraftify\tools\start-native.mjs" halflife
if errorlevel 1 pause
endlocal
