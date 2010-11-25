#!/bin/sh

TMPFILE=/tmp/$$.out

FCTS_ROOT=${FCTS_ROOT:-./fcts}
MZN=${MZN:-./minicsp-fz_debug}
CANON=${CANON:-./solns2dzn -c}

test()
{
    DIR=$1
    for i in `find $FCTS_ROOT/$DIR -name "*.fzn"`; do
	rm -f $TMPFILE
	FDIR=`dirname $i`
	OPT=$FDIR/`basename $i .fzn`.opt
	EXP=$FDIR/`basename $i .fzn`.exp
	$MZN `cat $OPT` $i | $CANON 2>&1 > $TMPFILE
	if diff -uw $TMPFILE $EXP 2>&1 > /dev/null; then
	    echo $i pass
	else
	    echo $i failed
	    diff -uw $TMPFILE $EXP
	fi
    done
}

if test $# -eq 0; then
    test builtins/int
else
    while test $# -gt 0
    do
	DIR=$1
	shift
	test $DIR
    done
fi
