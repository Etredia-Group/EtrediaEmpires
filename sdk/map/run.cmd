@echo off

set ASSETS=../../assets
set COMPILE=%CD%/../../out/build/DEBUG/sdk/map/convert_map.exe

for %%F in (maps/*) do %COMPILE% "maps/%%~nF.bmp" "%ASSETS%/map/%%~nF" && echo "Compiled: %%~nF"
