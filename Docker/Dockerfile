FROM ubuntu:21.04

###### install SEAL dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential clang-tools-9 libmsgsl-dev zlib1g-dev cmake

###### build and install SEAL
RUN git clone --branch 3.6.5 https://github.com/microsoft/SEAL.git && \
    cd SEAL; \
    cmake -S . -B build; \
    cmake --build build; \
    cmake --install build

###### install other ABC dependencies
RUN apt-get update; \
    apt-get -y install nlohmann-json3-dev

###### clone the ABC repository
RUN git clone https://github.com/MarbleHE/ABC.git \
    # TODO: remove these lines after debugging
    && cd ABC \
    && git checkout ec2-automation

###### build ABC
RUN cd ABC; \
    mkdir build; \
    cd build; \
    cmake ..; \
    make -j$(nproc)

###### run the passed benchmark
ENTRYPOINT ["/ABC/build/ast_demo", "-bench", "$1"]
