#!/bin/bash

# To send verification request (mocked 1st) - To initialize
# ./android_sdk/platform-tools/adb shell am start -n anon.fidoac/.VerifyActivity --es id 0 --es ageLimit 20 --es zkproof 1 --es verificationKey 1

# To query reuslt
# ./android_sdk/platform-tools/adb shell run-as anon.fidoac cat /data/data/anon.fidoac/shared_prefs/result_1.xml /result/result_1.xml

# cd /app/build/setup && make && ./setup
# python3 /app/HttpServer.py