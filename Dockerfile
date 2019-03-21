FROM eadium/boost-1.69

WORKDIR /highload
COPY . .

USER root

RUN mkdir build && \
    cd build && \
    cmake -DBOOST_ROOT=/tmp/boost-prefix-dir -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

EXPOSE 80

CMD ["./build/src/highload"]
