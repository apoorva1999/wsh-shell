FROM --platform=linux/amd64 ubuntu:22.04
# COPY xv6.tar.gz /
RUN apt-get update && apt-get install -y build-essential qemu qemu-system-x86 gdb git