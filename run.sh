#!/bin/bash
# TEST_WRAPPER="gdb -ex run --args"
# TEST_WRAPPER="valgrind --leak-check=full"
# TEST_WRAPPER="valgrind --tool=cachegrind"
ROOT=`pwd`
ACTION=$1

if [ ! -f /.dockerenv ]; then
        if [[ "$(docker images -q apteryx-kermond 2> /dev/null)" == "" ]]; then
                docker build -t apteryx-kermond .
                rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
        fi
        docker rm apteryx-kermond
        docker run --privileged -it --user manager -v $ROOT/..:/home --env-file <(env | grep TEST_) --name apteryx-kermond apteryx-kermond ./run.sh $ACTION
        exit 0
fi

# Build needed packages
BUILD=$ROOT/.build
mkdir -p $BUILD
cd $BUILD

# Generic cleanup
function quit {
        RC=$1
        # Stop apteryx-rest
        killall apteryx-kermond &> /dev/null
        kill `pidof valgrind.bin` &> /dev/null
        # Stop Apteryx
        killall -9 apteryxd &> /dev/null
        rm -f /tmp/apteryx
        exit $RC
}

# Check required libraries and tools
if ! pkg-config --exists glib-2.0 libxml-2.0 cunit; then
        echo "Please install glib-2.0, libxml-2.0, and cunit"
        echo "(sudo apt-get install build-essential libglib2.0-dev libxml2-dev libcunit1-dev)"
        exit 1
fi

# Check Apteryx install
if [ ! -d apteryx ]; then
        echo "Downloading Apteryx"
        git clone --depth 1 https://github.com/alliedtelesis/apteryx.git
        rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
fi
if [ ! -f $BUILD/usr/lib/libapteryx.so ]; then
        echo "Building Apteryx"
        cd apteryx
        make install DESTDIR=$BUILD
        rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
        cd $BUILD
fi

# Check Apteryx XML Schema library
if [ ! -d apteryx-xml ]; then
        echo "Downloading apteryx-xml"
        git clone --depth 1 https://github.com/alliedtelesis/apteryx-xml.git
        rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
fi
if [ ! -f $BUILD/usr/lib/libapteryx-schema.so ]; then
        echo "Building apteryx-xml"
        cd apteryx-xml
        rm -f $BUILD/usr/lib/libapteryx-xml.so
        rm -f $BUILD/usr/lib/libapteryx-schema.so
        export EXTRA_CFLAGS="-fprofile-arcs -ftest-coverage"
        export EXTRA_LDFLAGS="-fprofile-arcs -ftest-coverage"
        make install DESTDIR=$BUILD APTERYX_PATH=$BUILD/apteryx
        rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
        cd $BUILD
fi

# Build
echo "Building apteryx-kermond"
if [ ! -f $BUILD/../Makefile ]; then
        export CFLAGS="-g -Wall -Werror -I$BUILD/usr/include -fprofile-arcs -ftest-coverage"
        export LDFLAGS=-L$BUILD/usr/lib
        export PKG_CONFIG_PATH=$BUILD/usr/lib/pkgconfig
        cd $BUILD/../
        ./autogen.sh
        ./configure --with-pyang="pyang --plugindir $BUILD/apteryx-xml/ -p $BUILD/../models"
        rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
        cd $BUILD
fi
make -C $BUILD/../
rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi

# Check tests
echo Checking pytest coding style ...
flake8 --max-line-length=180 ../tests/test*.py
rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi

# Start Apteryx and populate the database
export LD_LIBRARY_PATH=$BUILD/usr/lib
rm -f /tmp/apteryx
$BUILD/usr/bin/apteryxd -b
rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi

# Parameters
if [ "$ACTION" == "test" ]; then
        PARAM="-b"
else
        PARAM="-v"
fi

# Start apteryx-kermond in test namespace
sudo G_SLICE=always-malloc LD_LIBRARY_PATH=$BUILD/usr/lib \
        $TEST_WRAPPER ../apteryx-kermond $PARAM -p apteryx-kermond.pid
rc=$?; if [[ $rc != 0 ]]; then quit $rc; fi
sleep 0.5
cd $BUILD/../

if [ "$ACTION" == "test" ]; then
        sleep 1
        python3 -m pytest -vv
        killall apteryx-kermond &> /dev/null
        kill `pidof valgrind.bin` &> /dev/null
        sleep 1
fi

# Gcov
mkdir -p .gcov
find . -path ./.gcov -prune -o -name '*.gcno' -exec mv -f {} .gcov/ \;
find . -path ./.gcov -prune -o -name '*.gcda' -exec mv -f {} .gcov/ \;
lcov -q --capture --directory . --output-file .gcov/coverage.info &> /dev/null
genhtml -q .gcov/coverage.info --output-directory .gcov/

# Done - cleanup
quit 0
