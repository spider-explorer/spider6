@if "%1"=="" (
  echo Please specify module name.
  exit /b 1
)
git submodule deinit -f sub/%1
git rm -f %HOME%/sub/%1
rm -rf .git/modules/sub/%1
