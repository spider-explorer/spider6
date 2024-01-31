@echo off
@if "%1"=="" (
  echo Please specify tag name.
  exit /b 1

)
set tag=%1
echo tag=%tag%

git tag -d %tag%
git push origin :%tag%
