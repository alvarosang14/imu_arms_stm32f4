FROM debian:bookworm-slim

LABEL org.opencontainers.image.source=https://github.com/alvarosang14/GeometryRust
LABEL org.opencontainers.image.description="Stm32 Compile"
LABEL org.opencontainers.image.licenses=MIT

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib \
    binutils-arm-none-eabi cmake ninja-build make git ca-certificates \
    && rm -rf /var/lib/apt/lists/*