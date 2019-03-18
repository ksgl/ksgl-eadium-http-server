FROM ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive

WORKDIR /highload
COPY . .

USER root

RUN apt-get -qq update && apt-get install -q -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get -qq update && apt-get install -qy build-essential g++ gcc git wget cmake libboost-all-dev

RUN rm -rf /tmp/boost-build && \
mkdir -p /tmp/boost-build && \
cd /tmp/boost-build && \
wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz && \
tar xvf boost_1_69_0.tar.gz && \
cd boost_1_69_0/ && \
./bootstrap.sh && \
./b2 toolset=gcc address-model=64 variant=release link=shared threading=multi runtime-debugging=off runtime-link=shared --with-atomic --with-date_time --with-chrono --with-filesystem --with-stacktrace --with-system --with-thread --build-dir=/tmp/boost-build-dir --prefix=/tmp/boost-prefix-dir install

RUN mkdir build && \
    cd build && \
    cmake -DBOOST_ROOT=/tmp/boost-prefix-dir -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

EXPOSE 80

CMD ["./build/src/highload"]
