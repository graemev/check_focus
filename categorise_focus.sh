# !/bin/bash -ue

# Categorise photos by focus quality
# 

PROG=$0

set -ue 

usage() {

    cat <<-EOF
	USAGE: categorise [-v][--verbose] [-k key][--key key][-r][--rename] best [worst] < scorefile  > command-file

	-v or --verbose does the obvious thing
	-k key or --key key selects which key (column) is used to decide best/worst defaults to 2 (whole image from checkfocus)
	-r or --rename causes the output commands to rename the image file to prefix the chosen score (key) to filename, allows easy review in order

	<best> and <worst> are scores. 
	Images with values above <best> are categorised as good
	Images with values below <worst> are categorised as bad

	This script generates a set of shell commands to move the good images into BEST directory and bad images into WORST directory
	normally you would redirect the output into a file. Edit it if required, then execute it:

	find . -mtime -1       > filelist
	checkfocus  -f filelist > scorefile
	${PROG} 7300000 100000 < scorefile > command-file
	...possibly edit the command-file...
	bash ./command-file
EOF
}

typeset -i verbose 
typeset -i best
typeset -i worst
typeset -i keycolumn
typeset -i keyelement
typeset -i n
typeset -i col
typeset -i lineno

typeset -a line


PROG=$0
verbose=0
rename=0
keycolumn=2

: ${best:=0}
: ${worst:=0}



TEMP=`getopt -o vrk: --long verbose,rename,key: -n ${PROG} -- "$@"`

if [ $? != 0 ] ; then echo "Getopt failed, terminating..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

while true ; do
	case "$1" in
		-v|--verbose)       verbose=1      ; shift   ;;
		-r|--rename)        rename=1       ; shift   ;;
		-k|--key)           keycolumn="$2" ; shift 2 ;;
		--) shift ; break ;;
		*) echo "Internal error!" ; exit 1 ;;
	esac
done

if [ $# -lt 1 -o  $# -gt 2 ] ; then
  usage
  exit 1
fi

best=$1

if [ $# -gt 1 ] ; then
    worst=$2
fi


if [ ${verbose} -eq 1 ] ; then
    echo "verbose selected"             >&2
    echo "key=${keycolumn}"             >&2
    echo "best=${best}"                 >&2
    echo "worst=${0}"                   >&2
fi

if [[ ${keycolumn} -lt 2 ]] ; then
    echo "Error key must be 2 or greater (column 1 is the filename)" >&2
    exit 1
fi

keyelement=keycolumn-1

if [[ ${best} -ne 0 ]] ; then
    echo "mkdir -p BEST"
fi

if [[ ${worst} -ne 0 ]] ; then
    echo "mkdir -p WORST"
fi

lineno=0

while read -a line
do
    lineno+=1
    
    if [[ verbose -ne 0 ]] ; then
	n=${#line[*]}
	echo -n "${n} items on line ${lineno} " >&2
	echo -n "Filename: ${line[0]} "         >&2
	echo -n "Fields: "                      >&2

	for ((i=1; i<n; ++i))
	do
	    col=i+1
	    echo -n "k[${col}]=${line[i]},"    >&2  # sort is 1 based, arrrays are 0 based
	done
	echo                                   >&2

	if [[ ${best} -ne 0 ]] ; then
	    if [[ line[keyelement] -gt best ]] ; then
		if [[ ${rename} -eq 0 ]] ;then
		    echo "mv ${line[0]} BEST"
		else
		    echo "mv ${line[0]} BEST/${line[keyelement]}-${line[0]}"
		fi
		
	    fi
	fi

	if [[ ${worst} -ne 0 ]] ; then
	    if [[ line[keyelement] -lt worst ]] ; then
		if [[ ${rename} -eq 0 ]] ;then
		    echo "mv ${line[0]} WORST"
		else
		    echo "mv ${line[0]} WORST/${line[keyelement]}-${line[0]}"
		fi
		
	    fi
	fi
	
    fi
    
done

exit

