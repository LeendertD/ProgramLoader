#!/bin/sh

RM='rm -f'

NA=$1
$RM $NA'_arg_shared'
$RM $NA'_arg'
$RM $NA'_shared'
$RM $NA

