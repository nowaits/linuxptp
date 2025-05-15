#!/bin/bash
set -e

WORK_DIR=`realpath $(dirname $0)`
cd $WORK_DIR

usage() {
    cat <<EOF
Usage: ${0##*/} OPTIONS
OPTIONS:
    -a|--all)           build all
    -k=*|--ktime=*)     ktime install dir
    -s=*|--server=*)    server name
                            default: s
    -r                  release build
                            default: debug
EOF
    exit 1
}

D=`basename $WORK_DIR`
function output_finish()
{
    w=`stty size|awk '{print $2}'`
    yes = | sed ${w}q | tr -d '\n'
}

ALL=
TARGET=debug
ROOT_DIR=~/test/linuxptp

for i in "$@"
do
case $i in
    -a)
        ALL=yes
    shift
    ;;
    -s=*|--server=*)
        SERVERS="${i#*=}"
    shift
    ;;
    -k=*|--ktime=*)
        export KTIME_INSTALL="${i#*=}"
    shift
    ;;
    -r)
        TARGET=release
    shift
    ;;
    *)
        usage
    ;;
esac
done

if [ ! -z "$ALL" ]; then
    make clean;
fi

make BUILD_TYPE=$TARGET install;

for S in ${SERVERS//","/ }
do
    sp=(${S//":"/ })
    SERVER=${sp[0]}
    PORT=${sp[1]}
    if [ -z $PORT ]; then
        PORT=22
    fi

    echo "-- $SERVER:$PORT Deploy start!";
    ssh -p $PORT root@$SERVER "mkdir -p $ROOT_DIR;killall -9 ptp4l >/dev/null 2>&1;" || true
    scp -P $PORT -r install/* root@$SERVER:$ROOT_DIR

    echo "-- ********* Deploy finish! *************";
done