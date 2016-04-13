# Copyright (C) 2012 Ben Jencks <ben@bjencks.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# This -march is pretty safe since no archs lower than this have rdrand
CFLAGS = -std=gnu99 -march=core-avx-i -O3 -Wall -Werror
rngd-rdrand: main.o cpuid.o rdrand.o
	$(CC) $(LDFLAGS) -o $@ $^

install:
	install -D -m 0755 rngd-rdrand $(DESTDIR)/usr/bin/rngd-rdrand

deb:
	debuild -I -us -uc

clean:
	rm -f main.o rngd-rdrand
