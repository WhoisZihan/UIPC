#!/bin/bash

# reference is https://stackoverflow.com/questions/10909685/run-parallel-multiple-commands-at-once-in-the-same-terminal

START_CORE=3
MAX_CORE=7
STEP_SIZE=2

for i in `seq $START_CORE 2 $MAX_CORE`
do
	./uipc_user -e $i & pid=$!
	PID_LIST+=" $pid";
done

trap "kill $PID_LIST" SIGINT

echo "Parallel processes have started";

wait $PID_LIST

echo
echo "All processes have completed";

