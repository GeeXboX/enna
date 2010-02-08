#!/bin/bash

# GeeXboX Enna Media Center.
# Copyright (C) 2005-2010 The Enna Project
#
# This file is part of Enna.
#
# Enna is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# Enna is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with Enna; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

packages_gb="libplayer libnfo libvalhalla enna"
src_path="$HOME/geexbox_src"
prefix="/usr"


if [ "$1" == "-u" ]; then
    echo -e "Update Mode"
elif [ "$1" == "-i" ]; then
    echo -e "Install mode"
else
    echo -e "Usage : $0 -i|-u"
    exit
fi

# actual working parts of the script (no need to really touch this)

# test sudo
echo "Check sudo access"
NOSUDO="no"
sudo ls /root || NOSUDO="yes"

if [ $NOSUDO == "yes" ]; then
    echo "You have no sudo access. You need this to install on the system."
    echo "Try add this line to your /etc/sudoers file:"
    echo ""
    echo $USER" ALL=(ALL) NOPASSWD: ALL"
    exit -1
fi

if [ "$NOSUDO" == "no" ]; then
  # detect distribution here
    F=`grep "hardy" "/etc/lsb-release"`
    if [ -n "$F" ]; then
        DISTRO="ubuntu-hardy"
    else
	F=`grep "intrepid" "/etc/lsb-release"`
	if [ -n "$F" ]; then
            DISTRO="ubuntu-intrepid"
	else
	    F=`grep "karmic" "/etc/lsb-release"`
	    if [ -n "$F" ]; then
	        DISTRO="ubuntu-karmic"
	    fi
	fi
    fi
fi

echo "Detected distribution: $DISTRO"
if [ "$DISTRO" == "ubuntu-hardy" ]; then
    sudo apt-get update
    sudo apt-get install \
        xterm make gcc bison flex subversion automake1.10 autoconf autotools-dev \
        autoconf-archive libtool gettext \
        libpam0g-dev libfreetype6-dev libpng12-dev zlib1g-dev libjpeg-dev \
        libtiff-dev libungif4-dev librsvg2-dev libx11-dev libxcursor-dev \
        libxrender-dev libxrandr-dev libxfixes-dev libxdamage-dev \
        libxcomposite-dev libxss-dev libxp-dev libxext-dev libxinerama-dev \
        libxft-dev libxfont-dev libxi-dev libxv-dev libxkbfile-dev \
        libxkbui-dev libxres-dev libxtst-dev libltdl3-dev libglu1-xorg-dev \
        libglut3-dev xserver-xephyr libdbus-1-dev cvs subversion mercurial \
        liblua5.1-dev libavformat-dev mplayer libxml2-dev \
        libcurl4-openssl-dev wget libexif-dev libsqlite3-dev
elif [ "$DISTRO" == "ubuntu-intrepid" ]; then
    #sudo apt-get update
    sudo apt-get install \
        xterm make gcc bison flex subversion automake1.10 autoconf autotools-dev \
        autoconf-archive libtool gettext \
        libpam0g-dev libfreetype6-dev libpng12-dev zlib1g-dev libjpeg62-dev \
        libtiff4-dev libungif4-dev librsvg2-dev libx11-dev libxcursor-dev \
        libxrender-dev libxrandr-dev libxfixes-dev libxdamage-dev \
        libxcomposite-dev libxss-dev libxp-dev libxext-dev libxinerama-dev \
        libxft-dev libxfont-dev libxi-dev libxv-dev libxkbfile-dev \
        libxkbui-dev libxres-dev libxtst-dev libltdl7-dev libglu1-xorg-dev \
        libglut3-dev xserver-xephyr libdbus-1-dev cvs subversion mercurial \
        liblua5.1-dev libavformat-dev mplayer libxml2-dev \
        libcurl4-openssl-dev wget libexif-dev libsqlite3-dev
elif [ "$DISTRO" == "ubuntu-karmic" ]; then
    #sudo apt-get update
    sudo apt-get install \
        xterm make gcc bison flex subversion automake autoconf autotools-dev \
        autoconf-archive libtool gettext \
        libpam0g-dev libfreetype6-dev libpng12-dev zlib1g-dev libjpeg62-dev \
        libtiff4-dev libungif4-dev librsvg2-dev libx11-dev libxcursor-dev \
        libxrender-dev libxrandr-dev libxfixes-dev libxdamage-dev \
        libxcomposite-dev libxss-dev libxp-dev libxext-dev libxinerama-dev \
        libxft-dev libxfont-dev libxi-dev libxv-dev libxkbfile-dev \
        libxkbui-dev libxres-dev libxtst-dev libltdl7-dev libglu1-xorg-dev \
        libglut3-dev xserver-xephyr libdbus-1-dev cvs subversion mercurial \
        liblua5.1-dev libavformat-dev mplayer libxine-dev libxml2-dev \
        libcurl4-openssl-dev wget libexif-dev libsqlite3-dev libxine1-all-plugins libxine1-ffmpeg
elif [ "$DISTRO" == "sample-distribution" ]; then
    echo "sample distro"
    # FIXME:  put in whatever commands are needed to get a good list of
    # dependencies for e and install themb - svn build tools etc. too. only
    # put what is needed - there are optional libs like libtiiff/gif etc.
    # but they hve no direct required to really suggested usefulness.
else
    echo "Your distribution is not supported. Please edit this script and"
    echo "add a section to detect your distribution and then to install"
    echo "required packages for it."
fi

if [ ! -e ./easy_e17.sh ]; then
    wget http://omicron.homeip.net/projects/easy_e17/easy_e17.sh ./easy_e17.sh
    chmod +x easy_e17.sh
fi


if [ "$1" == "-i" ]; then
    ./easy_e17.sh --instpath=$prefix --only=eina,eet,evas,ecore,efreet,e_dbus,embryo,edje,elementary,expedite
else
    ./easy_e17.sh --instpath=$prefix -u
fi

set -e

for package in $packages_gb; do
    if [ -d "$src_path/$package/.hg" ]; then
        echo -e "updating $package"
        cd "$src_path/$package"  2>&1 > /dev/null
        sudo make uninstall  2>&1 > /dev/null
        sudo make clean distclean  2>&1 > /dev/null
        hg pull -u  2>&1
    else
        echo -e "clone $package"
        mkdir -p "$src_path"  2>&1 > /dev/null
        cd "$src_path"  2>&1 > /dev/null
        hg clone "http://hg.geexbox.org/$package"  2>&1 > /dev/null
        cd "$src_path/$package"  2>&1 > /dev/null
    fi
    if [ "$package" == "enna" ]; then
        ./autogen.sh --prefix=$prefix 2>&1 > /dev/null
    else
        ./configure --prefix=$prefix 2>&1 > /dev/null
    fi
    make  2>&1 > /dev/null
    sudo make install 2>&1 > /dev/null
done

echo -e "Enna compilation succesfull :)"





