#! bash -uvx
set -uvx
set -e
cwd=`pwd`
gh auth login --hostname github.com --git-protocol https --web
killall boot5-x86_64-static || true
sleep 3
#cp -p boot5-x86_64-static.exe ~/../
rm -rf *.tmp
find .  -maxdepth 2 -iname *.h -o -iname *.cpp | xargs uncrustify -l CPP --replace --no-backup
#astyle --recursive --ascii --style=allman --suffix=none *.java,*.c,*.cpp,*.cxx,*.h,*.hpp,*.hxx | sed -e "/Unchanged  /d"
ts=`date "+%Y.%m.%d.%H.%M.%S"`
echo $ts > main/spider2.version.txt
UNAME=`uname`
mkdir -p upload.tmp/spider-release
7z a -tzip upload.tmp/spider-release/spider-v$ts.zip *-static.exe *-static.dll scoop-sw-list.ini scoop-bucket-map.json cmd temp -x!temp/*
sha256sum upload.tmp/spider-release/spider-v$ts.zip
sum1=`sha256sum upload.tmp/spider-release/spider-v$ts.zip | awk '{print $1}'`
#sum1=`sha256-x86_64-static.exe upload.tmp/spider-release/spider-v$ts.zip`
echo $sum1
cat << EOS > spider.json
{
    "version": "$ts",
    "description": "",
    "homepage": "",
    "license": "MIT",
    "depends": [
    ],
    "url": [
        "https://github.com/spider-explorer/spider5/releases/download/v$ts/spider-v$ts.zip"
    ],
    "hash": [
        "$sum1"
    ],
    "bin": [
        [
            "boot4-x86_64-static.exe",
            "spider2",
            "--dumy"
        ]
    ],
    "shortcuts": [
        [
            "boot5-x86_64-static.exe",
            "Spider2",
            "--dummy"
        ]
    ],
    "persist": "temp"
}
EOS
#echo $GITHUB_ALL | gh auth login --with-token
cp spider.json upload.tmp/spider-release/spider-v$ts.json
git add .
git commit -m"Spider Explorer v$ts"
git tag -a v$ts -mv$ts
git push origin v$ts
git push
gh release list | sed 's/|/ /' | awk '{print $1, $8}' | while read -r line; do gh release delete -y "$line"; done
gh release create v$ts "upload.tmp/spider-release/spider-v$ts.zip" --repo spider-explorer/spider5 --generate-notes --target main
