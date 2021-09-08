#!/bin/bash

name=tracy
cc=gcc
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
    -Iglee/
    -Igleex/
    -I.
)

lib=(
    -Llib/
    -limgtool
    -lfract
    -lutopia
    -lmass
    -lphoton
    -lglee
    -lgleex
    -lz
    -lpng
    -ljpeg
    -lglfw
)

mac=(
    -framework OpenGL
    #-mmacosx-version-min=10.9
)

linux=(
    -lm
    -lpthread
    -lGL
    -lGLEW
    -D_POSIX_C_SOURCE=199309L
)

fail() {
    echo "Use with -comp to compile or -run to compile and execute"
    exit
}

lib_build() {
    pushd $1/ && ./build.sh $2 && mv *.a ../lib/ && popd
}

build() {
    mkdir lib/
    lib_build utopia -slib
    lib_build fract -s
    lib_build imgtool -slib
    lib_build mass -s
    lib_build photon -s
    lib_build glee -s
    lib_build gleex -s
}

comp() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        $cc $src -o $name $std ${flags[*]} ${mac[*]} ${inc[*]} ${lib[*]}
    elif echo "$OSTYPE" | grep -q "linux"; then
        $cc $src -o $name $std ${flags[*]} ${inc[*]} ${lib[*]} ${linux[*]}
    else
        echo "OS not supported yet" && exit
    fi
}

clean() {
    rm -r lib/ && rm $name
}

case "$1" in
    "-build")
        build;;
    "-comp")
        comp;;
    "-run")
        shift
        comp && ./$name "$@";;
    "-clean")
        clean;;
    "-all")
        shift
        build && comp && ./$name "$@";;
    *)
        fail;;
esac
