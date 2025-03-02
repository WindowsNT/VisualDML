@echo off
cls


REM Manifest Touch
f:\tools\pack\pack64.exe /m %CD%\Package.appxmanifest /n CN=EDD191C1-439D-4D37-B608-DD521142451D

msbuild DirectMLGraph.sln /clp:ErrorsOnly /p:Configuration="Release" /p:Platform=x64 /t:restore /p:RestorePackagesConfig=true
msbuild DirectMLGraph.sln /clp:ErrorsOnly /p:Configuration="Release" /p:Platform=x64 
call clbcall
call sign x64\Release\DirectMLGraph\DirectMLGraph.exe

REM Portable
f:\tools\pack\pack64.exe /i %CD%\app.ico /c x64w,%CD%\x64\Release\DirectMLGraph,DirectMLGraph.exe /o %CD%\DirectMLGraph.exe /u 88887777-0000-7654-A2E5-DECB481E355D
call sign DirectMLGraph.exe

REM MSIX
del DirectMLGraph.msix
rem makeappx pack /d x64\Release\DirectMLGraph /p %CD%\DirectMLGraph.msix > nul


call clbcall
del "Generated Files"\* /s /q 
del packages\* /s /q
del x64\* /s /q
del DirectMLGraph\x64\* /s /q

f:\tools\tu8\uploader\uploader SmallTools
msbuild DirectMLGraph.sln -target:Clean

git add *
git commit -m "Update"
git push

