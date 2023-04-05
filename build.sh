#!/bin/bash

name=tracy

cc=gcc
src=src/*.c
cli=cli.c
rt=rt.c

flags=(
    -Wall
    -Wextra
    -pedantic
    -O2
    -std=c99
)

sub=(
    imgtool
    fract
    mass
    utopia
    photon
)

inc=(
    -I.
    -Ispxe
)

lib=(
    -Llib
    -lz
    -lpng
    -ljpeg
)

opngl=(
    -lglfw
)

for mod in ${sub[*]}
do
    inc+=(-I$mod)
    lib+=(-l$mod)
done

if echo "$OSTYPE" | grep -q "darwin"; then
    opngl+=(
        -framework OpenGL
    )
elif echo "$OSTYPE" | grep -q "linux"; then
    opngl+=(
        -lGL
        -lGLEW
    )
    lib+=(
        -lm
        -lpthread
    )
else
    echo "This OS is not supported by this shell script yet..." && exit
fi

cmd() {
    echo "$@" && $@
}

lib_build() {
    cmd pushd $1/ && ./build.sh $2 && cmd mv bin/*.a ../lib/ && cmd popd
}

build() {
    cmd mkdir -p lib/
    for mod in ${sub[*]} 
    do
        lib_build $mod static
    done
}

objs() {
    [ ! -d lib/ ] && build
    cmd mkdir -p tmp/
    cmd $cc -c $src ${flags[*]} ${inc[*]} && cmd mv *.o tmp/
}

crt() {
    objs && cmd $cc tmp/*.o $rt -o $name ${flags[*]} ${lib[*]} ${inc[*]} ${opngl[*]}
}

ccli() {
    objs && cmd $cc tmp/*.o $cli -o tracy_cli ${flags[*]} ${inc[*]} ${lib[*]}
}

cleanf() {
    [ -f $1 ] && cmd rm $1
}

cleand() {
    [ -d $1 ] && cmd rm -r $1
}

cleanr() {
    cleand $1/tmp/
    cleand $1/bin/
}

clean() {
    for mod in ${sub[*]}
    do
        cleanr $mod
    done

    cleand lib
    cleand tmp
    cleanf $name
    cleanf tracy_cli
    return 0
}

case "$1" in
    "build")
        build;;
    "cli")
        ccli;;
    "rt")
        crt;;
    "all")
        crt && ccli;;
    "clean")
        clean;;
    *)
        echo "Run with 'cli' or 'rt' to compile runtime or client executables"
        echo "Use 'clean' to remove local builds.";;
esac
