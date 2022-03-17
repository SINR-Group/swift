#!/bin/sh

echo 176x144_15                                                      >dat/CITY.txt
batch/decode.sh	CITY	176x144_15	64      176 144 15	2>>dat/CITY.txt
batch/decode.sh	CITY	176x144_15	80 	176 144 15	2>>dat/CITY.txt
batch/decode.sh	CITY	176x144_15	96 	176 144 15	2>>dat/CITY.txt
batch/decode.sh	CITY	176x144_15	112	176 144 15	2>>dat/CITY.txt
batch/decode.sh	CITY	176x144_15	128	176 144 15	2>>dat/CITY.txt

echo 352x288_7.5                                                     >>dat/CITY.txt
batch/decode.sh	CITY	352x288_7.5	128	352 288 7.5	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_7.5	160	352 288 7.5	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_7.5	192	352 288 7.5	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_7.5	224	352 288 7.5	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_7.5	256	352 288 7.5	2>>dat/CITY.txt

echo 352x288_15                                                      >>dat/CITY.txt
batch/decode.sh	CITY	352x288_15	192	352 288 15	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_15	224	352 288 15	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_15	256	352 288 15	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_15	320	352 288 15	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_15	384	352 288 15	2>>dat/CITY.txt

echo 352x288_30                                                      >>dat/CITY.txt
batch/decode.sh	CITY	352x288_30	256	352 288 30	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_30	320	352 288 30	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_30	384	352 288 30	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_30	448	352 288 30	2>>dat/CITY.txt
batch/decode.sh	CITY	352x288_30	512	352 288 30	2>>dat/CITY.txt

echo 704x576_15                                                      >>dat/CITY.txt
batch/decode.sh	CITY	704x576_15	512     704 576   15       2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_15	640     704 576   15       2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_15	768     704 576   15       2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_15	896     704 576   15       2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_15	1024    704 576   15       2>>dat/CITY.txt
					           
echo 704x576_30                                                 >>dat/CITY.txt
batch/decode.sh	CITY	704x576_30	768     704 576   30	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_30	896     704 576   30	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_30	1024    704 576   30	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_30	1280    704 576   30	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_30	1536    704 576   30	2>>dat/CITY.txt
					           
echo 704x576_60                                                 >>dat/CITY.txt
batch/decode.sh	CITY	704x576_60	1024    704 576   60	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_60	1280    704 576   60	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_60	1536    704 576   60	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_60	1792    704 576   60	2>>dat/CITY.txt
batch/decode.sh	CITY	704x576_60	2048    704 576   60	2>>dat/CITY.txt


