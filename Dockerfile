FROM ubuntu:20.04

RUN apt update && apt install -y libncurses5-dev python2.7-dev build-essential automake autoconf libtool m4
RUN mkdir -p /build
COPY . /build/bnirc
WORKDIR /build/bnirc
RUN ./autogen.sh --prefix=/usr && make install

RUN useradd -m -s /bin/bash bnirc
USER bnirc

ENTRYPOINT /usr/bin/bnirc
