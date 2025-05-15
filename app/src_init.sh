#!/bin/sh

ROOT_DIR=`realpath $(dirname $0)`
cd $ROOT_DIR

filters_src="filter mave mmedian"
security_src="sad"
servo_src="linreg ntpshm nullf pi refclock_sock servo"
transp_src="sk raw transport udp udp6 uds"
ptp4l_src="bmc clock clockadj clockcheck config designated_fsm \
 e2e_tc fault fsm hash interface monitor msg phc \
 pmc_common port port_signaling pqueue print ptp4l p2p_tc rtnl \
 stats tc telecom tlv tsproc \
 unicast_client unicast_fsm unicast_service util version"

mkdir -p src/filters;
for f in $filters_src
do
    ln -f ../$f.c src/filters/$f.c
done

mkdir -p src/sec;
for f in $security_src
do
    ln -f ../$f.c src/sec/$f.c
done

mkdir -p src/servo
for f in $servo_src
do
    ln -f ../$f.c src/servo/$f.c
done

mkdir -p src/transp;
for f in $transp_src
do
    ln -f ../$f.c src/transp/$f.c
done

mkdir -p src/ptp4l;
for f in $ptp4l_src
do
    ln -f ../$f.c src/ptp4l/$f.c
done