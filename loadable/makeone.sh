#!/bin/sh
CRT="crt_fun.o"
ARG="argroom.o envroom.o"
CC="slc -nostdlib -fPIC -fpic"
CFLAGS=`../gcflags`

NA=$1
$CC $CFLAGS -b mta $NA.c $CRT $ARG -Wl,-shared -o $NA'_arg_shared'
$CC $CFLAGS -b mta $NA.c $CRT $ARG -o $NA'_arg'
$CC $CFLAGS -b mta $NA.c $CRT -Wl,-shared -o $NA'_shared'
$CC $CFLAGS -b mta $NA.c $CRT -o $NA

