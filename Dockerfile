# Stage 1: 编译
FROM alpine:3.19 AS builder

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
    apk add --no-cache \
    g++ make cmake \
    pkgconf \
    hiredis-dev \
    mysql-dev \
    jsoncpp-dev \
    linux-headers

WORKDIR /app

COPY muduo/ ./muduo/

RUN cd muduo && \
    mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make shortener_server -j$(nproc)

# Stage 2: 运行
FROM alpine:3.19

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
    apk add --no-cache \
    libstdc++ \
    hiredis \
    mariadb-connector-c \
    jsoncpp

COPY --from=builder /app/muduo/bin/shortener_server /app/shortener_server

WORKDIR /app
EXPOSE 8889

ENTRYPOINT ["./shortener_server"]
