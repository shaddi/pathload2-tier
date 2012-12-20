#!/bin/sh

killall server_select;

INTERVAL=30;
while [ 1 ];
do
	nohup ./server_select &
	pid=$!;
	while [ 1 ];
	do
			if [ -z `pgrep -x server_select` ]; then
				break;
			elif [ `pgrep -x server_select` != $pid ]; then
				break;
			fi
			sleep $INTERVAL;
	done
done

