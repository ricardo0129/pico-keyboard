FROM arm64v8/ubuntu:24.04

RUN apt update && \
    apt install -y ca-certificates git curl && \
    update-ca-certificates

# Combine RUN commands and clean up unnecessary files

RUN apt update && \
    apt upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
        cmake \
        python3 \
        build-essential \
        gcc-arm-none-eabi \
        libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib && \
    apt clean && \
    rm -rf /var/lib/apt/lists/*
