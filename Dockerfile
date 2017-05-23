FROM ubuntu:16.04
MAINTAINER Alex Gandy <alexgandy@gmail.com>

RUN apt-get update \
  && apt-get install -yq --no-install-recommends \
  libssl-dev \
  libjansson-dev \
  libcurl4-openssl-dev \
  libsqlite3-dev \
  sendemail \
  make \
  gcc \
  g++ \
  && rm -rf /var/lib/apt/lists/*

COPY . /blackbird

WORKDIR /blackbird

RUN make -B
