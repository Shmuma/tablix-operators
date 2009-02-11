#!/bin/sh

tablix2 -n 2 -d 3 -i /home/shmuma/tmp/tablix/lib/tablix2/ room_avail.xml
tablix2_output efim result0.xml -i /home/shmuma/tmp/tablix/lib/tablix2/ > 1.html
