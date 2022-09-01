#!/bin/bash

# In this configuration, the following dependent libraries are compiled:
#
# * zlib
# * c-ares
# * openSSL
# * libssh2

sudo yum install -y zlib-devel c-ares openssl-devel libssh2-devel

# Compiler and path
PREFIX=$PWD/aria2-lib
C_COMPILER="gcc"
CXX_COMPILER="g++"

DOWNLOADER="wget -c"

echo "Remove old libs..."
rm -rf ${PREFIX}
rm -rf _obj

## Version
ARIA2_V=1.36.0

## Dependencies
ARIA2=https://github.com/aria2/aria2/releases/download/release-${ARIA2_V}/aria2-${ARIA2_V}.tar.bz2

## Config
BUILD_DIRECTORY=/tmp/

## Build
cd ${BUILD_DIRECTORY}

# build aria2 static library
if ! [[ -e aria2-${ARIA2_V}.tar.bz2 ]]; then
    ${DOWNLOADER} ${ARIA2}
fi
tar jxvf aria2-${ARIA2_V}.tar.bz2
cd aria2-${ARIA2_V}/
PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig/ \
    LD_LIBRARY_PATH=${PREFIX}/lib/ \
    CC="$C_COMPILER" \
    CXX="$CXX_COMPILER" \
    CXXFLAGS="-fPIC" \
    CFLAGS="-fPIC" \
    ./configure \
    --prefix=${PREFIX} \
    --without-sqlite3 \
    --without-libxml2 \
    --without-libexpat \
    --without-libgcrypt \
    --with-openssl \
    --without-libnettle \
    --without-gnutls \
    --without-libgmp \
    --with-libssh2 \
    --enable-libaria2 \
    --enable-shared=no \
    --enable-static=yes
make -j`nproc`
make install
cd ..

# cleaning
rm -rf aria2-${ARIA2_V}
rm -rf ${PREFIX}/bin

# generate files for c
cd ${PREFIX}/../
go tool cgo libaria2.go

echo "Prepare finished!"
