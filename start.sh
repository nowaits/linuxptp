#!/bin/bash
set -e

ROOT_DIR=`realpath $(dirname $0)`
cd $ROOT_DIR

usage() {
    cat <<EOF
Usage: ${0##*/} OPTIONS

OPTIONS:
  -g/--gdb          gdb mode
                    accel-ppp will start with gdb
  -s/--server       server mode
  -c/--client       client mode
  -2                IEEE 802.3
  -4                UDP IPv4(default)
  -6                UDP IPv6
  -l/--log-level    log level(0-7)
                        default 7
  -h/--help         help
EOF
    exit 1
}

function check_veth() {
if [ ! -d /sys/class/net/veth$1 ]; then
    su <<EOF
    ip link del veth$1 >/dev/null 2>&1;
    ip link add veth$1 type veth peer name veth$2
    ip addr add 3.0.0.1/24 dev veth$1
    ip addr add 3.0.0.2/24 dev veth$2
    ip link set veth$1 up
    ip link set veth$2 up
EOF
fi
}

IF=eth0
LOG_LEVEL=7
GDB=
APP=$ROOT_DIR/ptp4l
PROTO="-4"
CLIENT=
ARGS="-m -S -l $LOG $PROTO -i $IF"

for i in "$@"
do
case $i in
    -h|--help)
        usage
    shift
    ;;
    -s|--server)
        CLIENT=
    shift
    ;;
    -c|--client)
        CLIENT="-s"
    shift
    ;;
    -2)
        PROTO="-2"
    shift
    ;;
    -4)
        PROTO="-4"
    shift
    ;;
    -6)
        PROTO="-6"
    shift
    ;;
    -l=*|--log-level=*)
        LOG_LEVEL="${i#*=}"
    shift
    ;;
    -g|--gdb)
        GDB=ON
    shift
    ;;
    *)
        echo -e "****** Unsupport option:[$i] ******"
        usage
    ;;
esac
done

ARGS="-m -S $CLIENT -l $LOG_LEVEL $PROTO -i $IF"

check_veth 0 1
if [ ! -z $GDB ]; then
GDB_FILE=/tmp/gdb-$HASH_ID.conf

cat <<EOF>$GDB_FILE
set breakpoint pending on
set logging overwrite on
set logging off
handle SIGPIPE SIGUSR1 noprint nostop
set detach-on-fork on
set follow-fork-mode parent
set print inferior-events
set print inferior-events on
set print inferior-events off
set print thread-events off
set args $ARGS
b exit
b abort
b sigsegv
EOF

echo "r" >> $GDB_FILE;

gdb \
    -iex "set confirm off" \
    -iex "set pagination off" \
    -iex "set print pretty on" \
    -iex "set auto-load safe-path /" \
    -x $GDB_FILE $APP
else
$APP $ARGS
fi
