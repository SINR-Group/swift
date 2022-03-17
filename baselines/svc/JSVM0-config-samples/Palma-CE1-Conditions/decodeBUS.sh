#!/bin/sh

echo 176x144_7.5                                                     >dat/BUS.txt
batch/decode.sh	BUS	176x144_7.5	64	176 144 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_7.5	80	176 144 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_7.5	96	176 144 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_7.5	112	176 144 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_7.5	128	176 144 7.5	2>>dat/BUS.txt

echo 176x144_15                                                     >>dat/BUS.txt
batch/decode.sh	BUS	176x144_15	96	176 144 15	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_15	112	176 144 15	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_15	128	176 144 15	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_15	160	176 144 15	2>>dat/BUS.txt
batch/decode.sh	BUS	176x144_15	192	176 144 15	2>>dat/BUS.txt

echo 352x288_7.5                                                     >>dat/BUS.txt
batch/decode.sh	BUS	352x288_7.5	128	352 288 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_7.5	160	352 288 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_7.5	192	352 288 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_7.5	224	352 288 7.5	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_7.5	256	352 288 7.5	2>>dat/BUS.txt

echo 352x288_15                                                      >>dat/BUS.txt
batch/decode.sh	BUS	352x288_15	192	352 288 15	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_15	224	352 288 15	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_15	256	352 288 15	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_15	320	352 288 15	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_15	384	352 288 15	2>>dat/BUS.txt

echo 352x288_30                                                      >>dat/BUS.txt
batch/decode.sh	BUS	352x288_30	256	352 288 30	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_30	320	352 288 30	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_30	384	352 288 30	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_30	448	352 288 30	2>>dat/BUS.txt
batch/decode.sh	BUS	352x288_30	512	352 288 30	2>>dat/BUS.txt
