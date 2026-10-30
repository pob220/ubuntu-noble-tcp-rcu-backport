FROM docker.io/library/ubuntu@sha256:78d4744ead1f5b19b04d0df558ab182a5300903900755e515b2ca4d10eb2044d
ENV DEBIAN_FRONTEND=noninteractive
RUN sed -i 's/^Types: deb$/Types: deb deb-src/' /etc/apt/sources.list.d/ubuntu.sources \
 && apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates ubuntu-keyring gnupg build-essential fakeroot dpkg-dev devscripts debhelper dh-python bison flex libssl-dev libelf-dev libncurses-dev dwarves libudev-dev libcap-dev bc rsync cpio kmod zstd lz4 xz-utils python3 git initramfs-tools busybox-static curl \
 && apt-get build-dep -y linux=6.8.0-147.147
USER ubuntu
WORKDIR /work
