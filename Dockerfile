FROM alpine:3.13 as build

RUN apk add build-base binutils autoconf automake libtool pkgconfig check-dev file patch git

COPY / /lolremez/
#RUN git clone https://github.com/samhocevar/lolremez.git

WORKDIR /lolremez/

RUN git submodule update --init --recursive

RUN ./bootstrap \
    && ./configure \
    && make

FROM alpine:3.13

RUN apk add libstdc++

COPY --from=build lolremez/lolremez /

ENTRYPOINT ["/lolremez"]