#!/bin/bash

name=tracy
comp=gcc
src=src/*.c
std='-std=c99'

flags=(
    -Wall
    -Wextra
    -O3
)

inc=(
    -Iinclude/
    -Iimgtool/
    -Ifract/
    -Iutopia/
    -Imass/
    -Iphoton/
    -I.
)

lib=(
    -Llib/
    -limgtool
    -lfract
    -lutopia
    -lmass
    -lphoton
    -lz
    -lpng
    -ljpeg
)

mac=(
    #-mmacosx-version-min=10.10
)

linux=(
    -lm
    -lpthread
    -D_POSIX_C_SOURCE=199309L
)

fail() {
    echo "Use with -comp to compile or -run to compile and execute"
    exit
}

lib_build() {
    pushd $1/
    ./build.sh $2
    popd 
    mv $1/lib$1.a lib/lib$1.a
}

build() {
    mkdir lib/
    lib_build utopia -slib
    lib_build fract -s
    lib_build imgtool -slib
    lib_build mass -s
    lib_build photon -s
}

comp() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        $comp $src -o $name $std ${flags[*]} ${mac[*]} ${inc[*]} ${lib[*]}
    elif echo "$OSTYPE" | grep -q "linux"; then
        $comp $src -o $name $std ${flags[*]} ${inc[*]} ${lib[*]} ${linux[*]}
    else
        echo "OS not supported yet"
        exit
    fi
}

clean() {
    rm -r lib/
    rm $name
}

if [[ $# < 1 ]]; then
    fail
elif [[ "$1" == "-cpp" ]]; then
    shift
    echo "C++"
    std='-std=c++17'
    comp=g++
    src=*.cpp
fi

if [[ "$1" == "-build" ]]; then
    build
    exit
elif [[ "$1" == "-comp" ]]; then
    comp
    exit
elif [[ "$1" == "-run" ]]; then
    shift
    comp && ./$name "$@"
    exit
elif [[ "$1" == "-clean" ]]; then
    clean
    exit
elif [[ "$1" == "-all" ]]; then
    shift
    build && comp && ./$name "$@"
    exit
else 
    fail
fi
