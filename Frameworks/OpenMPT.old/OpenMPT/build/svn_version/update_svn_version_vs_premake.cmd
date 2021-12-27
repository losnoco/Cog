@echo off
set INTDIR=%1%
if not exist %INTDIR% mkdir %INTDIR%
if not exist %INTDIR%\svn_version mkdir %INTDIR%\svn_version
subwcrev ..\.. ..\..\build\svn_version\svn_version.template.subwcrev.h %INTDIR%\svn_version\svn_version.h || del %INTDIR%\svn_version\svn_version.h || exit 0
exit 0
