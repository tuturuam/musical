#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra"
LIBS="-Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -latomic -DPLATFORM_DESKTOP"

clang $CFLAGS -o musical main.c $LIBS
