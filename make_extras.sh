cat $1 | grep -v extra | sed 's/$/,/' | sort > $2
