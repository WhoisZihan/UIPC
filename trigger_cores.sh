#!/bin/bash

START_CORE=3
MAX_CORE=7
STEP_SIZE=2

for i in `seq $START_CORE 2 $MAX_CORE`
do
        ./uipc_user -t 1 -d $i
done
