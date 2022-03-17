#!/bin/sh

echo 176x144_7.5                                                     >dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_7.5	128     176 144 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_7.5	160     176 144 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_7.5	192     176 144 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_7.5	224	176 144 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_7.5	256	176 144 7.5	2>>dat/FOOTBALL.txt

echo 176x144_15                                                     >>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_15	192     176 144 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_15	224	176 144 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_15	256	176 144 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_15	320	176 144 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	176x144_15	384	176 144 15	2>>dat/FOOTBALL.txt

echo 352x288_7.5                                                     >>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_7.5	256	352 288 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_7.5	320	352 288 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_7.5	384	352 288 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_7.5	448	352 288 7.5	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_7.5	512	352 288 7.5	2>>dat/FOOTBALL.txt

echo 352x288_15                                                      >>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_15	384	352 288 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_15	448	352 288 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_15	512	352 288 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_15	640	352 288 15	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_15	768	352 288 15	2>>dat/FOOTBALL.txt

echo 352x288_30                                                      >>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_30	512	352 288 30	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_30	640	352 288 30	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_30	768	352 288 30	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_30	896	352 288 30	2>>dat/FOOTBALL.txt
batch/decode.sh	FOOTBALL	352x288_30	1024	352 288 30	2>>dat/FOOTBALL.txt
