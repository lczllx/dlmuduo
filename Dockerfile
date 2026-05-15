# Stage 1: 编译
FROM alpine:3.21 AS builder

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
    apk add --no-cache \
    g++ make cmake \
    pkgconf \
    hiredis-dev \
    mariadb-connector-c-dev \
    jsoncpp-dev \
    linux-headers

# Alpine 的 mariadb-connector-c-dev 只提供 libmariadb.so，
# CMakeLists.txt 查找的是 mysqlclient，建一条兼容软链接
RUN ln -sf libmariadb.so /usr/lib/libmysqlclient.so

WORKDIR /app

COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make shortener_server -j$(nproc)

# Stage 2: 运行
FROM alpine:3.21

RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
    apk add --no-cache \
    libstdc++ \
    hiredis \
    mariadb-connector-c \
    jsoncpp \
    busybox-extras

COPY --from=builder /app/bin/shortener_server /app/shortener_server
COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

WORKDIR /app
EXPOSE 8889

ENTRYPOINT ["/docker-entrypoint.sh"]
