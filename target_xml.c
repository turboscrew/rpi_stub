/*
target_xml.h

Copyright (C) 2015 Juha Aaltonen

This file is part of standalone gdb stub for Raspberry Pi 2B.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "target_xml.h"
#include "util.h"
#include "log.h"
#define MAX_FEAT_NAME 128
#define MAX_NAME_STR 32
#define MAX_OTHER_STR 32

// feature types
typedef enum
{
	feat_reg_desc,
	feat_type_last
} feat_type_t;

/*
The structure for register feature becomes something like this

features:
    feature_t
        feat_name
        num_descs
        feat_descs -> feat_desc*
                      feat_desc*
                      feat_desc* -> feat_desc_t
                                        type
                                        descr -> reg_feat_t
                                                     num_descs
                                        reg_desc* <- reg_desc
                                        reg_desc*
                          reg_desc_t <- reg_desc*

*/

// description for generating register (set) descriptions
typedef struct
{
	int num_regs; // (if 0 then individual register)
	char regname[MAX_NAME_STR];
	int regstart; // register name index start offset
	char reglen[MAX_NAME_STR];
	char valtype[MAX_NAME_STR];
	int regnum; // gdb register index start (for gdb)
	char other[MAX_OTHER_STR]; // other
} reg_desc_t;

// which reg descs for a single register feature
typedef struct
{
	int num_descs;
	reg_desc_t **reg_desc; // to table of reg desc pointers
} reg_feat_t;

// just for test (and place holder - maybe in future...)
typedef struct
{
	char name[MAX_OTHER_STR];
} other_feat_t;

// feature descriptor
// void * because pre-C99 doesn't support proper union initializers
typedef struct
{
	feat_type_t type; // type of feature
	void *descr; // pointer to feat type - "union"
} feat_desc_t;

// feature - can consist of different "sub-features"
typedef struct
{
	char feat_name[MAX_FEAT_NAME];
	int num_descs;
	feat_desc_t **feat_descs; // table of features
} feature_t;

// map arch type -> arch name (for gdb)
typedef struct
{
	arch_type_t arch_type;
	char arch_name[MAX_NAME_STR];
	feature_t **feature_list;
} arch_t;

// target description buffer
target_xml *msg_buff;

// core registers r0 - r15
reg_desc_t reg_desc_arm_gen_regs =
{
	16, // num_regs
	"r", // regname
	0, // regstart
	"32", // reglen
	"uint32", // valtype
	0, // regnum
	"" // other
};

// core register cpsr
reg_desc_t reg_desc_arm_cpsr =
{
	0, // num_regs
	"cpsr", // regname
	16, // regstart
	"32", // reglen
	"uint32", // valtype
	25, // regnum
	"" // other
};

// neon registers d0 - d31
reg_desc_t reg_desc_neon_gen_regs =
{
	32, // num_regs
	"d", // regname
	0, // regstart
	"64", // reglen
	"ieee_double", // valtype
	26, // regnum
	"" // other
};

// neon register fpscr
reg_desc_t reg_desc_neon_fpscr =
{
	0, // num_regs
	"fpscr", // regname
	0, // regstart
	"32", // reglen
	"uint32", // valtype
	58, // regnum
	"" // other
};

// list of core register descriptors
reg_desc_t *feat_descs_core[2] =
{
	&reg_desc_arm_gen_regs,
	&reg_desc_arm_cpsr
};

// descriptor of core register feature
reg_feat_t reg_feat_core =
{
		2, // num_descs
		feat_descs_core // reg_desc
};

// list of neon register descriptors
reg_desc_t *feat_descs_neon[2] =
{
	&reg_desc_neon_gen_regs,
	&reg_desc_neon_fpscr
};

// descriptor of neon register feature
reg_feat_t reg_feat_neon =
{
		2, // num_descs
		feat_descs_neon // reg_desc
};

// the only core feature descriptor
feat_desc_t feat_desc_core =
{
	feat_reg_desc, // type
	(void *)(&reg_feat_core) // descr
};

// list of feature descriptors of core feature
feat_desc_t *core_feat_descs[1] =
{
		&feat_desc_core
};

// the core feature
// I think feature name needs to be this to match gdb's
feature_t feature_core =
{
	"org.gnu.gdb.arm.core", // feat name
	1, // num_descs
	core_feat_descs // feat_descs
};

// the only neon feature descriptor
feat_desc_t feat_desc_neon =
{
	feat_reg_desc, // type
	(void *)(&reg_feat_neon) // descr
};

// list of feature descriptors of neon feature
feat_desc_t *neon_feat_descs[1] =
{
		&feat_desc_neon
};

