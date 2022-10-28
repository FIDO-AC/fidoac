from ubuntu:latest
RUN apt update -y
RUN apt install -y g++ wget
RUN apt install -y software-properties-common lsb-release && apt clean all
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6AF7F09730B3F0A4
RUN apt update && apt install -y kitware-archive-keyring
RUN apt update && apt install -y cmake pkg-config libcurl4-openssl-dev libssl-dev libprocps-dev libboost-all-dev  libmpfr-dev libgmp-dev

RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh
RUN bash ~/miniconda.sh -b -p $HOME/miniconda
RUN $HOME/miniconda/bin/conda shell.bash hook
RUN $HOME/miniconda/bin/conda init
RUN $HOME/miniconda/bin/conda install -c conda-forge nlohmann_json


RUN mkdir /app
COPY depends /app/depends
COPY include /app/include
COPY src /app/src
COPY setup /app/setup
COPY CMakeLists.txt /app/CMakeLists.txt
COPY entrypoint.sh /app/entrypoint.sh
COPY HttpServer.py /app/HttpServer.py
#RUN ls /app
#RUN rm -rf /app/build
#RUN rm -rf /app/cmake-build-debug
RUN mkdir app/build && cd /app/build && export nlohmann_json_DIR=/root/miniconda && cmake .. && make setup
ENTRYPOINT /app/entrypoint.sh
