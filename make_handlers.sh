# make_handlers.sh
# 
# Copyright (C) 2015 Juha Aaltonen
# 
# This file is part of standalone gdb stub for Raspberry Pi 2B.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
cat $1 | grep -v handler | sort | uniq  | sed 's/^/instr_next_addr_t /' > $1.tmp
cat $1.tmp | sed 's/$/(unsigned int instr, ARM_decode_extra_t extra);/' > $2
rm $1.tmp
