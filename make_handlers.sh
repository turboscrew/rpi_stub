cat $1 | grep -v handler | sort | uniq  | sed 's/^/instr_next_addr_t /' > $1.tmp
cat $1.tmp | sed 's/$/(unsigned int instr, ARM_decode_extra_t extra);/' > $2
rm $1.tmp
