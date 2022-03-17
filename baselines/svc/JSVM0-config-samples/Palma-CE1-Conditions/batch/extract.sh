#!/bin/sh
# (1:inseq) (2:outseq) (3-9:parameters)

INSEQ=$1
OUTSEQ=$2
shift 2
bin/BitStreamExtractorStatic str/$INSEQ.264 str/$OUTSEQ.264 $@
