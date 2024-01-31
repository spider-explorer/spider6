@echo off
@if "%1"=="" (
  echo Please specify tag name.
  exit /b 1

)
set tag=%1
set comment=%tag%
@if not "%2"=="" (
  set comment=%2
)
echo tag=%tag%
echo comment=%comment%

git tag -a %tag% -m%comment%
git push origin %tag%
