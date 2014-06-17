#!/bin/bash

for i in 1 2 3 4 5 6
do
        client1 localhost 51717 garbage$i>/dev/null 2>&1 &
done