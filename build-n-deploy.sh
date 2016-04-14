#!/bin/sh

#SDK=/opt/windriver/wrlinux/8.0-axxiaarm64-ml
#SDK=/opt/windriver/wrlinux/8.0-intel-x86-64
SDK=/opt/windriver/wrlinux/6.0-axm5516-rcpl26

APP=$(basename $PWD)

make clean
(. $SDK/env.sh; make)
file ${APP}

sshpass -p root scp udp-rxtx amarillo-1:
sshpass -p root scp udp-rxtx amarillo-2:

