#!/bin/sh

DEFAULT_SYSROOT="/usr"
COMPONENTS="ecore edje edje_editor edje_viewer eet embryo emotion esmart etk evas"
SVN_DIR="/tmp/E17-svn"
SVN_URL="http://svn.enlightenment.org/svn/e/trunk/"

[ -n "$1" ] && BUILD_DIR="$1" || BUILD_DIR=$DEFAULT_SYSROOT

mkdir -p "$SVN_DIR"
cd "$SVN_DIR"
for c in $COMPONENTS; do
  svn co $SVN_URL/$c
done

export PATH="$BUILD_DIR/bin:$PATH"
export CFLAGS="-I$BUILD_DIR/include"
export LDFLAGS="-L$BUILD_DIR/lib"
export LD_LIBRARY_PATH="$BUILD_DIR/lib"
export PKG_CONFIG_PATH="$BUILD_DIR/lib/pkgconfig"

cd eet
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd evas
./autogen.sh
./configure --prefix="$BUILD_DIR" \
            --with-x \
            --enable-gl-x11 \
            --enable-eet \
            --enable-software-xcb \
            --enable-xrender-xcb \
            --enable-software-16-x11 \
            --enable-fb
make install
cd ..

cd ecore
./autogen.sh
./configure --prefix="$BUILD_DIR" \
            --with-x \
            --enable-ecore-fb \
            --enable-ecore-x-xcb \
            --enable-ecore-evas-opengl-x11 \
            --enable-ecore-evas-software-16-x11 \
            --enable-ecore-evas-software-xcb \
            --enable-ecore-evas-xrender-xcb \
            --enable-ecore-evas
make install
cd ..

cd embryo
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd edje
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd etk
./autogen.sh
./configure --prefix="$BUILD_DIR" \
            --enable-ecore-fb-x11-support 
make install
cd ..

cd edje_viewer
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd edje_editor
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install DESTDIR="$BUILD_DIR"
cd ..

cd esmart
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd emotion
./autogen.sh
./configure --prefix="$BUILD_DIR"
make install
cd ..

cd -

rm -rf "$SVN_DIR"
