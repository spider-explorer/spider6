@echo off
git fetch
git diff --summary origin/HEAD
git status --short
