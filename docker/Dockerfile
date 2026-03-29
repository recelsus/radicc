FROM alpine:3.21 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    pkgconf \
    ffmpeg-dev \
    curl-dev

WORKDIR /src
COPY . /src

RUN cmake -S /src -B /src/build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build /src/build --target radicc-server -j$(getconf _NPROCESSORS_ONLN)


FROM alpine:3.21 AS runtime

RUN apk add --no-cache \
    libstdc++ \
    ffmpeg \
    curl

WORKDIR /app

COPY --from=builder /src/build/radicc-server /usr/local/bin/radicc-server

EXPOSE 8080

ENTRYPOINT ["/usr/local/bin/radicc-server"]
CMD ["--host", "0.0.0.0", "--port", "8080"]
