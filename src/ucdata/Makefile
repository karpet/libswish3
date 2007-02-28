#
# Copyright 2005 Computing Research Labs, New Mexico State University
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
# OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
# THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# $Id: Makefile,v 1.2 2005/03/22 00:04:21 mleisher Exp $
#

CC = gcc
CFLAGS = -g -Wall

#
# Just build the data compiler.
#
all: ucgendat

#
# Build all test programs as well as the data compiler.
#
#all: ucgendat testutf8bm testpgba testucdata

#
# Test executable.
#
testucdata: ucdata.c
	$(CC) $(CFLAGS) -DTEST -o testucdata $^

#
# Test executable.
#
testutf8bm: utf8bm.c ucdata.c
	$(CC) $(CFLAGS) -DBMTEST -o testutf8bm $^

#
# Test executable.
#
testpgba: testpgba.c ucdata.c ucpgba.c
	$(CC) $(CFLAGS) -o testpgba $^

#
# The data generation program.
#
ucgendat: ucgendat.c
	$(CC) $(CFLAGS) -o ucgendat ucgendat.c

clean:
	/bin/rm -f *.o *.BAK *.CKP *~

realclean: clean
	/bin/rm -f ucgendat testucdata testpgba testutf8bm
