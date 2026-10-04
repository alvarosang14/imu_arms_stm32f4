FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib \
    binutils-arm-none-eabi cmake ninja-build make git ca-certificates \
    && rm -rf /var/lib/apt/lists/*