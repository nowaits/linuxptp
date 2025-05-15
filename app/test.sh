#!/bin/bash
set -e

WORK_DIR=`realpath $(dirname $0)`
cd $WORK_DIR

usage() {
    cat <<EOF
Usage: ${0##*/} OPTIONS
OPTIONS:
    -s=*|--server=*)    server name
                            default: s
    -l=*|--log=*)       log level
                            default: 6
    -c                  run as client
    -i=*                interface
                            default eth0
    -r                  release build
                            default: debug
EOF
    exit 1
}

ROOT_DIR=~/test/linuxptp
CLIENT=
INT=eth0
LOG_LEVEL=6

for i in "$@"
do
case $i in
    -s=*|--server=*)
        SERVER="${i#*=}"
    shift
    ;;
    -l=*|--log=*)
        LOG_LEVEL="${i#*=}"
    shift
    ;;
    -i=*)
        INT="${i#*=}"
    shift
    ;;
    -c)
        CLIENT="-c"
    ;;
    *)
        usage
    ;;
esac
done

if [ -z "$SERVER" ]; then
    echo "missing server"
fi

sp=(${SERVER//":"/ })
S=${sp[0]}
P=${sp[1]}
if [ -z $P ]; then
    P=22
fi

CMD="$ROOT_DIR/bin/start.sh -4 -i=$INT -l=$LOG_LEVEL $CLIENT -g";

ssh -p $P -t $S "$CMD"
