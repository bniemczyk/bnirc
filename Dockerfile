FROM ubuntu:20.04

RUN apt update && apt install -y libncurses-dev python2.7-dev build-essential automake1.11 autoconf libtool m4
RUN mkdir -p /build
COPY . /build/bnirc
WORKDIR /build/bnirc
RUN aclocal && automake -a -c
RUN autoconf
RUN ./configure --prefix=/usr && make install