// the neon feature
// I think feature name needs to be this to match gdb's
feature_t feature_neon =
{
	"org.gnu.gdb.arm.vfp", // feat_name
	1, // num_descs
	neon_feat_descs // feat_descs
};

// arm architecture feature list
feature_t *feats_arm[] =
{
	&feature_core,
	&feature_neon,
	(feature_t *) 0 // end marker
};

// list of architectures
// I think arch name needs to be this to match gdb's
arch_t arch_list[] =
{
	{arch_arm, "arm", feats_arm},
	{arch_last, "", (feature_t **)0}
};

// print string in the target description buffer
void tgt_put(char *str)
{
	char *ptr;
	int len;
	len = msg_buff->len;
	ptr = msg_buff->buff;
	ptr += len;
	len += util_str_copy(ptr, str, util_str_len(str)+1);
	msg_buff->len = len;
}

// generate header for the xml register description
void gen_target_header()
{
	tgt_put("<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\">");
}

// generate xml for one register
void gen_reg_entry(char *reg_name, char *reg_len, char *reg_type,
char *reg_num, char *reg_other)
{
	tgt_put("<reg name=\"");
	tgt_put(reg_name);
	tgt_put("\" bitsize=\"");
	tgt_put(reg_len);
	tgt_put("\" type=\"");
	tgt_put(reg_type);
	tgt_put("\" regnum=\"");
	tgt_put(reg_num);
	if (*reg_other != '\0')
	{
		tgt_put("\" ");
		tgt_put(reg_other);
	}
	tgt_put("\"/>");
}

// handle register descriptor
void gen_regs_desc(reg_desc_t *regs)
{
	char name_str[MAX_NAME_STR];
	char regnum_str[MAX_NAME_STR];
	char str_tmp[MAX_NAME_STR];
	int name_ind, reg_num;
	int i;

	if (regs->num_regs == 0)
	{
		util_word_to_dec(regnum_str, (unsigned int)regs->regnum);
		gen_reg_entry(regs->regname, regs->reglen, regs->valtype,
					regnum_str, regs->other);
	}
	else
	{
		for (i=0; i<regs->num_regs; i++)
		{
			name_ind = regs->regstart + i;
			util_str_copy(name_str, regs->regname, MAX_NAME_STR);
			util_word_to_dec(str_tmp, name_ind);
			util_append_str(name_str, str_tmp, MAX_NAME_STR);

			reg_num = regs->regnum + i;
			util_word_to_dec(regnum_str, reg_num);
			gen_reg_entry(name_str, regs->reglen, regs->valtype,
						regnum_str, regs->other);
		}
	}
}

// handle register feature
void gen_reg_feature(reg_feat_t *reg_feature)
{
	int i;
	for (i=0; i<reg_feature->num_descs; i++)
		gen_regs_desc((reg_feature->reg_desc)[i]);
}

// handle one feature descriptor
void gen_feat_desc(feat_desc_t *feature)
{
	switch (feature->type)
	{
		case feat_reg_desc:
			gen_reg_feature((reg_feat_t *)(feature->descr));
			break;
		default:
			break;
	}
}

// handle feature
void gen_feature(feature_t *feature)
{
	int i;
	tgt_put("<feature name=\"");
	tgt_put(feature->feat_name);
	tgt_put("\">");
	for (i=0; i<feature->num_descs; i++)
	{
		gen_feat_desc((feature->feat_descs)[i]);
	}
	tgt_put("</feature>");
}

// generate xml for architecture
void gen_arch(arch_type_t arch)
{
	int i = 0;

	while(arch_list[i].arch_type != arch_last)
	{
		if (arch_list[i].arch_type == arch) break;
		i++;
	}
	tgt_put("<architecture>");
	tgt_put(arch_list[i].arch_name);
	tgt_put("</architecture>");
}

// get the feature list of the architecture
feature_t **get_features(arch_type_t arch)
{
	int i = 0;

	while(arch_list[i].arch_type != arch_last)
	{
		if (arch_list[i].arch_type == arch) break;
		i++;
	}
	return arch_list[i].feature_list;
}

// generate target description
void gen_target(target_xml *buf, arch_type_t arch)
{
	int i;
	msg_buff = buf;
	feature_t **feature_list;

	LOG_PR_VAL("target_xml.buff: ", (unsigned int)(msg_buff->buff));
	gen_target_header();
	tgt_put("<target>");
	gen_arch(arch);
	feature_list = get_features(arch);
	i = 0;
	while (feature_list[i] != ((feature_t *) 0))
	{
		gen_feature(feature_list[i]);
		i++;
	}
	tgt_put("</target>");
}


