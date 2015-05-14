/*
 * ARM_decode_table_data.h
 *
 *  Created on: May 15, 2015
 *      Author: jaa
 */

#ifndef ARM_DECODE_TABLE_DATA_H_
#define ARM_DECODE_TABLE_DATA_H_
// This should be included  within the decode table variable definition
// as the initializer:
// table_type_t table[] = {
// #include "ARM_decode_table_data.h"
// }
{0xE7F000F0,0xFFF000F0,arm_xtra_none,arm_core_exc}, // UDF<c> #<imm12>
{0xF1010000,0xFFF100F0,arm_xtra_none,arm_core_misc}, // SETEND <endian specifier>
{0xF1000000,0xFFF10020,arm_xtra_none,arm_core_status}, // CPS<effect> <iflags>{, #<mode>} CPS #<mode>
{0xF3B20000,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VSWP<c> <Qd>, <Qm> VSWP<c> <Dd>, <Dm>
{0xF3B20080,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VTRN<c>.<size> <Qd>, <Qm> VTRN<c>.<size> <Dd>, <Dm>
{0xF2000000,0xFE800F10,arm_xtra_none,arm_v_par}, // VHADD<c> <Qd>, <Qn>, <Qm> VH<op><c> <Dd>, <Dn>, <Dm>
{0xF3B20100,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VUZP<c>.<size> <Qd>, <Qm> VUZP<c>.<size> <Dd>, <Dm>
{0xF3B20180,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VZIP<c>.<size> <Qd>, <Qm> VZIP<c>.<size> <Dd>, <Dm>
{0xF2000100,0xFE800F10,arm_xtra_none,arm_v_par}, // VRHADD<c> <Qd>, <Qn>, <Qm> VRHADD<c> <Dd>, <Dn>, <Dm>
{0xF3B00000,0xFFB30E10,arm_xtra_none,arm_v_misc}, // VREV<n><c>.<size> <Qd>, <Qm> VREV<n><c>.<size> <Dd>, <Dm>
{0xF2800040,0xFE800E50,arm_xtra_none,arm_v_mac}, // VMLA<c>.<dt> <Qd>, <Qn>, <Dm[x]> V<op><c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2800000,0xFE800E50,arm_xtra_none,arm_v_par}, // VADDL<c>.<dt> <Qd>, <Dn>, <Dm> VADDW<c>.<dt> <Qd>, <Qn>, <Dm>
{0xF3B00200,0xFFB30F10,arm_xtra_none,arm_v_par}, // VPADDL<c>.<dt> <Qd>, <Qm> VPADDL<c>.<dt> <Dd>, <Dm>
{0xF3B20200,0xFFB30F10,arm_xtra_op,arm_mux}, // VQMOV{U}N<c>.<type><size> <Dd>, <Qm>; VMOVN<c>.<dt> <Dd>, <Qm>
{0xF2000200,0xFE800F10,arm_xtra_none,arm_v_par}, // VHSUB<c> <Qd>, <Qn>, <Qm> VH<op><c> <Dd>, <Dn>, <Dm>
{0xF2800240,0xFE800F50,arm_xtra_none,arm_v_mac}, // VMLAL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800340,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMLAL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF3B20300,0xFFB30FD0,arm_xtra_none,arm_v_shift}, // VSHLL<c>.<type><size> <Qd>, <Dm>, #<imm> (<imm> == <size>)
{0xF2000300,0xFE800F10,arm_xtra_none,arm_v_comp}, // VCGT<c>.<dt> <Qd>, <Qn>, <Qm> VCGT<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800200,0xFE800E50,arm_xtra_none,arm_v_par}, // VSUBL<c>.<dt> <Qd>, <Dn>, <Dm> VSUBW<c>.<dt> <Qd>, <Qn>, <Dm>
{0xF2800400,0xFF800F50,arm_xtra_none,arm_v_par}, // VADDHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF3B00400,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VCLS<c>.<dt> <Qd>, <Qm> VCLS<c>.<dt> <Dd>, <Dm>
{0xF3B00480,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VCLZ<c>.<dt> <Qd>, <Qm> VCLZ<c>.<dt> <Dd>, <Dm>
{0xF3800400,0xFF800F50,arm_xtra_none,arm_v_par}, // VRADDHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF2000400,0xFE800F10,arm_xtra_none,arm_v_shift}, // VSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF3B00500,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VCNT<c>.8 <Qd>, <Qm> VCNT<c>.8 <Dd>, <Dm>
{0xF3B00580,0xFFB30F90,arm_xtra_none,arm_v_bits}, // VMVN<c> <Qd>, <Qm> VMVN<c> <Dd>, <Dm>
{0xF2000500,0xFE800F10,arm_xtra_none,arm_v_shift}, // VRSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VRSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2800500,0xFE800F50,arm_xtra_none,arm_v_misc}, // VABAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF3B30400,0xFFB30E90,arm_xtra_none,arm_v_misc}, // VRECPE<c>.<dt> <Qd>, <Qm> VRECPE<c>.<dt> <Dd>, <Dm>
{0xF3B30480,0xFFB30E90,arm_xtra_none,arm_v_misc}, // VRSQRTE<c>.<dt> <Qd>, <Qm> VRSQRTE<c>.<dt> <Dd>, <Dm>
{0xF2800440,0xFE800E50,arm_xtra_none,arm_v_mac}, // VMLS<c>.<dt> <Qd>, <Qn>, <Dm[x]> V<op><c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2800600,0xFF800F50,arm_xtra_none,arm_v_par}, // VSUBHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF3B00600,0xFFB30F10,arm_xtra_none,arm_v_par}, // VPADAL<c>.<dt> <Qd>, <Qm> VPADAL<c>.<dt> <Dd>, <Dm>
{0xF3800600,0xFF800F50,arm_xtra_none,arm_v_par}, // VRSUBHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF2000600,0xFE800F10,arm_xtra_none,arm_v_misc}, // VMAX<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800640,0xFE800F50,arm_xtra_none,arm_v_mac}, // VMLSL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800740,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMLSL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF3B00700,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VQABS<c>.<dt> <Qd>, <Qm> VQABS<c>.<dt> <Dd>, <Dm>
{0xF3B00780,0xFFB30F90,arm_xtra_none,arm_v_misc}, // VQNEG<c>.<dt> <Qd>, <Qm> VQNEG<c>.<dt> <Dd>, <Dm>
{0xF2000700,0xFE800F10,arm_xtra_none,arm_v_misc}, // VABD<c>.<dt> <Qd>, <Qn>, <Qm> VABD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800700,0xFE800F50,arm_xtra_none,arm_v_misc}, // VABDL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF3B20600,0xFFB30ED0,arm_xtra_none,arm_v_misc}, // VCVT<c>.F32.F16 <Qd>, <Dm> VCVT<c>.F16.F32 <Dd>, <Qm>
{0xF3B30600,0xFFB30E10,arm_xtra_none,arm_v_misc}, // VCVT<c>.<Td>.<Tm> <Qd>, <Qm> VCVT<c>.<Td>.<Tm> <Dd>, <Dm>
{0xF3B10000,0xFFB30B90,arm_xtra_none,arm_v_comp}, // VCGT<c>.<dt> <Qd>, <Qm>, #0 VCGT<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10080,0xFFB30B90,arm_xtra_none,arm_v_comp}, // VCGE<c>.<dt> <Qd>, <Qm>, #0 VCGE<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10100,0xFFB30B90,arm_xtra_none,arm_v_comp}, // VCEQ<c>.<dt> <Qd>, <Qm>, #0 VCEQ<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10180,0xFFB30B90,arm_xtra_none,arm_v_comp}, // VCLE<c>.<dt> <Qd>, <Qm>, #0 VCLE<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10200,0xFFB30B90,arm_xtra_none,arm_v_comp}, // VCLT<c>.<dt> <Qd>, <Qm>, #0 VCLT<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10300,0xFFB30B90,arm_xtra_fp,arm_v_misc}, // VABS<c>.<dt> <Qd>, <Qm> VABS<c>.<dt> <Dd>, <Dm>
{0xF3B10380,0xFFB30B90,arm_xtra_fp,arm_v_misc}, // VNEG<c>.<dt> <Qd>, <Qm> VNEG<c>.<dt> <Dd>, <Dm>
{0xF2000800,0xFF800F10,arm_xtra_none,arm_v_par}, // VADD<c>.<dt> <Qd>, <Qn>, <Qm> VADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3000800,0xFF800F10,arm_xtra_none,arm_v_par}, // VSUB<c>.<dt> <Qd>, <Qn>, <Qm> VSUB<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800800,0xFE800F50,arm_xtra_none,arm_v_mac}, // VMLAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800900,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMLAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2000900,0xFF800F10,arm_xtra_none,arm_v_mac}, // VMLA<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3000900,0xFF800F10,arm_xtra_none,arm_v_mac}, // VMLS<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800840,0xFE800E50,arm_xtra_none,arm_v_mac}, // VMUL<c>.<dt> <Qd>, <Qn>, <Dm[x]> VMUL<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2000A00,0xFE800F10,arm_xtra_none,arm_v_misc}, // VPMAX<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800A40,0xFE800F50,arm_xtra_none,arm_v_mac}, // VMULL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800A00,0xFE800F50,arm_xtra_none,arm_v_mac}, // VMLSL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2000B00,0xFF800F10,arm_xtra_none,arm_v_mac}, // VQDMULH<c>.<dt> <Qd>, <Qn>, <Qm> VQDMULH<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800B40,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMULL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800B00,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMLSL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF3000B00,0xFF800F10,arm_xtra_none,arm_v_mac}, // VQRDMULH<c>.<dt> <Qd>, <Qn>, <Qm> VQRDMULH<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3B00800,0xFFB00C50,arm_xtra_none,arm_v_misc}, // VTBL<c>.8 <Dd>, <list>, <Dm>
{0xF3B00840,0xFFB00C50,arm_xtra_none,arm_v_misc}, // VTBX<c>.8 <Dd>, <list>, <Dm>
{0xF3B00C00,0xFFB00F90,arm_xtra_none,arm_v_misc}, // VDUP<c>.<size> <Qd>, <Dm[x]> VDUP<c>.<size> <Dd>, <Dm[x]>
{0xF2800C40,0xFE800F50,arm_xtra_none,arm_v_mac}, // VQDMULH<c>.<dt> <Qd>, <Qn>, <Dm[x]> VQDMULH<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2000D00,0xFFA00F10,arm_xtra_fp,arm_v_par}, // VADD<c>.F32 <Qd>, <Qn>, <Qm> VADD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2200D00,0xFFA00F10,arm_xtra_fp,arm_v_par}, // VSUB<c>.F32 <Qd>, <Qn>, <Qm> VSUB<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800D00,0xFF800F50,arm_xtra_none,arm_v_mac}, // VQDMULL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF3000D00,0xFFA00F10,arm_xtra_none,arm_v_par}, // VPADD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3200D00,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VABD<c>.F32 <Qd>, <Qn>, <Qm> VABD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800D40,0xFE800F50,arm_xtra_none,arm_v_mac}, // VQRDMULH<c>.<dt> <Qd>, <Qn>, <Dm[x]> VQRDMULH<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2000E00,0xFFA00F10,arm_xtra_none,arm_v_comp}, // VCEQ<c>.F32 <Qd>, <Qn>, <Qm> VCEQ<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3000E00,0xFFA00F10,arm_xtra_none,arm_v_comp}, // VCGE<c>.F32 <Qd>, <Qn>, <Qm> VCGE<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3200E00,0xFFA00F10,arm_xtra_none,arm_v_comp}, // VCGT<c>.F32 <Qd>, <Qn>, <Qm> VCGT<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000F00,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VMAX<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2200F00,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VMIN<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF3000F00,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VPMAX<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3200F00,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VPMIN<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800C00,0xFE800D50,arm_xtra_none,arm_v_mac}, // VMULL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2B00000,0xFFB00010,arm_xtra_none,arm_v_misc}, // VEXT<c>.8 <Qd>, <Qn>, <Qm>, #<imm> VEXT<c>.8 <Dd>, <Dn>, <Dm>, #<imm>
{0xF2000010,0xFE800F10,arm_xtra_none,arm_v_par}, // VQADD<c>.<dt> <Qd>, <Qn>, <Qm> VQADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VAND<c> <Qd>, <Qn>, <Qm> VAND<c> <Dd>, <Dn>, <Dm>
{0xF2100110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VBIC<c> <Qd>, <Qn>, <Qm> VBIC<c> <Dd>, <Dn>, <Dm>
{0xF2200110,0xFFB00F10,arm_xtra_same_nm,arm_mux}, // VORR<c> <Qd>, <Qn>, <Qm> VORR<c> <Dd>, <Dn>, <Dm>; VMOV<c> <Qd>, <Qm> VMOV<c> <Dd>, <Dm>
{0xF2300110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VORN<c> <Qd>, <Qn>, <Qm> VORN<c> <Dd>, <Dn>, <Dm>
{0xF3000110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VEOR<c> <Qd>, <Qn>, <Qm> VEOR<c> <Dd>, <Dn>, <Dm>
{0xF3100110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VBSL<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF3200110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VBIT<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF3300110,0xFFB00F10,arm_xtra_none,arm_v_bits}, // VBIF<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF2800110,0xFE800F10,arm_xtra_none,arm_v_shift}, // VSRA<c>.<type><size> <Qd>, <Qm>, #<imm> VSRA<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2000210,0xFE800F10,arm_xtra_none,arm_v_par}, // VQSUB<c>.<type><size> <Qd>, <Qn>, <Qm> VQSUB<c>.<type><size> <Dd>, <Dn>, <Dm>
{0xF2800210,0xFE800F10,arm_xtra_none,arm_v_shift}, // VRSHR<c>.<type><size> <Qd>, <Qm>, #<imm> VRSHR<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2000310,0xFE800F10,arm_xtra_none,arm_v_comp}, // VCGE<c>.<dt> <Qd>, <Qn>, <Qm> VCGE<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800310,0xFE800F10,arm_xtra_none,arm_v_shift}, // VRSRA<c>.<type><size> <Qd>, <Qm>, #<imm> VRSRA<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF3800410,0xFF800F10,arm_xtra_none,arm_v_shift}, // VSRI<c>.<size> <Qd>, <Qm>, #<imm> VSRI<c>.<size> <Dd>, <Dm>, #<imm>
{0xF2000410,0xFE800F10,arm_xtra_none,arm_v_shift}, // VQSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VQSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2800510,0xFF800F10,arm_xtra_none,arm_v_shift}, // VSHL<c>.I<size> <Qd>, <Qm>, #<imm> VSHL<c>.I<size> <Dd>, <Dm>, #<imm>
{0xF3800510,0xFF800F10,arm_xtra_none,arm_v_shift}, // VSLI<c>.<size> <Qd>, <Qm>, #<imm> VSLI<c>.<size> <Dd>, <Dm>, #<imm>
{0xF2000510,0xFE800F10,arm_xtra_none,arm_v_shift}, // VQRSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VQRSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2000610,0xFE800F10,arm_xtra_none,arm_v_misc}, // VMIN<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000710,0xFE800F10,arm_xtra_none,arm_v_misc}, // VABA<c>.<dt> <Qd>, <Qn>, <Qm> VABA<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800610,0xFE800E10,arm_xtra_none,arm_v_shift}, // VQSHL{U}<c>.<type><size> <Qd>, <Qm>, #<imm> VQSHL{U}<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2000810,0xFF800F10,arm_xtra_none,arm_v_comp}, // VTST<c>.<size> <Qd>, <Qn>, <Qm> VTST<c>.<size> <Dd>, <Dn>, <Dm>
{0xF2800810,0xFF800FD0,arm_xtra_imm,arm_mux}, // VSHRN<c>.I<size> <Dd>, <Qm>, #<imm>; VQSHR{U}N<c>.<type><size> <Dd>, <Qm>, #<imm>
{0xF2800850,0xFF800FD0,arm_xtra_imm,arm_mux}, // VRSHRN<c>.I<size> <Dd>, <Qm>, #<imm>; VQRSHR{U}N<c>.<type><size> <Dd>, <Qm>, #<imm>
{0xF3000810,0xFF800F10,arm_xtra_none,arm_v_comp}, // VCEQ<c>.<dt> <Qd>, <Qn>, <Qm> VCEQ<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000910,0xFE800F10,arm_xtra_none,arm_v_mac}, // VMUL<c>.<dt> <Qd>, <Qn>, <Qm> VMUL<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000A10,0xFE800F10,arm_xtra_none,arm_v_misc}, // VPMIN<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800A10,0xFE800FD0,arm_xtra_imm,arm_mux}, // VSHLL<c>.<type><size> <Qd>, <Dm>, #<imm> (0 < <imm> < <size>); VMOVL<c>.<dt> <Qd>, <Dm>
{0xF2000B10,0xFF800F10,arm_xtra_none,arm_v_par}, // VPADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000C10,0xFF800F10,arm_xtra_none,arm_v_mac}, // VFM<y><c>.F32 <Qd>, <Qn>, <Qm> VFM<y><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000D10,0xFFA00F10,arm_xtra_fp,arm_v_mac}, // VMLA<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2200D10,0xFFA00F10,arm_xtra_fp,arm_v_mac}, // VMLS<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF3000D10,0xFFA00F10,arm_xtra_fp,arm_v_mac}, // VMUL<c>.F32 <Qd>, <Qn>, <Qm> VMUL<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3000E10,0xFFA00F10,arm_xtra_none,arm_v_comp}, // VACGE<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF3200E10,0xFFA00F10,arm_xtra_none,arm_v_comp}, // VACGT<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000F10,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VRECPS<c>.F32 <Qd>, <Qn>, <Qm> VRECPS<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2200F10,0xFFA00F10,arm_xtra_none,arm_v_misc}, // VRSQRTS<c>.F32 <Qd>, <Qn>, <Qm> VRSQRTS<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800E10,0xFE800E90,arm_xtra_none,arm_v_misc}, // VCVT<c>.<Td>.<Tm> <Qd>, <Qm>, #<fbits> VCVT<c>.<Td>.<Tm> <Dd>, <Dm>, #<fbits>
{0xF2800010,0xFEB800B0,arm_xtra_imm_cmode,arm_mux}, // VORR<c>.<dt> <Qd>, #<imm> VORR<c>.<dt> <Dd>, #<imm>; VMOV<c>.<dt> <Qd>, #<imm> VMOV<c>.<dt> <Dd>, #<imm>; VSHR<c>.<type><size> <Qd>, <Qm>, #<imm> VSHR<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2800030,0xFEB800B0,arm_xtra_cmode,arm_mux}, // VBIC<c>.<dt> <Qd>, #<imm> VBIC<c>.<dt> <Dd>, #<imm>; VMVN<c>.<dt> <Qd>, #<imm> VMVN<c>.<dt> <Dd>, #<imm>
{0xF4000000,0xFFB00000,arm_xtra_type,arm_mux}, // VST1-4<c>.<size> <list>, [<Rn>{:<align>}]{!} VST1-4<c>.<size> <list>, [<Rn>{:<align>}], <Rm> (multiple single)
{0xF4200000,0xFFB00000,arm_xtra_type,arm_mux}, // VLD1-4<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1-4<c>.<size> <list>, [<Rn>{:<align>}], <Rm> (multiple element structures)
{0xF4800000,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VST1<c>.<size> <list>, [<Rn>{:<align>}]{!} VST1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4800100,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VST2<c>.<size> <list>, [<Rn>{:<align>}]{!} VST2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4800200,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VST3<c>.<size> <list>, [<Rn>]{!} VST3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4800300,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VST4<c>.<size> <list>, [<Rn>{:<align>}]{!} VST4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00C00,0xFFB00F00,arm_xtra_all_lanes,arm_vfp_ldst_elem}, // VLD1<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00D00,0xFFB00F00,arm_xtra_all_lanes,arm_vfp_ldst_elem}, // VLD2<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00E00,0xFFB00F00,arm_xtra_all_lanes,arm_vfp_ldst_elem}, // VLD3<c>.<size> <list>, [<Rn>]{!} VLD3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4A00F00,0xFFB00F00,arm_xtra_all_lanes,arm_vfp_ldst_elem}, // VLD4<c>.<size> <list>, [<Rn>{ :<align>}]{!} VLD4<c>.<size> <list>, [<Rn>{ :<align>}], <Rm>
{0xF4A00000,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VLD1<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00100,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VLD2<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00200,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VLD3<c>.<size> <list>, [<Rn>]{!} VLD3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4A00300,0xFFB00300,arm_xtra_one_lane,arm_vfp_ldst_elem}, // VLD4<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4500000,0xFF700000,arm_xtra_none,arm_core_misc}, // PLI [<Rn>, #+/-<imm12>] PLI <label> PLI [PC, #-0] Special case
{0xF5700010,0xFFF000F0,arm_xtra_none,arm_core_misc}, // CLREX
{0xF5700040,0xFFF000F0,arm_xtra_none,arm_core_misc}, // DSB <option>
{0xF5700050,0xFFF000F0,arm_xtra_none,arm_core_misc}, // DMB <option>
{0xF5700060,0xFFF000F0,arm_xtra_none,arm_core_misc}, // ISB <option>
{0xF51F0000,0xFF3F0000,arm_xtra_none,arm_core_misc}, // PLD <label> PLD [PC, #-0] Special case
{0xF5100000,0xFF300000,arm_xtra_none,arm_core_misc}, // PLD{W} [<Rn>, #+/-<imm12>]
{0xF6500000,0xFF700010,arm_xtra_none,arm_core_misc}, // PLI [<Rn>,+/-<Rm>{, <shift>}]
{0xF7100000,0xFF300010,arm_xtra_none,arm_core_misc}, // PLD{W} [<Rn>,+/-<Rm>{, <shift>}]
{0xF8100000,0xFE500000,arm_xtra_none,arm_core_exc}, // RFE{<amode>} <Rn>{!}
{0xF8400000,0xFE500000,arm_xtra_none,arm_core_exc}, // SRS{<amode>} SP{!}, #<mode>
{0xFA000000,0xFE000000,arm_xtra_none,arm_branch}, // BLX <label>
{0xFC400000,0xFFF00000,arm_xtra_none,arm_coproc}, // MCRR2<c> <coproc>, <opc1>, <Rt>, <Rt2>, <CRm>
{0xFC500000,0xFFF00000,arm_xtra_none,arm_coproc}, // MRRC2<c> <coproc>, <opc>, <Rt>, <Rt2>, <CRm>
{0xFC000000,0xFE100000,arm_xtra_none,arm_coproc}, // STC2{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} STC2{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> STC2{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0xFC1F0000,0xFE1F0000,arm_xtra_none,arm_coproc}, // LDC2{L}<c> <coproc>, <CRd>, <label> LDC2{L}<c> <coproc>, <CRd>, [PC, #-0] Special case LDC2{L}<c> <coproc>, <CRd>, [PC], <option>
{0xFC100000,0xFE100000,arm_xtra_none,arm_coproc}, // LDC2{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} LDC2{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> LDC2{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0xFE000010,0xFF100010,arm_xtra_none,arm_coproc}, // MCR2<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0xFE100010,0xFF100010,arm_xtra_none,arm_coproc}, // MRC2<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0xFE000000,0xFF000010,arm_xtra_none,arm_coproc}, // CDP2<c> <coproc>, <opc1>, <CRd>, <CRn>, <CRm>, <opc2>
{0x004F00D0,0x0E5F00F0,arm_xtra_spec,arm_core_ldst}, // LDRD<c> <Rt>, <Rt2>, <label> LDRD<c> <Rt>, <Rt2>, [PC, #-0] Special case
{0x00000090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // MUL{S}<c> <Rd>, <Rn>, <Rm>
{0x00000010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // AND{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00000000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // AND{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00200090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // MLA{S}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x00200010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // EOR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00200000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // EOR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00400090,0x0FF000F0,arm_xtra_none,arm_core_data_mac}, // UMAAL<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x004D0000,0x0FEF0010,arm_xtra_none,arm_core_data_std}, // SUB{S}<c> <Rd>, SP, <Rm>{, <shift>}
{0x00400010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // SUB{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00400000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // SUB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00600090,0x0FF000F0,arm_xtra_none,arm_core_data_mac}, // MLS<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x00600010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // RSB{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00600000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // RSB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x008D0000,0x0FEF0010,arm_xtra_none,arm_core_data_std}, // ADD{S}<c> <Rd>, SP, <Rm>{, <shift>}
{0x00800090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // UMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00800010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // ADD{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00800000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // ADD{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00A00090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // UMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00A00010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // ADC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00A00000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // ADC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00C00090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // SMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00C00010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // SBC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00C00000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // SBC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00E00090,0x0FE000F0,arm_xtra_none,arm_core_data_mac}, // SMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00E00010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // RSC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00E00000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // RSC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x002000B0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // STRHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x003000B0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x003000D0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRSBT<c> <Rt>, [<Rn>], +/-<Rm>
{0x003000F0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRSHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x006000B0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // STRHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x007000B0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x007000D0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRSBT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x007000F0,0x0F7000F0,arm_xtra_unpriv,arm_core_ldst}, // LDRSHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x01000080,0x0FF00090,arm_xtra_none,arm_core_data_mac}, // SMLA<x><y><c> <Rd>, <Rn>, <Rm>, <Ra>
{0x01000050,0x0FF000F0,arm_xtra_none,arm_core_data_satx}, // QADD<c> <Rd>, <Rm>, <Rn>
{0x01100010,0x0FF00090,arm_xtra_none,arm_core_data_std}, // TST<c> <Rn>, <Rm>, <type> <Rs>
{0x01100000,0x0FF00010,arm_xtra_none,arm_core_data_std}, // TST<c> <Rn>, <Rm>{, <shift>}
{0x01200010,0x0FF000F0,arm_xtra_none,arm_branch}, // BX<c> <Rm>
{0x01200020,0x0FF000F0,arm_xtra_none,arm_branch}, // BXJ<c> <Rm>
{0x01200030,0x0FF000F0,arm_xtra_none,arm_branch}, // BLX<c> <Rm>
{0x012000A0,0x0FF000B0,arm_xtra_none,arm_core_data_mac}, // SMULW<y><c> <Rd>, <Rn>, <Rm>
{0x01200080,0x0FF000B0,arm_xtra_none,arm_core_data_mac}, // SMLAW<y><c> <Rd>, <Rn>, <Rm>, <Ra>
{0x01200050,0x0FF000F0,arm_xtra_none,arm_core_data_satx}, // QSUB<c> <Rd>, <Rm>, <Rn>
{0x01200070,0x0FF000F0,arm_xtra_none,arm_core_exc}, // BKPT #<imm12>
{0x01200000,0x0FF302F0,arm_xtra_priv,arm_mux}, // MSR<c> <spec reg>, <Rn>
{0x01300010,0x0FF00090,arm_xtra_none,arm_core_data_std}, // TEQ<c> <Rn>, <Rm>, <type> <Rs>
{0x01300000,0x0FF00010,arm_xtra_none,arm_core_data_std}, // TEQ<c> <Rn>, <Rm>{, <shift>}
{0x01400080,0x0FF00090,arm_xtra_none,arm_core_data_mac}, // SMLAL<x><y><c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x01400050,0x0FF000F0,arm_xtra_none,arm_core_data_satx}, // QDADD<c> <Rd>, <Rm>, <Rn>
{0x01400070,0x0FF000F0,arm_xtra_none,arm_core_exc}, // HVC #<imm>
{0x01500010,0x0FF00090,arm_xtra_none,arm_core_data_std}, // CMP<c> <Rn>, <Rm>, <type> <Rs>
{0x01500000,0x0FF00010,arm_xtra_none,arm_core_data_std}, // CMP<c> <Rn>, <Rm>{, <shift>}
{0x01600060,0x0FF000F0,arm_xtra_none,arm_core_exc}, // ERET<c>
{0x01600070,0x0FF000F0,arm_xtra_none,arm_core_exc}, // SMC<c> #<imm4>
{0x01600010,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // CLZ<c> <Rd>, <Rm>
{0x01600080,0x0FF00090,arm_xtra_none,arm_core_data_mac}, // SMUL<x><y><c> <Rd>, <Rn>, <Rm>
{0x01600050,0x0FF000F0,arm_xtra_none,arm_core_data_satx}, // QDSUB<c> <Rd>, <Rm>, <Rn>
{0x01700010,0x0FF00090,arm_xtra_none,arm_core_data_std}, // CMN<c> <Rn>, <Rm>, <type> <Rs>
{0x01700000,0x0FF00010,arm_xtra_none,arm_core_data_std}, // CMN<c> <Rn>, <Rm>{, <shift>}
{0x01000090,0x0FB000F0,arm_xtra_none,arm_core_misc}, // SWP{B}<c> <Rt>, <Rt2>, [<Rn>]
{0x01000000,0x0FB002F0,arm_xtra_priv,arm_mux}, // MRS<c> <Rd>, <spec reg>
{0x01000200,0x0FB002F0,arm_xtra_banked,arm_core_status}, // MRS<c> <Rd>, <banked reg>
{0x01200200,0x0FB002F0,arm_xtra_banked,arm_core_status}, // MSR<c> <banked reg>, <Rn>
{0x01800090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // STREX<c> <Rd>, <Rt>, [<Rn>]
{0x01900090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // LDREX<c> <Rt>, [<Rn>]
{0x01800010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // ORR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01800000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // ORR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x01A00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // STREXD<c> <Rd>, <Rt>, <Rt2>, [<Rn>]
{0x01B00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // LDREXD<c> <Rt>, <Rt2>, [<Rn>]
{0x01A00010,0x0FE000F0,arm_xtra_none,arm_core_data_shift}, // LSL{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00030,0x0FE000F0,arm_xtra_none,arm_core_data_shift}, // LSR{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00050,0x0FE000F0,arm_xtra_none,arm_core_data_shift}, // ASR{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00070,0x0FE000F0,arm_xtra_none,arm_core_data_shift}, // ROR{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00000,0x0FE00070,arm_xtra_immzero,arm_mux}, // LSL{S}<c> <Rd>, <Rm>, #<imm5>; MOV{S}<c> <Rd>, <Rm>
{0x01A00020,0x0FE00070,arm_xtra_none,arm_core_data_shift}, // LSR{S}<c> <Rd>, <Rm>, #<imm>
{0x01A00040,0x0FE00070,arm_xtra_none,arm_core_data_shift}, // ASR{S}<c> <Rd>, <Rm>, #<imm>
{0x01A00060,0x0FE00070,arm_xtra_immzero,arm_mux}, // ROR{S}<c> <Rd>, <Rm>, #<imm>; RRX{S}<c> <Rd>, <Rm>
{0x01C00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // STREXB<c> <Rd>, <Rt>, [<Rn>]
{0x01D00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // LDREXB<c> <Rt>, [<Rn>]
{0x01C00010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // BIC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01C00000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // BIC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x01E00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // STREXH<c> <Rd>, <Rt>, [<Rn>]
{0x01F00090,0x0FF000F0,arm_xtra_excl,arm_core_ldst}, // LDREXH<c> <Rt>, [<Rn>]
{0x01E00010,0x0FE00090,arm_xtra_none,arm_core_data_std}, // MVN{S}<c> <Rd>, <Rm>, <type> <Rs>
{0x01E00000,0x0FE00010,arm_xtra_none,arm_core_data_std}, // MVN{S}<c> <Rd>, <Rm>{, <shift>}
{0x000000B0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // STRH<c> <Rt>, [<Rn>,+/-<Rm>]{!} STRH<c> <Rt>, [<Rn>],+/-<Rm>
{0x000000D0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRD<c> <Rt>, <Rt2>, [<Rn>,+/-<Rm>]{!} LDRD<c> <Rt>, <Rt2>, [<Rn>],+/-<Rm>
{0x000000F0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // STRD<c> <Rt>, <Rt2>, [<Rn>,+/-<Rm>]{!} STRD<c> <Rt>, <Rt2>, [<Rn>],+/-<Rm>
{0x001000B0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRH<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRH<c> <Rt>, [<Rn>],+/-<Rm>
{0x001000D0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRSB<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRSB<c> <Rt>, [<Rn>],+/-<Rm>
{0x001000F0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRSH<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRSH<c> <Rt>, [<Rn>],+/-<Rm>
{0x004000B0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // STRH<c> <Rt>, [<Rn>{, #+/-<imm8>}] STRH<c> <Rt>, [<Rn>], #+/-<imm8> STRH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x004000D0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm8>}] LDRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm8> LDRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm8>]!
{0x004000F0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // STRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm8>}] STRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm8> STRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm8>]!
{0x005F00B0,0x0E5F00F0,arm_xtra_none,arm_core_ldst}, // LDRH<c> <Rt>, <label> LDRH<c> <Rt>, [PC, #-0] Special case
{0x005F00D0,0x0E5F00F0,arm_xtra_none,arm_core_ldst}, // LDRSB<c> <Rt>, <label> LDRSB<c> <Rt>, [PC, #-0] Special case
{0x005F00F0,0x0E5F00F0,arm_xtra_none,arm_core_ldst}, // LDRSH<c> <Rt>, <label> LDRSH<c> <Rt>, [PC, #-0] Special case
{0x005000B0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRH<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRH<c> <Rt>, [<Rn>], #+/-<imm8> LDRH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x005000D0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRSB<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRSB<c> <Rt>, [<Rn>], #+/-<imm8> LDRSB<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x005000F0,0x0E5000F0,arm_xtra_none,arm_core_ldst}, // LDRSH<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRSH<c> <Rt>, [<Rn>], #+/-<imm8> LDRSH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x01B0F060,0x0FF0FFF0,arm_xtra_ret,arm_core_data_shift}, // RRXS<c> PC, <Rm>
{0x0000F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // AND{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0020F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // EOR{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0040F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // SUB{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0060F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // RSB{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0080F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // ADD{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00A0F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // ADC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00C0F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // SBC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00E0F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // RSC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0180F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // ORR{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x01A0F000,0x0FE0F070,arm_xtra_immzero,arm_mux}, // LSL{S}<c> PC, <Rm>, #<imm5>; MOV{S}<c> PC, <Rm>
{0x01A0F020,0x0FE0F070,arm_xtra_ret,arm_core_data_shift}, // LSR{S}<c> PC, <Rm>, #<imm>
{0x01A0F040,0x0FE0F070,arm_xtra_ret,arm_core_data_shift}, // ASR{S}<c> PC, <Rm>, #<imm>
{0x01A0F060,0x0FE0F070,arm_xtra_ret,arm_core_data_shift}, // ROR{S}<c> PC, <Rm>, #<imm>
{0x01C0F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // BIC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x01E0F000,0x0FE0F010,arm_xtra_ret,arm_core_data_std}, // MVN{S}<c> PC, <Rm>{, <sift>}
{0x02000000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // AND{S}<c> <Rd>, <Rn>, #<const>
{0x02200000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // EOR{S}<c> <Rd>, <Rn>, #<const>
{0x024F0000,0x0FFF0000,arm_xtra_none,arm_core_data_std}, // ADR<c> <Rd>, <label> <label> before current instruction SUB <Rd>, PC, #0 Special case for subtraction of zero
{0x024D0000,0x0FEF0000,arm_xtra_none,arm_core_data_std}, // SUB{S}<c> <Rd>, SP, #<const>
{0x02400000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // SUB{S}<c> <Rd>, <Rn>, #<const>
{0x02600000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // RSB{S}<c> <Rd>, <Rn>, #<const>
{0x028F0000,0x0FFF0000,arm_xtra_none,arm_core_data_std}, // ADR<c> <Rd>, <label> <label> after current instruction
{0x028D0000,0x0FEF0000,arm_xtra_none,arm_core_data_std}, // ADD{S}<c> <Rd>, SP, #<const>
{0x02800000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // ADD{S}<c> <Rd>, <Rn>, #<const>
{0x02A00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // ADC{S}<c> <Rd>, <Rn>, #<const>
{0x02C00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // SBC{S}<c> <Rd>, <Rn>, #<const>
{0x02E00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // RSC{S}<c> <Rd>, <Rn>, #<const>
{0x03000000,0x0FF00000,arm_xtra_movw,arm_core_data_misc}, // MOVW<c> <Rd>, #<imm12>
{0x03100000,0x0FF00000,arm_xtra_none,arm_core_data_std}, // TST<c> <Rn>, #<const>
{0x03200002,0x0FFF00FE,arm_xtra_hint,arm_mux}, // WFE<c>; WFI
{0x03200004,0x0FFF00FE,arm_xtra_none,arm_core_misc}, // SEV<c>
{0x032000F0,0x0FFF00F0,arm_xtra_none,arm_core_misc}, // DBG<c> #<option>
{0x03300000,0x0FF00000,arm_xtra_none,arm_core_data_std}, // TEQ<c> <Rn>, #<const>
{0x03400000,0x0FF00000,arm_xtra_none,arm_core_data_misc}, // MOVT<c> <Rd>, #<imm12>
{0x03500000,0x0FF00000,arm_xtra_none,arm_core_data_std}, // CMP<c> <Rn>, #<const>
{0x03700000,0x0FF00000,arm_xtra_none,arm_core_data_std}, // CMN<c> <Rn>, #<const>
{0x03200000,0x0FB00000,arm_xtra_priv_hint,arm_mux}, // MSR<c> <spec reg>, #<const>; NOP, YIELD
{0x03800000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // ORR{S}<c> <Rd>, <Rn>, #<const>
{0x03A00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // MOV{S}<c> <Rd>, #<const>
{0x03C00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // BIC{S}<c> <Rd>, <Rn>, #<const>
{0x03E00000,0x0FE00000,arm_xtra_none,arm_core_data_std}, // MVN{S}<c> <Rd>, #<const>
{0x0220F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // EOR{S}<c> PC, <Rn>, #<const>
{0x0240F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // SUB{S}<c> PC, <Rn>, #<const>
{0x0260F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // RSB{S}<c> PC, <Rn>, #<const>
{0x0280F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // ADD{S}<c> PC, <Rn>, #<const>
{0x02A0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // ADC{S}<c> PC, <Rn>, #<const>
{0x02C0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // SBC{S}<c> PC, <Rn>, #<const>
{0x02E0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // RSC{S}<c> PC, <Rn>, #<const>
{0x03A0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // MOV{S}<c> PC, #<const>
{0x03C0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // BIC{S}<c> PC, <Rn>, #<const>
{0x03E0F000,0x0FE0F000,arm_xtra_ret,arm_core_data_std}, // MVN{S}<c> PC, #<const>
{0x049D0004,0x0FFF0FFE,arm_xtra_single,arm_core_ldstm}, // POP<c> <registers>, <Rt>
{0x04200000,0x0F700000,arm_xtra_unpriv,arm_core_ldst}, // STRT<c> <Rt>, [<Rn>] {, +/-<imm12>}
{0x04300000,0x0F700000,arm_xtra_unpriv,arm_core_ldst}, // LDRT<c> <Rt>, [<Rn>] {, #+/-<imm12>}
{0x04600000,0x0F700000,arm_xtra_unpriv,arm_core_ldst}, // STRBT<c> <Rt>, [<Rn>], #+/-<imm12>
{0x04700000,0x0F700000,arm_xtra_unpriv,arm_core_ldst}, // LDRBT<c> <Rt>, [<Rn>], #+/-<imm12>
{0x052D0004,0x0FFF0FFE,arm_xtra_single,arm_core_ldstm}, // PUSH<c> <registers>, <Rt>
{0x04000000,0x0E500000,arm_xtra_none,arm_core_ldst}, // STR<c> <Rt>, [<Rn>{, #+/-<imm12>}] STR<c> <Rt>, [<Rn>], #+/-<imm12> STR<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x041F0000,0x0E5F0000,arm_xtra_none,arm_core_ldst}, // LDR<c> <Rt>, <label> LDR<c> <Rt>, [PC, #-0] Special case
{0x04100000,0x0E500000,arm_xtra_none,arm_core_ldst}, // LDR<c> <Rt>, [<Rn>{, #+/-<imm12>}] LDR<c> <Rt>, [<Rn>], #+/-<imm12> LDR<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x04400000,0x0E500000,arm_xtra_none,arm_core_ldst}, // STRB<c> <Rt>, [<Rn>{, #+/-<imm12>}] STRB<c> <Rt>, [<Rn>], #+/-<imm12> STRB<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x045F0000,0x0E5F0000,arm_xtra_none,arm_core_ldst}, // LDRB<c> <Rt>, <label> LDRB<c> <Rt>, [PC, #-0] Special case
{0x04500000,0x0E500000,arm_xtra_none,arm_core_ldst}, // LDRB<c> <Rt>, [<Rn>{, #+/-<imm12>}] LDRB<c> <Rt>, [<Rn>], #+/-<imm12> LDRB<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x06200000,0x0F700010,arm_xtra_unpriv,arm_core_ldst}, // STRT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06300000,0x0F700010,arm_xtra_unpriv,arm_core_ldst}, // LDRT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06600000,0x0F700010,arm_xtra_unpriv,arm_core_ldst}, // STRBT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06700000,0x0F700010,arm_xtra_unpriv,arm_core_ldst}, // LDRBT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06000000,0x0E500010,arm_xtra_none,arm_core_ldst}, // STR<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} STR<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06100000,0x0E500010,arm_xtra_none,arm_core_ldst}, // LDR<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} LDR<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06400000,0x0E500010,arm_xtra_none,arm_core_ldst}, // STRB<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} STRB<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06500000,0x0E500010,arm_xtra_none,arm_core_ldst}, // LDRB<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} LDRB<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06100010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SADD16<c> <Rd>, <Rn>, <Rm>
{0x06100030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SASX<c> <Rd>, <Rn>, <Rm>
{0x06100050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SSAX<c> <Rd>, <Rn>, <Rm>
{0x06100070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SSUB16<c> <Rd>, <Rn>, <Rm>
{0x06100090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SADD8<c> <Rd>, <Rn>, <Rm>
{0x061000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SSUB8<c> <Rd>, <Rn>, <Rm>
{0x06200010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QADD16<c> <Rd>, <Rn>, <Rm>
{0x06200030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QASX<c> <Rd>, <Rn>, <Rm>
{0x06200050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QSAX<c> <Rd>, <Rn>, <Rm>
{0x06200070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QSUB16<c> <Rd>, <Rn>, <Rm>
{0x06200090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QADD8<c> <Rd>, <Rn>, <Rm>
{0x062000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // QSUB8<c> <Rd>, <Rn>, <Rm>
{0x06300010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHADD16<c> <Rd>, <Rn>, <Rm>
{0x06300030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHASX<c> <Rd>, <Rn>, <Rm>
{0x06300050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHSAX<c> <Rd>, <Rn>, <Rm>
{0x06300070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHSUB16<c> <Rd>, <Rn>, <Rm>
{0x06300090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHADD8<c> <Rd>, <Rn>, <Rm>
{0x063000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // SHSUB8<c> <Rd>, <Rn>, <Rm>
{0x06500010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UADD16<c> <Rd>, <Rn>, <Rm>
{0x06500030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UASX<c> <Rd>, <Rn>, <Rm>
{0x06500050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // USAX<c> <Rd>, <Rn>, <Rm>
{0x06500070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // USUB16<c> <Rd>, <Rn>, <Rm>
{0x06500090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UADD8<c> <Rd>, <Rn>, <Rm>
{0x065000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // USUB8<c> <Rd>, <Rn>, <Rm>
{0x06600010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQADD16<c> <Rd>, <Rn>, <Rm>
{0x06600030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQASX<c> <Rd>, <Rn>, <Rm>
{0x06600050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQSAX<c> <Rd>, <Rn>, <Rm>
{0x06600070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQSUB16<c> <Rd>, <Rn>, <Rm>
{0x06600090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQADD8<c> <Rd>, <Rn>, <Rm>
{0x066000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UQSUB8<c> <Rd>, <Rn>, <Rm>
{0x06700010,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHADD16<c> <Rd>, <Rn>, <Rm>
{0x06700030,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHASX<c> <Rd>, <Rn>, <Rm>
{0x06700050,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHSAX<c> <Rd>, <Rn>, <Rm>
{0x06700070,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHSUB16<c> <Rd>, <Rn>, <Rm>
{0x06700090,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHADD8<c> <Rd>, <Rn>, <Rm>
{0x067000F0,0x0FF000F0,arm_xtra_none,arm_core_data_par}, // UHSUB8<c> <Rd>, <Rn>, <Rm>
{0x068F0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // SXTB16<c> <Rd>, <Rm>{, <rotation>}
{0x068000B0,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // SEL<c> <Rd>, <Rn>, <Rm>
{0x06800010,0x0FF00030,arm_xtra_none,arm_core_data_pack}, // PKHBT<c> <Rd>, <Rn>, <Rm>{, LSL #<imm>} PKHTB<c> <Rd>, <Rn>, <Rm>{, ASR #<imm>}
{0x06800070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // SXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06AF0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // SXTB<c> <Rd>, <Rm>{, <rotation>}
{0x06A00070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // SXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06A00030,0x0FF000F0,arm_xtra_none,arm_core_data_sat}, // SSAT16<c> <Rd>, #<imm>, <Rn>
{0x06B00030,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // REV<c> <Rd>, <Rm>
{0x06B000B0,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // REV16<c> <Rd>, <Rm>
{0x06BF0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // SXTH<c> <Rd>, <Rm>{, <rotation>}
{0x06B00070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // SXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06A00010,0x0FE00030,arm_xtra_none,arm_core_data_sat}, // SSAT<c> <Rd>, #<imm>, <Rn>{, <shift>}
{0x06CF0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // UXTB16<c> <Rd>, <Rm>{, <rotation>}
{0x06C00070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // UXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06EF0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // UXTB<c> <Rd>, <Rm>{, <rotation>}
{0x06E00070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // UXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06E00030,0x0FF000F0,arm_xtra_none,arm_core_data_sat}, // USAT16<c> <Rd>, #<imm4>, <Rn>
{0x06F00030,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // RBIT<c> <Rd>, <Rm>
{0x06F000B0,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // REVSH<c> <Rd>, <Rm>
{0x06FF0070,0x0FFF00F0,arm_xtra_none,arm_core_data_pack}, // UXTH<c> <Rd>, <Rm>{, <rotation>}
{0x06F00070,0x0FF000F0,arm_xtra_none,arm_core_data_pack}, // UXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06E00010,0x0FE00030,arm_xtra_none,arm_core_data_sat}, // USAT<c> <Rd>, #<imm5>, <Rn>{, <shift>}
{0x0700F010,0x0FF0F0D0,arm_xtra_none,arm_core_data_mac}, // SMUAD{X}<c> <Rd>, <Rn>, <Rm>
{0x0700F050,0x0FF0F0D0,arm_xtra_none,arm_core_data_mac}, // SMUSD{X}<c> <Rd>, <Rn>, <Rm>
{0x07000010,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMLAD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07000050,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMLSD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07100010,0x0FF000F0,arm_xtra_none,arm_core_data_div}, // SDIV<c> <Rd>, <Rn>, <Rm>
{0x07300010,0x0FF000F0,arm_xtra_none,arm_core_data_div}, // UDIV<c> <Rd>, <Rn>, <Rm>
{0x07400010,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMLALD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x07400050,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMLSLD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x0750F010,0x0FF0F0D0,arm_xtra_none,arm_core_data_mac}, // SMMUL{R}<c> <Rd>, <Rn>, <Rm>
{0x07500010,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMMLA{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x075000D0,0x0FF000D0,arm_xtra_none,arm_core_data_mac}, // SMMLS{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x0780F010,0x0FF0F0F0,arm_xtra_none,arm_core_data_misc}, // USAD8<c> <Rd>, <Rn>, <Rm>
{0x07800010,0x0FF000F0,arm_xtra_none,arm_core_data_misc}, // USADA8<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07A00050,0x0FE00070,arm_xtra_none,arm_core_data_misc}, // SBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x07C0001E,0x0FE0007E,arm_xtra_none,arm_core_data_misc}, // BFC<c> <Rd>, #<lsb>, #<width>
{0x07C00010,0x0FE00070,arm_xtra_none,arm_core_data_misc}, // BFI<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x07E00050,0x0FE00070,arm_xtra_none,arm_core_data_misc}, // UBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x08000000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // STMDA<c> <Rn>{!}, <registers>
{0x08800000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // STM<c> <Rn>{!}, <registers>
{0x092D0000,0x0FFF0000,arm_xtra_none,arm_core_ldstm}, // PUSH<c> <registers>
{0x09000000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // STMDB<c> <Rn>{!}, <registers>
{0x09800000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // STMIB<c> <Rn>{!}, <registers>
{0x08100000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // LDMDA<c> <Rn>{!}, <registers>
{0x08BD0000,0x0FFF0000,arm_xtra_none,arm_core_ldstm}, // POP<c> <registers>
{0x08900000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // LDM<c> <Rn>{!}, <registers>
{0x09100000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // LDMDB<c> <Rn>{!}, <registers>
{0x09900000,0x0FD00000,arm_xtra_none,arm_core_ldstm}, // LDMIB<c> <Rn>{!}, <registers>
{0x08400000,0x0E500000,arm_xtra_unpriv,arm_core_ldstm}, // STM{<amode>}<c> <Rn>, <registers>
{0x08500000,0x0E508000,arm_xtra_unpriv,arm_core_ldstm}, // LDM{<amode>}<c> <Rn>, <registers without pc>
{0x08508000,0x0E508000,arm_xtra_ret,arm_core_ldstm}, // LDM{<amode>}<c> <Rn>{!}, <registers with pc>
{0x0A000000,0x0F000000,arm_xtra_none,arm_branch}, // B<c> <label>
{0x0B000000,0x0F000000,arm_xtra_none,arm_branch}, // BL<c> <label>
{0x0C400000,0x0FF00000,arm_xtra_none,arm_coproc}, // MCRR<c> <coproc>, <opc1>, <Rt>, <Rt2>, <CRm>
{0x0C500000,0x0FF00000,arm_xtra_none,arm_coproc}, // MRRC<c> <coproc>, <opc>, <Rt>, <Rt2>, <CRm>
{0x0C400A10,0x0FE00FD0,arm_xtra_none,arm_vfp_xfer_reg}, // VMOV<c> <Sm>, <Sm1>, <Rt>, <Rt2> VMOV<c> <Rt>, <Rt2>, <Sm>, <Sm1>
{0x0C400B10,0x0FE00FD0,arm_xtra_none,arm_vfp_xfer_reg}, // VMOV<c> <Dm>, <Rt>, <Rt2> VMOV<c> <Rt>, <Rt2>, <Dm>
{0x0CBD0A00,0x0FBF0F00,arm_xtra_vldm,arm_vfp_ldst_ext}, // VPOP <list>
{0x0CBD0B00,0x0FBF0F00,arm_xtra_vldm,arm_vfp_ldst_ext}, // VPOP <list>
{0x0D2D0A00,0x0FBF0F00,arm_xtra_vstm,arm_vfp_ldst_ext}, // VPUSH<c> <list>
{0x0D2D0B00,0x0FBF0F00,arm_xtra_vstm,arm_vfp_ldst_ext}, // VPUSH<c> <list>
{0x0D000A00,0x0F300F00,arm_xtra_none,arm_vfp_ldst_ext}, // VSTR<c> <Sd>, [<Rn>{, #+/-<imm>}]
{0x0D000B00,0x0F300F00,arm_xtra_none,arm_vfp_ldst_ext}, // VSTR<c> <Dd>, [<Rn>{, #+/-<imm>}]
{0x0D100A00,0x0F300F00,arm_xtra_none,arm_vfp_ldst_ext}, // VLDR<c> <Sd>, [<Rn>{, #+/-<imm>}] VLDR<c> <Sd>, <label> VLDR<c> <Sd>, [PC, #-0] Special case
{0x0D100B00,0x0F300F00,arm_xtra_none,arm_vfp_ldst_ext}, // VLDR<c> <Dd>, [<Rn>{, #+/-<imm>}] VLDR<c> <Dd>, <label> VLDR<c> <Dd>, [PC, #-0] Special case
{0x0C000000,0x0E100000,arm_xtra_none,arm_coproc}, // STC{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} STC{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> STC{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0x0C000A00,0x0E100F00,arm_xtra_none,arm_vfp_ldst_ext}, // VSTM{mode}<c> <Rn>{!}, <list>
{0x0C000B00,0x0E100F00,arm_xtra_none,arm_vfp_ldst_ext}, // VSTM{mode}<c> <Rn>{!}, <list>
{0x0C1F0000,0x0E1F0000,arm_xtra_none,arm_coproc}, // LDC{L}<c> <coproc>, <CRd>, <label> LDC{L}<c> <coproc>, <CRd>, [PC, #-0] Special case LDC{L}<c> <coproc>, <CRd>, [PC], <option>
{0x0C100000,0x0E100000,arm_xtra_none,arm_coproc}, // LDC{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} LDC{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> LDC{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0x0C100A00,0x0E100F00,arm_xtra_none,arm_vfp_ldst_ext}, // VLDM{mode}<c> <Rn>{!}, <list>
{0x0C100B00,0x0E100F00,arm_xtra_none,arm_vfp_ldst_ext}, // VLDM{mode}<c> <Rn>{!}, <list>
{0x0E000A00,0x0FB00E50,arm_xtra_none,arm_v_mac}, // VMLA<c>.F64 <Dd>, <Dn>, <Dm> VMLA<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E000A40,0x0FB00E50,arm_xtra_none,arm_v_mac}, // VMLS<c>.F64 <Dd>, <Dn>, <Dm> VMLS<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E100A00,0x0FB00E10,arm_xtra_fp,arm_v_mac}, // VNMLA<c>.F64 <Dd>, <Dn>, <Dm> VNMLA<c>.F32 <Sd>, <Sn>, <Sm> VNMLS<c>.F64 <Dd>, <Dn>, <Dm> VNMLS<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E200A00,0x0FB00E50,arm_xtra_fp,arm_v_mac}, // VMUL<c>.F64 <Dd>, <Dn>, <Dm> VMUL<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E200A40,0x0FB00E50,arm_xtra_fp,arm_v_mac}, // VNMUL<c>.F64 <Dd>, <Dn>, <Dm> VNMUL<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E300A00,0x0FB00E50,arm_xtra_none,arm_v_par}, // VADD<c>.F64 <Dd>, <Dn>, <Dm> VADD<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E300A40,0x0FB00E50,arm_xtra_fp,arm_v_par}, // VSUB<c>.F64 <Dd>, <Dn>, <Dm> VSUB<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E800A00,0x0FB00E50,?,arm_fp}, // VDIV<c>.F64 <Dd>, <Dn>, <Dm> VDIV<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E900A00,0x0FB00E10,arm_xtra_fp,arm_fp}, // VFNM<y><c>.F64 <Dd>, <Dn>, <Dm> VFNM<y><c>.F32 <Sd>, <Sn>, <Sm>
{0x0EA00A00,0x0FB00E10,arm_xtra_fp,arm_v_mac}, // VFM<y><c>.F64 <Dd>, <Dn>, <Dm> VFM<y><c>.F32 <Sd>, <Sn>, <Sm>
{0x0EB00A40,0x0FBF0ED0,arm_xtra_fp,arm_v_bits}, // VMOV<c>.F64 <Dd>, <Dm> VMOV<c>.F32 <Sd>, <Sm>
{0x0EB00AC0,0x0FBF0ED0,arm_xtra_none,arm_v_misc}, // VABS<c>.F64 <Dd>, <Dm> VABS<c>.F32 <Sd>, <Sm>
{0x0EB10A40,0x0FBF0ED0,arm_xtra_fp,arm_v_misc}, // VNEG<c>.F64 <Dd>, <Dm> VNEG<c>.F32 <Sd>, <Sm>
{0x0EB10AC0,0x0FBF0ED0,arm_xtra_fp,arm_v_misc}, // VSQRT<c>.F64 <Dd>, <Dm> VSQRT<c>.F32 <Sd>, <Sm>
{0x0EB20A40,0x0FBE0E50,arm_xtra_none,arm_fp}, // VCVT<y><c>.F32.F16 <Sd>, <Sm> VCVT<y><c>.F16.F32 <Sd>, <Sm>
{0x0EB40A40,0x0FBF0E50,arm_xtra_none,arm_fp}, // VCMP{E}<c>.F64 <Dd>, <Dm> VCMP{E}<c>.F32 <Sd>, <Sm>
{0x0EB50A40,0x0FBF0E50,arm_xtra_none,arm_fp}, // VCMP{E}<c>.F64 <Dd>, #0.0 VCMP{E}<c>.F32 <Sd>, #0.0
{0x0EB70AC0,0x0FBF0ED0,arm_xtra_none,arm_fp}, // VCVT<c>.F64.F32 <Dd>, <Sm> VCVT<c>.F32.F64 <Sd>, <Dm>
{0x0EBA0A40,0x0FBA0E50,arm_xtra_none,arm_fp}, // VCVT<c>.<Td>.F64 <Dd>, <Dd>, #<fbits> VCVT<c>.<Td>.F32 <Sd>, <Sd>, #<fbits> VCVT<c>.F64.<Td> <Dd>, <Dd>, #<fbits> VCVT<c>.F32.<Td> <Sd>, <Sd>, #<fbits>
{0x0EB80A40,0x0FB80E50,arm_xtra_none,arm_fp}, // VCVT{R}<c>.S32.F64 <Sd>, <Dm> VCVT{R}<c>.S32.F32 <Sd>, <Sm> VCVT{R}<c>.U32.F64 <Sd>, <Dm> VCVT{R}<c>.U32.F32 <Sd>, <Sm> VCVT<c>.F64.<Tm> <Dd>, <Sm> VCVT<c>.F32.<Tm> <Sd>, <Sm>
{0x0EB00A00,0x0FB00E50,arm_xtra_fp,arm_v_bits}, // VMOV<c>.F64 <Dd>, #<imm> VMOV<c>.F32 <Sd>, #<imm>
{0x0E000000,0x0F000010,arm_xtra_none,arm_coproc}, // CDP<c> <coproc>, <opc1>, <CRd>, <CRn>, <CRm>, <opc2>
{0x0E000A10,0x0FE00F10,arm_xtra_none,arm_vfp_xfer_reg}, // VMOV<c> <Sn>, <Rt> VMOV<c> <Rt>, <Sn>
{0x0E000B10,0x0F900F10,arm_xtra_none,arm_vfp_xfer_reg}, // VMOV<c>.<size> <Dd[x]>, <Rt>
{0x0EE10A10,0x0FFF0F10,arm_xtra_none,arm_vfp_xfer_reg}, // VMSR<c> FPSCR, <Rt>
{0x0EE00A10,0x0FF00F10,arm_xtra_mode,arm_vfp_xfer_reg}, // VMSR<c> <spec reg>, <Rt>
{0x0EF10A10,0x0FFF0F10,arm_xtra_none,arm_vfp_xfer_reg}, // VMRS<c> <Rt>, FPSCR
{0x0EF00A10,0x0FF00F10,arm_xtra_mode,arm_vfp_xfer_reg}, // VMRS<c> <Rt>, <spec reg>
{0x0E800B10,0x0F900F50,arm_xtra_none,arm_vfp_xfer_reg}, // VDUP<c>.<size> <Qd>, <Rt> VDUP<c>.<size> <Dd>, <Rt>
{0x0E100B10,0x0F100F10,arm_xtra_none,arm_vfp_xfer_reg}, // VMOV<c>.<dt> <Rt>, <Dn[x]>
{0x0E000010,0x0F100010,arm_xtra_none,arm_coproc}, // MCR<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0x0E100010,0x0F100010,arm_xtra_none,arm_coproc}, // MRC<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0x0F000000,0x0F000000,arm_xtra_none,arm_core_exc} // SVC<c> #<imm24>


#endif /* ARM_DECODE_TABLE_DATA_H_ */
