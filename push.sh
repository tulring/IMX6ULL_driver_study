#!/bin/bash
time=$(date "+%Y-%m-%d_%H:%M:%S")
git add .
git commit -m "$time"
git push origin master
