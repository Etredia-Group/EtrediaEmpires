@echo off

set ASSETS=../../assets
set COMPILE=%CD%/../../out/build/DEBUG/sdk/assets/compile_assets.exe

set MATERIAL=4
set SAVE=2

for %%F in (materials/*) do %COMPILE% "materials/%%~nF.json" "%ASSETS%/materials/%%~nF" %MATERIAL% && echo "Compiled: %%~nF"
for %%F in (saves/*) do %COMPILE% "saves/%%~nF.json" "%ASSETS%/scenarios/%%~nF" %SAVE% && echo "Compiled: %%~nF"