#!/usr/bin/bash

set -eu
cd "$(dirname "$0")"

# unpack arguements
for arg in "$@"; do declare $arg='1'; done

# select compiler if none specified
# prefers clang if installed
if [ ! -v gcc ] && [ ! -v clang ]; then
    if command -v clang &> /dev/null; then
        echo "[Auto-detected compiler: clang]"
        clang=1
    elif command -v gcc &> /dev/null; then
        echo "[Auto-detected compiler: gcc]"
        gcc=1
    else
        echo "Error: No suitable compiler found! Please install clang or gcc."
        exit 1
    fi
fi

# defaults
if [ ! -v release ];  then debug=1; fi
if [ -v debug ];      then echo "[debug mode]"; fi
if [ -v release ];    then echo "[release mode]"; fi
if [ -v clang ];      then compiler="${CC:-clang}"; echo "[clang compile]"; fi
if [ -v gcc ];        then compiler="${CC:-gcc}"; echo "[gcc compile]"; fi

# if no target set, build everything
if [ ! -v server ] && [ ! -v client ]; then
    server=1
    client=1
fi

# compile flags
common="-I../app -I../engine -D_GNU_SOURCE"

clang_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${common}"
clang_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${common}"
clang_link="-lpthread -lm"
clang_out="-o"

gcc_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${common}"
gcc_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${common}"
gcc_link="-lpthread -lm"
gcc_out="-o"

# compile/Link Lines
if [ -v gcc ];      then compile_debug="$gcc_debug"; fi
if [ -v gcc ];      then compile_release="$gcc_release"; fi
if [ -v gcc ];      then compile_link="$gcc_link"; fi
if [ -v gcc ];      then out="$gcc_out"; fi

if [ -v clang ];    then compile_debug="$clang_debug"; fi
if [ -v clang ];    then compile_release="$clang_release"; fi
if [ -v clang ];    then compile_link="$clang_link"; fi
if [ -v clang ];    then out="$clang_out"; fi

if [ -v debug ];    then compile="$compile_debug"; fi
if [ -v release ];  then compile="$compile_release"; fi

# directories
mkdir -p build

cd build

# compile
if [ -v server ]; then 
    echo "[building server...]"
    $compile \
        ../src/app/main.c \
        ../src/app/server.c \
        ../src/app/threadpool.c \
        ../src/engine/token.c \
        ../src/engine/parser.c \
    $compile_link $out csql
    echo "csql built"
fi

if [ -v client ]; then
    echo "[building client...]"
    $compile \
        ../src/repl/main.c \
    $compile_link $out repl
    echo "repl built"
fi

cd ..
