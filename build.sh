#!/bin/bash

name=tracy
cc=gcc
src=src/*.c

flags=(
    -Wall
    -Wextra
    -O2
    -std=c99
)

inc=(
    -I.
    -Iimgtool
    -Ifract
    -Iutopia
    -Imass
    -Iphoton
)

lib=(
    -Llib
    -limgtool
    -lfract
    -lmass
    -lutopia
    -lphoton
    -lz
    -lpng
    -ljpeg
)

linux=(
    -lm
    -lpthread
    -D_POSIX_C_SOURCE=199309L
)

lib_build() {
    pushd $1/ && ./build.sh $2 && mv *.a ../lib/ && popd
}

build() {
    [ ! -d lib ] && mkdir lib && echo "mkdir lib"
    
    lib_build utopia static
    lib_build fract static
    lib_build imgtool static
    lib_build mass static
    lib_build photon static
}

comp() {
    if echo "$OSTYPE" | grep -q "darwin"; then
        $cc $src -o $name $std ${flags[*]} ${inc[*]} ${lib[*]}
    elif echo "$OSTYPE" | grep -q "linux"; then
        $cc $src -o $name $std ${flags[*]} ${inc[*]} ${lib[*]} ${linux[*]}
    else
        echo "This OS not supported yet" && exit
    fi
}

cleanf() {
    [ -f $1 ] && rm $1 && echo "deleted $1"
}

cleand() {
    [ -d $1 ] && rm -r $1 && echo "deleted $1"
}

clean() {
    cleand lib
    cleanf $name
    return 0
}

install() {
    [ "$EUID" -ne 0 ] && echo "Run with sudo to install" && exit

    build && comp
    [ -f $name ] && mv $name /usr/local/bin/

    echo "Successfully installed $name"
    return 0
}

uninstall() {
    [ "$EUID" -ne 0 ] && echo "Run with sudo to uninstall" && exit

    cleanf /usr/local/bin/$name

    echo "Successfully uninstalled $name"
    return 0
}

case "$1" in
    "build")
        build;;
    "comp")
        comp;;
    "clean")
        clean;;
    "all")
        build && comp;;
    "install")
        install;;
    "uninstall")
        uninstall;;
    *)
        echo "Run with 'build', 'comp' or 'all' to build."
        echo "Use 'install' to build and install in /usr/local"
        echo "Use 'clean' to remove local builds.";;
esac
