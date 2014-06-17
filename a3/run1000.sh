#!/bin/bash
for x in {2..1000}
do
	./pfact $x 2>/dev/null | grep "is prime"
done
