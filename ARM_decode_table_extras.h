/*
ARM_decode_table_extras.h

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

arm_bra_blx_lbl,
arm_bra_bx_r,
arm_bra_bxj_r,
arm_bra_blx_r,
arm_bra_b_lbl,
arm_bra_bl_lbl,
arm_cop_mcrr2,
arm_cop_mrrc2,
arm_cop_stc2,
arm_cop_ldc2_pc,
arm_cop_ldc2,
arm_cop_mcr2,
arm_cop_mrc2,
arm_cop_cdp2,
arm_cop_mcrr,
arm_cop_mrrc,
arm_cop_stc,
arm_cop_ldc_pc,
arm_cop_ldc,
arm_cop_cdp,
arm_cop_mcr,
arm_cop_mrc,
arm_div_sdiv,
arm_div_udiv,
arm_cmac_mul,
arm_cmac_mla,
arm_cmac_umaal,
arm_cmac_mls,
arm_cmac_umull,
arm_cmac_umlal,
arm_cmac_smull,
arm_cmac_smlal,
arm_cmac_smla,
arm_cmac_smulw,
arm_cmac_smlaw,
arm_cmac_smlal16,
arm_cmac_smul,
arm_cmac_smuad,
arm_cmac_smusd,
arm_cmac_smlad,
arm_cmac_smlsd,
arm_cmac_smlald,
arm_cmac_smlsld,
arm_cmac_smmul,
arm_cmac_smmla,
arm_cmac_smmls,
arm_cmisc_movw,
arm_cmisc_clz,
arm_cmisc_movt,
arm_cmisc_sel,
arm_cmisc_rev,
arm_cmisc_rev16,
arm_cmisc_rbit,
arm_cmisc_revsh,
arm_cmisc_usad8,
arm_cmisc_usada8,
arm_cmisc_sbfx,
arm_cmisc_bfc,
arm_cmisc_bfi,
arm_cmisc_ubfx,
arm_pack_sxtb16,
arm_pack_pkh,
arm_pack_sxtab16,
arm_pack_sxtb,
arm_pack_sxtab,
arm_pack_sxth,
arm_pack_sxtah,
arm_pack_uxtb16,
arm_pack_uxtab16,
arm_pack_uxtb,
arm_pack_uxtab,
arm_pack_uxth,
arm_pack_uxtah,
arm_par_sadd16,
arm_par_sasx,
arm_par_ssax,
arm_par_ssub16,
arm_par_sadd8,
arm_par_ssub8,
arm_par_qadd16,
arm_par_qasx,
arm_par_qsax,
arm_par_qsub16,
arm_par_qadd8,
arm_par_qsub8,
arm_par_shadd16,
arm_par_shasx,
arm_par_shsax,
arm_par_shsub16,
arm_par_shadd8,
arm_par_shsub8,
arm_par_uadd16,
arm_par_uasx,
arm_par_usax,
arm_par_usub16,
arm_par_uadd8,
arm_par_usub8,
arm_par_uqadd16,
arm_par_uqasx,
arm_par_uqsax,
arm_par_uqsub16,
arm_par_uqadd8,
arm_par_uqsub8,
arm_par_uhadd16,
arm_par_uhasx,
arm_par_uhsax,
arm_par_uhsub16,
arm_par_uhadd8,
arm_par_uhsub8,
arm_sat_ssat16,
arm_sat_ssat,
arm_sat_usat16,
arm_sat_usat,
arm_sat_qadd,
arm_sat_qsub,
arm_sat_qdadd,
arm_sat_qdsub,
arm_cdata_lsl_r,
arm_cdata_lsr_r,
arm_cdata_asr_r,
arm_cdata_ror_r,
arm_cdata_lsr_imm,
arm_cdata_asr_imm,
arm_ret_rrx_pc,
arm_ret_lsr_imm,
arm_ret_asr_imm,
arm_ret_ror_imm,
arm_cdata_and_rshr,
arm_cdata_and_r,
arm_cdata_eor_rshr,
arm_cdata_eor_r,
arm_cdata_sub_r_sp,
arm_cdata_sub_rshr,
arm_cdata_sub_r,
arm_cdata_rsb_rshr,
arm_cdata_rsb_r,
arm_cdata_add_r_sp,
arm_cdata_add_rshr,
arm_cdata_add_r,
arm_cdata_adc_rshr,
arm_cdata_adc_r,
arm_cdata_sbc_rshr,
arm_cdata_sbc_r,
arm_cdata_rsc_rshr,
arm_cdata_rsc_r,
arm_cdata_tst_rshr,
arm_cdata_tst_r,
arm_cdata_teq_rshr,
arm_cdata_teq_r,
arm_cdata_cmp_rshr,
arm_cdata_cmp_r,
arm_cdata_cmn_rshr,
arm_cdata_cmn_r,
arm_cdata_orr_rshr,
arm_cdata_orr_r,
arm_cdata_bic_rshr,
arm_cdata_bic_r,
arm_cdata_mvn_rshr,
arm_cdata_mvn_r,
arm_cdata_and_imm,
arm_cdata_eor_imm,
arm_cdata_adr_lblb,
arm_cdata_sub_imm_sp,
arm_cdata_sub_imm,
arm_cdata_rsb_imm,
arm_cdata_adr_lbla,
arm_cdata_add_imm_sp,
arm_cdata_add_imm,
arm_cdata_adc_imm,
arm_cdata_sbc_imm,
arm_cdata_rsc_imm,
arm_cdata_tst_imm,
arm_cdata_teq_imm,
arm_cdata_cmp_imm,
arm_cdata_cmn_imm,
arm_cdata_orr_imm,
arm_cdata_mov_imm,
arm_cdata_bic_imm,
arm_cdata_mvn_imm,
arm_ret_and_r,
arm_ret_eor_r,
arm_ret_sub_r,
arm_ret_rsb_r,
arm_ret_add_r,
arm_ret_adc_r,
arm_ret_sbc_r,
arm_ret_rsc_r,
arm_ret_orr_r,
arm_ret_bic_r,
arm_ret_mvn_r,
arm_ret_eor_imm,
arm_ret_sub_imm,
arm_ret_rsb_imm,
arm_ret_add_imm,
arm_ret_adc_imm,
arm_ret_sbc_imm,
arm_ret_rsc_imm,
arm_ret_mov_imm,
arm_ret_bic_imm,
arm_ret_mvn_imm,
arm_exc_udf,
arm_exc_rfe,
arm_exc_srs,
arm_exc_bkpt,
arm_exc_hvc,
arm_exc_eret,
arm_exc_smc,
arm_exc_svc,
arm_sync_strex,
arm_sync_ldrex,
arm_sync_strexd,
arm_sync_ldrexd,
arm_sync_strexb,
arm_sync_ldrexb,
arm_sync_strexh,
arm_sync_ldrexh,
arm_cldstx_strh_r,
arm_cldstx_ldrd_r,
arm_cldstx_strd_r,
arm_cldstx_ldrh_r,
arm_cldstx_ldrsb_r,
arm_cldstx_ldrsh_r,
arm_cldstx_strh_imm,
arm_cldstx_ldrd_imm,
arm_cldstx_strd_imm,
arm_cldstx_ldrh_lbl,
arm_cldstx_ldrsb_lbl,
arm_cldstx_ldrsh_lbl,
arm_cldstx_ldrh_imm,
arm_cldstx_ldrsb_imm,
arm_cldstx_ldrsh_imm,
arm_cldst_str_imm,
arm_cldst_ldr_lbl,
arm_cldst_ldr_imm,
arm_cldst_strb_imm,
arm_cldst_ldrb_lbl,
arm_cldst_ldrb_imm,
arm_cldst_str_r,
arm_cldst_ldr_r,
arm_cldst_strb_r,
arm_cldst_ldrb_r,
arm_cldstx_ldrd_lbl,
arm_cldstx_strht_r,
arm_cldstx_ldrht_r,
arm_cldstx_ldrsbt_r,
arm_cldstx_ldrsht_r,
arm_cldstx_strht_imm,
arm_cldstx_ldrht_imm,
arm_cldstx_ldrsbt_imm,
arm_cldstx_ldrsht_imm,
arm_cldst_strt_imm,
arm_cldst_ldrt_imm,
arm_cldst_strbt_imm,
arm_cldst_ldrbt_imm,
arm_cldst_strt_r,
arm_cldst_ldrt_r,
arm_cldst_strbt_r,
arm_cldst_ldrbt_r,
arm_cldstm_stmda,
arm_cldstm_stm,
arm_cldstm_push,
arm_cldstm_stmdb,
arm_cldstm_stmib,
arm_cldstm_dlmda,
arm_cldstm_pop,
arm_cldstm_ldm,
arm_cldstm_ldmdb,
arm_cldstm_ldmib,
arm_cldstm_ldm_pc,
arm_cldstm_pop_r,
arm_cldstm_push_r,
arm_cldstm_stm_usr,
arm_cldstm_ldm_usr,
arm_misc_setend,
arm_misc_pli_lbl,
arm_misc_clrex,
arm_misc_dsb,
arm_misc_dmb,
arm_misc_isb,
arm_misc_pld_lbl,
arm_misc_pld_imm,
arm_misc_pli_r,
arm_misc_pld_r,
arm_misc_swp,
arm_misc_sev,
arm_misc_dbg,
arm_cstat_mrs_b,
arm_cstat_msr_b,
arm_cstat_cps,
arm_fp_vdiv,
arm_fp_vfnm,
arm_fp_vcvt_f3216,
arm_fp_vcmp_r,
arm_fp_vcmp_z,
arm_fp_vcvt_f6432,
arm_fp_vcvt_f6432_bc,
arm_fp_vcvt_sf6432,
arm_mux_vbic_vmvn,
arm_mux_wfe_wfi,
arm_mux_vshrn_q_imm,
arm_mux_vrshrn_q_imm,
arm_mux_vshll_i_vmovl,
arm_mux_vorr_i_vmov_i,
arm_mux_lsl_i_mov,
arm_mux_ror_i_rrx,
arm_mux_lsl_i_mov_pc,
arm_mux_vmovn_q,
arm_mux_msr_r_pr,
arm_mux_mrs_r_pr,
arm_mux_msr_i_pr_hints,
arm_mux_vorr_vmov_nm,
arm_mux_vst_type,
arm_mux_vld_type,
arm_vbits_vmov_6432_r,
arm_vbits_vmov_6432_i,
arm_vbits_vmvn,
arm_vbits_vand,
arm_vbits_vbic,
arm_vbits_vorn,
arm_vbits_veor,
arm_vbits_vbsl,
arm_vbits_vbit,
arm_vbits_vbif,
arm_vcmp_vcgt,
arm_vcmp_vcgt_z,
arm_vcmp_vcge_z,
arm_vcmp_vceq_z,
arm_vcmp_vcle_z,
arm_vcmp_vclt_z,
arm_vcmp_vceq,
arm_vcmp_vcge,
arm_vcmp_vcgt_dt,
arm_vcmp_vcge_dt,
arm_vcmp_vtst,
arm_vcmp_vceq_dt,
arm_vcmp_vacge,
arm_vcmp_vacgt,
arm_vmac_vmla_f,
arm_vmac_vmls_f,
arm_vmac_vmul_f,
arm_vmac_vnmlas_f64,
arm_vmac_vmul_f64,
arm_vmac_vnmul_f64,
arm_vmac_vfm_f64,
arm_vmac_vmla_dtx,
arm_vmac_vmlal_dtx,
arm_vmac_vqdmlal_dtx,
arm_vmac_vmls_dtx,
arm_vmac_vmlsl_dtx,
arm_vmac_vqdmlsl_dtx,
arm_vmac_vmlal_dt,
arm_vmac_vqdmlal_dt,
arm_vmac_vmla_dt,
arm_vmac_vmls_dt,
arm_vmac_vmul_dtx,
arm_vmac_vmull_dtx,
arm_vmac_vmlsl_dt,
arm_vmac_vqdmulh_dt,
arm_vmac_vqdmull_dtx,
arm_vmac_vqdmlsl_dt,
arm_vmac_vqrdmulh_dt,
arm_vmac_vqdmulh_dtx,
arm_vmac_vqdmull_dt,
arm_vmac_vqrdmulh_dtx,
arm_vmac_vmull_dt,
arm_vmac_vmul_dt,
arm_vmac_vfm,
arm_vmac_vmla_f64,
arm_vmac_vmls_f64,
arm_vmisc_vabs_dt,
arm_vmisc_vneg_dt,
arm_vmisc_vneg_f64,
arm_vmisc_vsqrt_f64,
arm_vmisc_vswp,
arm_vmisc_vtrn,
arm_vmisc_vuzp,
arm_vmisc_vzip,
arm_vmisc_vrev,
arm_vmisc_vcls_dt,
arm_vmisc_vclz_dt,
arm_vmisc_vcnt,
arm_vmisc_vabal_dt,
arm_vmisc_vrecpe_dt,
arm_vmisc_vrsqrte_dt,
arm_vmisc_vmax_dt,
arm_vmisc_vqabs_dt,
arm_vmisc_vqneg_dt,
arm_vmisc_vabd_dt,
arm_vmisc_vabdl_dt,
arm_vmisc_vcvt_f3216,
arm_vmisc_vcvt_tdtm,
arm_vmisc_vpmax_dt,
arm_vmisc_vtbl,
arm_vmisc_vtbx,
arm_vmisc_vdup_x,
arm_vmisc_vabd_f,
arm_vmisc_vmax_f,
arm_vmisc_vmin_f,
arm_vmisc_vpmax_f,
arm_vmisc_vpmin_f,
arm_vmisc_vext_imm,
arm_vmisc_vmin_dt,
arm_vmisc_vaba_dt,
arm_vmisc_vpmin_dt,
arm_vmisc_vrecps_f,
arm_vmisc_vrsqrts_f,
arm_vmisc_vcvt_tdtmb,
arm_vmisc_vabs_f64,
arm_vpar_vadd_f,
arm_vpar_vsub_f,
arm_vpar_vsub_f64,
arm_vpar_vhadd,
arm_vpar_vrhadd,
arm_vpar_vadd_lw_dt,
arm_vpar_vpaddl_dt,
arm_vpar_vhsub,
arm_vpar_vsubl_dt,
arm_vpar_vaddhn_dt,
arm_vpar_vraddhn_dt,
arm_vpar_vsubhn_dt,
arm_vpar_vpadal_dt,
arm_vpar_vrsubhn_dt,
arm_vpar_vadd_dt,
arm_vpar_vsub_dt,
arm_vpar_vpadd_f,
arm_vpar_vqadd_dt,
arm_vpar_vqsub_dt,
arm_vpar_vpadd_dt,
arm_vpar_vadd_f64,
arm_vsh_vshll_imm,
arm_vsh_vshl_dt,
arm_vsh_vrshl_dt,
arm_vsh_vsra_dt_imm,
arm_vsh_vrshr_dt_imm,
arm_vsh_vrsra_dt_imm,
arm_vsh_vsri_imm,
arm_vsh_vqshl_dt,
arm_vsh_vshl_imm,
arm_vsh_vsli_imm,
arm_vsh_vqrshl_dt,
arm_vsh_vqshl_dt_imm,
arm_vldste_vld1_all,
arm_vldste_vld2_all,
arm_vldste_vld3_all,
arm_vldste_vld4_all,
arm_vldste_vst1_one,
arm_vldste_vst2_one,
arm_vldste_vst3_one,
arm_vldste_vst4_one,
arm_vldste_vld1_one,
arm_vldste_vld2_one,
arm_vldste_vld3_one,
arm_vldste_vld4_one,
arm_vldstx_vstr_s_imm,
arm_vldstx_vstr_d_imm,
arm_vldstx_vldr_s_imm,
arm_vldstx_vldr_d_imm,
arm_vldstx_vstm32,
arm_vldstx_vstm64,
arm_vldstx_vldm32,
arm_vldstx_vldm64,
arm_vldstx_vpop32,
arm_vldstx_vpop64,
arm_vldstx_vpush32,
arm_vldstx_vpush64,
arm_vfpxfer_vmsr_r,
arm_vfpxfer_vmrs_r,
arm_vfpxfer_vmov_ss,
arm_vfpxfer_vmov_d,
arm_vfpxfer_vmov_s,
arm_vfpxfer_vmov_dx,
arm_vfpxfer_vmsr_fpscr,
arm_vfpxfer_vmrs_fpscr,
arm_vfpxfer_vdup,
arm_vfpxfer_vmov_dt_dx,

