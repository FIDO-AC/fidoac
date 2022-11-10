#!/bin/bash
./android_sdk/platform-tools/adb devices
./android_sdk/emulator/emulator -no-window -avd "my_avd_30" &
./android_sdk/platform-tools/adb  wait-for-device shell 'while [[ -z $(getprop dev.bootcomplete) ]] ; do sleep 1; done'
./android_sdk/platform-tools/adb install app-debug.apk 

# To send verification request (mocked 1st) - To initialize
# ./android_sdk/platform-tools/adb shell am start -n anon.fidoac/.VerifyActivity --es id 0 --es ageLimit 20 --es zkproof 1 --es verificationKey 1

# To query reuslt
# ./android_sdk/platform-tools/adb shell run-as anon.fidoac cat /data/data/anon.fidoac/shared_prefs/result_1.xml /result/result_1.xml

# To view logcat
# ./android_sdk/platform-tools/adb logcat -s "FIDO_AC_VerifyActivity"

# cd /app/build/setup && make && ./setup
python3 /app/HttpServer.py