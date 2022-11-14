FROM bitnami/minideb:latest as build

RUN install_packages \
    build-essential \
    binutils \
    autoconf \
    automake \
    libtool \
    pkg-config \
    file \
    git

COPY / /lolremez/

WORKDIR /lolremez/

RUN git submodule update --init --recursive

RUN ./bootstrap \
    && ./configure \
    && make

FROM bitnami/minideb:latest

COPY --from=build lolremez/lolremez /

ENTRYPOINT ["/lolremez"]