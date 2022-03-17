#!/bin/sh

echo 176x144_7.5                                                     >dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_7.5	 32       176 144 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_7.5	 40       176 144 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_7.5	 48       176 144 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_7.5	 56       176 144 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_7.5	 64       176 144 7.5	2>>dat/FOREMAN.txt
					
echo 176x144_15                                                  >>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_15	 48       176 144 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_15	 56       176 144 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_15	 64       176 144 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_15	 80       176 144 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	176x144_15	 96       176 144 15	2>>dat/FOREMAN.txt
					
echo 352x288_7.5                                                  >>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_7.5	 64       352 288 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_7.5	 80       352 288 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_7.5	 96       352 288 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_7.5	 112      352 288 7.5	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_7.5	 128      352 288 7.5	2>>dat/FOREMAN.txt
					
echo 352x288_15                                                   >>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_15	 96    	  352 288 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_15	 112      352 288 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_15	 128      352 288 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_15	 160      352 288 15	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_15	 192      352 288 15	2>>dat/FOREMAN.txt
					
echo 352x288_30                                                   >>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_30	 128      352 288 30	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_30	 160      352 288 30	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_30	 192      352 288 30	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_30	 224      352 288 30	2>>dat/FOREMAN.txt
batch/decode.sh	FOREMAN	352x288_30	 256      352 288 30	2>>dat/FOREMAN.txt


