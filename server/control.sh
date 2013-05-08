#!/bin/sh

. /etc/init.d/functions

killall pathload_rcv;

FIVE_MIN=30;
while [ 1 ];
do
	killall pathload_rcv;
	nohup ./pathload_rcv 2>/dev/null &
	pid=$!;
	while [ 1 ];
	do
			#if [ -z `pgrep -x pathload_rcv` ]; then
			status pathload_rcv
			ret=$?
			if [ "$ret" -ne 0 ]; then
				break;
			fi
			sleep $FIVE_MIN;
	done
done

