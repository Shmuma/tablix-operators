#!/bin/sh

rm -f result0.xml
tablix2 -t 1 -n 1 -i /home/shmuma/tmp/tablix/lib/tablix2/ test.xml
tablix2_output htmlcss result0.xml -i /home/shmuma/tmp/tablix/lib/tablix2/ > 1.html