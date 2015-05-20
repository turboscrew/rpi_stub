cat $1 | grep -v handler | sed 's/0x,/0x/g' | sed 's|,\},\s\/\/\s,|\}, \/\/ |g' > $2
