# from ubuntu:latest
# RUN apt update -y
# RUN apt install -y g++ wget
# RUN apt install -y software-properties-common lsb-release && apt clean all
# RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
# RUN apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
# RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6AF7F09730B3F0A4
# RUN apt update && apt install -y kitware-archive-keyring
# RUN apt update && apt install -y cmake pkg-config libcurl4-openssl-dev libssl-dev libprocps-dev libboost-all-dev  libmpfr-dev libgmp-dev
# RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh
# RUN bash ~/miniconda.sh -b -p $HOME/miniconda
# RUN $HOME/miniconda/bin/conda shell.bash hook
# RUN $HOME/miniconda/bin/conda init
# RUN $HOME/miniconda/bin/conda install -c conda-forge nlohmann_json
# RUN mkdir /app
# COPY depends /app/depends
# COPY include /app/include
# COPY src /app/src
# COPY setup /app/setup
# COPY CMakeLists.txt /app/CMakeLists.txt
# COPY entrypoint.sh /app/entrypoint.sh
# COPY HttpServer.py /app/HttpServer.py
# #RUN ls /app
# #RUN rm -rf /app/build
# #RUN rm -rf /app/cmake-build-debug
# RUN mkdir app/build && cd /app/build && export nlohmann_json_DIR=/root/miniconda && cmake .. && make setup
# ENTRYPOINT /app/entrypoint.sh

from ubuntu:22.04
RUN apt update -y
RUN apt install unzip
RUN apt install default-jre -y
RUN apt install default-jdk -y
# RUN apt install libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386 -y
COPY androidtools.zip .
RUN unzip androidtools.zip
# Folder Structure : android_sdk/cmdline-tools/latest/(all files here)
RUN mkdir /android_sdk
RUN mkdir /android_sdk/cmdline-tools
RUN mv /cmdline-tools /android_sdk/cmdline-tools/latest
RUN ./android_sdk/cmdline-tools/latest/bin/sdkmanager --list
RUN (echo "y") | ./android_sdk/cmdline-tools/latest/bin/sdkmanager platform-tools emulator
# x86_64 support ARM emulation on Google_APIS
RUN (echo "y") | ./android_sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-30" "system-images;android-30;google_apis;x86_64"
RUN (echo "") | ./android_sdk/cmdline-tools/latest/bin/avdmanager create avd -n "my_avd_30" -k "system-images;android-30;google_apis;x86_64"
RUN apt-get install libpulse0 -y
RUN apt-get install libxcursor-dev -y
RUN apt-get install libxdamage-dev -y
COPY app-debug.apk app-debug.apk
# RUN apt-get install cpu-checker -y
# RUN kvm-ok

# Setup Server
RUN mkdir /result
RUN mkdir /app
COPY HttpServer.py /app/HttpServer.py
COPY entrypoint.sh /app/entrypoint.sh
ENTRYPOINT /app/entrypoint.sh

# It is important to run docker using --device=/dev/kvm --privileged
# To produce verify uses
# ./android_sdk/platform-tools/adb shell am start -n anon.fidoac/.VerifyActivity --es ageLimit 20 --es zkproof <here> --es serverChallenge ASAwBA== --es verificationKey <here>


