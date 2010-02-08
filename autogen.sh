#!/bin/sh

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

rm -rf autom4te.cache
rm -f aclocal.m4 ltmain.sh

touch README

echo "Running autopoint..." ; autopoint -f || exit 1
echo "Running aclocal..." ; aclocal $ACLOCAL_FLAGS -I m4 || exit 1
echo "Running autoheader..." ; autoheader || exit 1
echo "Running autoconf..." ; autoconf || exit 1
echo "Running libtoolize..." ; (libtoolize --copy --automake || glibtoolize --automake) || exit 1
echo "Running automake..." ; automake --add-missing --copy --gnu || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
