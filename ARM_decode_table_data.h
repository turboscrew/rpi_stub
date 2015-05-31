{0x01200020,0x0FF000F0,arm_bra_bx_r,arm_branch}, // BXJ<c> <Rm>
{0x01200010,0x0FF000F0,arm_bra_blx_lbl,arm_branch}, // BX<c> <Rm>
{0x01200030,0x0FF000F0,arm_bra_bxj_r,arm_branch}, // BLX<c> <Rm>
{0x0B000000,0x0F000000,arm_bra_b_lbl,arm_branch}, // BL<c> <label>
{0x0A000000,0x0F000000,arm_bra_blx_r,arm_branch}, // B<c> <label>
{0xFA000000,0xFE000000,arm_bra_bl_lbl,arm_branch}, // BLX <label>
{0x0C400000,0x0FF00000,arm_cop_mcrr2,arm_coproc}, // MCRR<c> <coproc>, <opc1>, <Rt>, <Rt2>, <CRm>
{0x0C500000,0x0FF00000,arm_cop_mrrc2,arm_coproc}, // MRRC<c> <coproc>, <opc>, <Rt>, <Rt2>, <CRm>
{0x0C100000,0x0E100000,arm_cop_ldc2,arm_coproc}, // LDC{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} LDC{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> LDC{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0x0C1F0000,0x0E1F0000,arm_cop_ldc2_pc,arm_coproc}, // LDC{L}<c> <coproc>, <CRd>, <label> LDC{L}<c> <coproc>, <CRd>, [PC, #-0] Special case LDC{L}<c> <coproc>, <CRd>, [PC], <option>
{0x0C000000,0x0E100000,arm_cop_stc2,arm_coproc}, // STC{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} STC{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> STC{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0x0E000000,0x0F000010,arm_cop_stc,arm_coproc}, // CDP<c> <coproc>, <opc1>, <CRd>, <CRn>, <CRm>, <opc2>
{0x0E100010,0x0F100010,arm_cop_ldc,arm_coproc}, // MRC<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0x0E000010,0x0F100010,arm_cop_ldc_pc,arm_coproc}, // MCR<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0xFC000000,0xFE100000,arm_cop_cdp2,arm_coproc}, // STC2{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} STC2{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> STC2{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0xFC400000,0xFFF00000,arm_cop_mcr2,arm_coproc}, // MCRR2<c> <coproc>, <opc1>, <Rt>, <Rt2>, <CRm>
{0xFC1F0000,0xFE1F0000,arm_cop_mcrr,arm_coproc}, // LDC2{L}<c> <coproc>, <CRd>, <label> LDC2{L}<c> <coproc>, <CRd>, [PC, #-0] Special case LDC2{L}<c> <coproc>, <CRd>, [PC], <option>
{0xFC500000,0xFFF00000,arm_cop_mrc2,arm_coproc}, // MRRC2<c> <coproc>, <opc>, <Rt>, <Rt2>, <CRm>
{0xFC100000,0xFE100000,arm_cop_mrrc,arm_coproc}, // LDC2{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm>]{!} LDC2{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm> LDC2{L}<c> <coproc>, <CRd>, [<Rn>], <option>
{0xFE000010,0xFF100010,arm_cop_cdp,arm_coproc}, // MCR2<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0xFE100010,0xFF100010,arm_cop_mcr,arm_coproc}, // MRC2<c> <coproc>, <opc1>, <Rt>, <CRn>, <CRm>{, <opc2>}
{0xFE000000,0xFF000010,arm_cop_mrc,arm_coproc}, // CDP2<c> <coproc>, <opc1>, <CRd>, <CRn>, <CRm>, <opc2>
{0x01A00040,0x0FE00070,arm_cdata_asr_imm,arm_core_data_bit}, // ASR{S}<c> <Rd>, <Rm>, #<imm>
{0x01A00020,0x0FE00070,arm_cdata_lsr_imm,arm_core_data_bit}, // LSR{S}<c> <Rd>, <Rm>, #<imm>
{0x01A0F040,0x0FE0F070,arm_ret_asr_imm,arm_core_data_bit}, // ASR{S}<c> PC, <Rm>, #<imm>
{0x01A0F020,0x0FE0F070,arm_ret_lsr_imm,arm_core_data_bit}, // LSR{S}<c> PC, <Rm>, #<imm>
{0x01A0F060,0x0FE0F070,arm_ret_ror_imm,arm_core_data_bit}, // ROR{S}<c> PC, <Rm>, #<imm>
{0x01B0F060,0x0FF0FFF0,arm_ret_rrx_pc,arm_core_data_bit}, // RRXS<c> PC, <Rm>
{0x01A00050,0x0FE000F0,arm_cdata_asr_r,arm_core_data_bit}, // ASR{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00010,0x0FE000F0,arm_cdata_lsl_r,arm_core_data_bit}, // LSL{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00030,0x0FE000F0,arm_cdata_lsr_r,arm_core_data_bit}, // LSR{S}<c> <Rd>, <Rn>, <Rm>
{0x01A00070,0x0FE000F0,arm_cdata_ror_r,arm_core_data_bit}, // ROR{S}<c> <Rd>, <Rn>, <Rm>
{0x07100010,0x0FF000F0,arm_div_sdiv,arm_core_data_div}, // SDIV<c> <Rd>, <Rn>, <Rm>
{0x07300010,0x0FF000F0,arm_div_udiv,arm_core_data_div}, // UDIV<c> <Rd>, <Rn>, <Rm>
{0x01600080,0x0FF00090,arm_cmac_smul,arm_core_data_mac}, // SMUL<x><y><c> <Rd>, <Rn>, <Rm>
{0x012000A0,0x0FF000B0,arm_cmac_smulw,arm_core_data_mac}, // SMULW<y><c> <Rd>, <Rn>, <Rm>
{0x00000090,0x0FE000F0,arm_cmac_mul,arm_core_data_mac}, // MUL{S}<c> <Rd>, <Rn>, <Rm>
{0x0750F010,0x0FF0F0D0,arm_cmac_smmul,arm_core_data_mac}, // SMMUL{R}<c> <Rd>, <Rn>, <Rm>
{0x0700F010,0x0FF0F0D0,arm_cmac_smuad,arm_core_data_mac}, // SMUAD{X}<c> <Rd>, <Rn>, <Rm>
{0x0700F050,0x0FF0F0D0,arm_cmac_smusd,arm_core_data_mac}, // SMUSD{X}<c> <Rd>, <Rn>, <Rm>
{0x01000080,0x0FF00090,arm_cmac_smla,arm_core_data_mac}, // SMLA<x><y><c> <Rd>, <Rn>, <Rm>, <Ra>
{0x01200080,0x0FF000B0,arm_cmac_smlaw,arm_core_data_mac}, // SMLAW<y><c> <Rd>, <Rn>, <Rm>, <Ra>
{0x00200090,0x0FE000F0,arm_cmac_mla,arm_core_data_mac}, // MLA{S}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x00600090,0x0FF000F0,arm_cmac_mls,arm_core_data_mac}, // MLS<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07000010,0x0FF000D0,arm_cmac_smlad,arm_core_data_mac}, // SMLAD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07000050,0x0FF000D0,arm_cmac_smlsd,arm_core_data_mac}, // SMLSD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x07500010,0x0FF000D0,arm_cmac_smmla,arm_core_data_mac}, // SMMLA{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x075000D0,0x0FF000D0,arm_cmac_smmls,arm_core_data_mac}, // SMMLS{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x01400080,0x0FF00090,arm_cmac_smlal16,arm_core_data_macd}, // SMLAL<x><y><c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00E00090,0x0FE000F0,arm_cmac_smlal,arm_core_data_macd}, // SMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00C00090,0x0FE000F0,arm_cmac_smull,arm_core_data_macd}, // SMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00400090,0x0FF000F0,arm_cmac_umaal,arm_core_data_macd}, // UMAAL<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00A00090,0x0FE000F0,arm_cmac_umlal,arm_core_data_macd}, // UMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x00800090,0x0FE000F0,arm_cmac_umull,arm_core_data_macd}, // UMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x07400010,0x0FF000D0,arm_cmac_smlald,arm_core_data_macd}, // SMLALD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x07400050,0x0FF000D0,arm_cmac_smlsld,arm_core_data_macd}, // SMLSLD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
{0x01600010,0x0FF000F0,arm_cmisc_movw,arm_core_data_misc}, // CLZ<c> <Rd>, <Rm>
{0x03000000,0x0FF00000,arm_cmisc_clz,arm_core_data_misc}, // MOVW<c> <Rd>, #<imm12>
{0x03400000,0x0FF00000,arm_cmisc_movt,arm_core_data_misc}, // MOVT<c> <Rd>, #<imm12>
{0x07C0001E,0x0FE0007E,arm_cmisc_bfc,arm_core_data_misc}, // BFC<c> <Rd>, #<lsb>, #<width>
{0x07C00010,0x0FE00070,arm_cmisc_bfi,arm_core_data_misc}, // BFI<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x06F00030,0x0FF000F0,arm_cmisc_rbit,arm_core_data_misc}, // RBIT<c> <Rd>, <Rm>
{0x06B00030,0x0FF000F0,arm_cmisc_rev,arm_core_data_misc}, // REV<c> <Rd>, <Rm>
{0x06B000B0,0x0FF000F0,arm_cmisc_rev16,arm_core_data_misc}, // REV16<c> <Rd>, <Rm>
{0x06F000B0,0x0FF000F0,arm_cmisc_revsh,arm_core_data_misc}, // REVSH<c> <Rd>, <Rm>
{0x07A00050,0x0FE00070,arm_cmisc_sbfx,arm_core_data_misc}, // SBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x068000B0,0x0FF000F0,arm_cmisc_sel,arm_core_data_misc}, // SEL<c> <Rd>, <Rn>, <Rm>
{0x07E00050,0x0FE00070,arm_cmisc_ubfx,arm_core_data_misc}, // UBFX<c> <Rd>, <Rn>, #<lsb>, #<width>
{0x0780F010,0x0FF0F0F0,arm_cmisc_usad8,arm_core_data_misc}, // USAD8<c> <Rd>, <Rn>, <Rm>
{0x07800010,0x0FF000F0,arm_cmisc_usada8,arm_core_data_misc}, // USADA8<c> <Rd>, <Rn>, <Rm>, <Ra>
{0x06800010,0x0FF00030,arm_pack_pkh,arm_core_data_pack}, // PKHBT<c> <Rd>, <Rn>, <Rm>{, LSL #<imm>} PKHTB<c> <Rd>, <Rn>, <Rm>{, ASR #<imm>}
{0x06A00070,0x0FF000F0,arm_pack_sxtab,arm_core_data_pack}, // SXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06800070,0x0FF000F0,arm_pack_sxtab16,arm_core_data_pack}, // SXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06B00070,0x0FF000F0,arm_pack_sxtah,arm_core_data_pack}, // SXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06AF0070,0x0FFF00F0,arm_pack_sxtb,arm_core_data_pack}, // SXTB<c> <Rd>, <Rm>{, <rotation>}
{0x068F0070,0x0FFF00F0,arm_pack_sxtb16,arm_core_data_pack}, // SXTB16<c> <Rd>, <Rm>{, <rotation>}
{0x06BF0070,0x0FFF00F0,arm_pack_sxth,arm_core_data_pack}, // SXTH<c> <Rd>, <Rm>{, <rotation>}
{0x06E00070,0x0FF000F0,arm_pack_uxtab,arm_core_data_pack}, // UXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06C00070,0x0FF000F0,arm_pack_uxtab16,arm_core_data_pack}, // UXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06F00070,0x0FF000F0,arm_pack_uxtah,arm_core_data_pack}, // UXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
{0x06EF0070,0x0FFF00F0,arm_pack_uxtb,arm_core_data_pack}, // UXTB<c> <Rd>, <Rm>{, <rotation>}
{0x06CF0070,0x0FFF00F0,arm_pack_uxtb16,arm_core_data_pack}, // UXTB16<c> <Rd>, <Rm>{, <rotation>}
{0x06FF0070,0x0FFF00F0,arm_pack_uxth,arm_core_data_pack}, // UXTH<c> <Rd>, <Rm>{, <rotation>}
{0x06200010,0x0FF000F0,arm_par_qadd16,arm_core_data_par}, // QADD16<c> <Rd>, <Rn>, <Rm>
{0x06200090,0x0FF000F0,arm_par_qadd8,arm_core_data_par}, // QADD8<c> <Rd>, <Rn>, <Rm>
{0x06200030,0x0FF000F0,arm_par_qasx,arm_core_data_par}, // QASX<c> <Rd>, <Rn>, <Rm>
{0x06200050,0x0FF000F0,arm_par_qsax,arm_core_data_par}, // QSAX<c> <Rd>, <Rn>, <Rm>
{0x06200070,0x0FF000F0,arm_par_qsub16,arm_core_data_par}, // QSUB16<c> <Rd>, <Rn>, <Rm>
{0x062000F0,0x0FF000F0,arm_par_qsub8,arm_core_data_par}, // QSUB8<c> <Rd>, <Rn>, <Rm>
{0x06100010,0x0FF000F0,arm_par_sadd16,arm_core_data_par}, // SADD16<c> <Rd>, <Rn>, <Rm>
{0x06100090,0x0FF000F0,arm_par_sadd8,arm_core_data_par}, // SADD8<c> <Rd>, <Rn>, <Rm>
{0x06100030,0x0FF000F0,arm_par_sasx,arm_core_data_par}, // SASX<c> <Rd>, <Rn>, <Rm>
{0x06300010,0x0FF000F0,arm_par_shadd16,arm_core_data_par}, // SHADD16<c> <Rd>, <Rn>, <Rm>
{0x06300090,0x0FF000F0,arm_par_shadd8,arm_core_data_par}, // SHADD8<c> <Rd>, <Rn>, <Rm>
{0x06300030,0x0FF000F0,arm_par_shasx,arm_core_data_par}, // SHASX<c> <Rd>, <Rn>, <Rm>
{0x06300050,0x0FF000F0,arm_par_shsax,arm_core_data_par}, // SHSAX<c> <Rd>, <Rn>, <Rm>
{0x06300070,0x0FF000F0,arm_par_shsub16,arm_core_data_par}, // SHSUB16<c> <Rd>, <Rn>, <Rm>
{0x063000F0,0x0FF000F0,arm_par_shsub8,arm_core_data_par}, // SHSUB8<c> <Rd>, <Rn>, <Rm>
{0x06100050,0x0FF000F0,arm_par_ssax,arm_core_data_par}, // SSAX<c> <Rd>, <Rn>, <Rm>
{0x06100070,0x0FF000F0,arm_par_ssub16,arm_core_data_par}, // SSUB16<c> <Rd>, <Rn>, <Rm>
{0x061000F0,0x0FF000F0,arm_par_ssub8,arm_core_data_par}, // SSUB8<c> <Rd>, <Rn>, <Rm>
{0x06500010,0x0FF000F0,arm_par_uadd16,arm_core_data_par}, // UADD16<c> <Rd>, <Rn>, <Rm>
{0x06500090,0x0FF000F0,arm_par_uadd8,arm_core_data_par}, // UADD8<c> <Rd>, <Rn>, <Rm>
{0x06500030,0x0FF000F0,arm_par_uasx,arm_core_data_par}, // UASX<c> <Rd>, <Rn>, <Rm>
{0x06700010,0x0FF000F0,arm_par_uhadd16,arm_core_data_par}, // UHADD16<c> <Rd>, <Rn>, <Rm>
{0x06700090,0x0FF000F0,arm_par_uhadd8,arm_core_data_par}, // UHADD8<c> <Rd>, <Rn>, <Rm>
{0x06700030,0x0FF000F0,arm_par_uhasx,arm_core_data_par}, // UHASX<c> <Rd>, <Rn>, <Rm>
{0x06700050,0x0FF000F0,arm_par_uhsax,arm_core_data_par}, // UHSAX<c> <Rd>, <Rn>, <Rm>
{0x06700070,0x0FF000F0,arm_par_uhsub16,arm_core_data_par}, // UHSUB16<c> <Rd>, <Rn>, <Rm>
{0x067000F0,0x0FF000F0,arm_par_uhsub8,arm_core_data_par}, // UHSUB8<c> <Rd>, <Rn>, <Rm>
{0x06600010,0x0FF000F0,arm_par_uqadd16,arm_core_data_par}, // UQADD16<c> <Rd>, <Rn>, <Rm>
{0x06600090,0x0FF000F0,arm_par_uqadd8,arm_core_data_par}, // UQADD8<c> <Rd>, <Rn>, <Rm>
{0x06600030,0x0FF000F0,arm_par_uqasx,arm_core_data_par}, // UQASX<c> <Rd>, <Rn>, <Rm>
{0x06600050,0x0FF000F0,arm_par_uqsax,arm_core_data_par}, // UQSAX<c> <Rd>, <Rn>, <Rm>
{0x06600070,0x0FF000F0,arm_par_uqsub16,arm_core_data_par}, // UQSUB16<c> <Rd>, <Rn>, <Rm>
{0x066000F0,0x0FF000F0,arm_par_uqsub8,arm_core_data_par}, // UQSUB8<c> <Rd>, <Rn>, <Rm>
{0x06500050,0x0FF000F0,arm_par_usax,arm_core_data_par}, // USAX<c> <Rd>, <Rn>, <Rm>
{0x06500070,0x0FF000F0,arm_par_usub16,arm_core_data_par}, // USUB16<c> <Rd>, <Rn>, <Rm>
{0x065000F0,0x0FF000F0,arm_par_usub8,arm_core_data_par}, // USUB8<c> <Rd>, <Rn>, <Rm>
{0x01000050,0x0FF000F0,arm_sat_qadd,arm_core_data_sat}, // QADD<c> <Rd>, <Rm>, <Rn>
{0x01400050,0x0FF000F0,arm_sat_qdadd,arm_core_data_sat}, // QDADD<c> <Rd>, <Rm>, <Rn>
{0x01600050,0x0FF000F0,arm_sat_qdsub,arm_core_data_sat}, // QDSUB<c> <Rd>, <Rm>, <Rn>
{0x01200050,0x0FF000F0,arm_sat_qsub,arm_core_data_sat}, // QSUB<c> <Rd>, <Rm>, <Rn>
{0x06A00010,0x0FE00030,arm_sat_ssat,arm_core_data_sat}, // SSAT<c> <Rd>, #<imm>, <Rn>{, <shift>}
{0x06A00030,0x0FF000F0,arm_sat_ssat16,arm_core_data_sat}, // SSAT16<c> <Rd>, #<imm>, <Rn>
{0x06E00010,0x0FE00030,arm_sat_usat,arm_core_data_sat}, // USAT<c> <Rd>, #<imm5>, <Rn>{, <shift>}
{0x06E00030,0x0FF000F0,arm_sat_usat16,arm_core_data_sat}, // USAT16<c> <Rd>, #<imm4>, <Rn>
{0x02A00000,0x0FE00000,arm_cdata_adc_imm,arm_core_data_std_i}, // ADC{S}<c> <Rd>, <Rn>, #<const>
{0x02800000,0x0FE00000,arm_cdata_add_imm,arm_core_data_std_i}, // ADD{S}<c> <Rd>, <Rn>, #<const>
{0x028D0000,0x0FEF0000,arm_cdata_add_imm_sp,arm_core_data_std_i}, // ADD{S}<c> <Rd>, SP, #<const>
{0x028F0000,0x0FFF0000,arm_cdata_adr_lbla,arm_core_data_std_i}, // ADR<c> <Rd>, <label> <label> after current instruction
{0x024F0000,0x0FFF0000,arm_cdata_adr_lblb,arm_core_data_std_i}, // ADR<c> <Rd>, <label> <label> before current instruction SUB <Rd>, PC, #0 Special case for subtraction of zero
{0x02000000,0x0FE00000,arm_cdata_and_imm,arm_core_data_std_i}, // AND{S}<c> <Rd>, <Rn>, #<const>
{0x03C00000,0x0FE00000,arm_cdata_bic_imm,arm_core_data_std_i}, // BIC{S}<c> <Rd>, <Rn>, #<const>
{0x03700000,0x0FF00000,arm_cdata_cmn_imm,arm_core_data_std_i}, // CMN<c> <Rn>, #<const>
{0x03500000,0x0FF00000,arm_cdata_cmp_imm,arm_core_data_std_i}, // CMP<c> <Rn>, #<const>
{0x02200000,0x0FE00000,arm_cdata_eor_imm,arm_core_data_std_i}, // EOR{S}<c> <Rd>, <Rn>, #<const>
{0x03A00000,0x0FE00000,arm_cdata_mov_imm,arm_core_data_std_i}, // MOV{S}<c> <Rd>, #<const>
{0x03E00000,0x0FE00000,arm_cdata_mvn_imm,arm_core_data_std_i}, // MVN{S}<c> <Rd>, #<const>
{0x03800000,0x0FE00000,arm_cdata_orr_imm,arm_core_data_std_i}, // ORR{S}<c> <Rd>, <Rn>, #<const>
{0x02600000,0x0FE00000,arm_cdata_rsb_imm,arm_core_data_std_i}, // RSB{S}<c> <Rd>, <Rn>, #<const>
{0x02E00000,0x0FE00000,arm_cdata_rsc_imm,arm_core_data_std_i}, // RSC{S}<c> <Rd>, <Rn>, #<const>
{0x02C00000,0x0FE00000,arm_cdata_sbc_imm,arm_core_data_std_i}, // SBC{S}<c> <Rd>, <Rn>, #<const>
{0x02400000,0x0FE00000,arm_cdata_sub_imm,arm_core_data_std_i}, // SUB{S}<c> <Rd>, <Rn>, #<const>
{0x024D0000,0x0FEF0000,arm_cdata_sub_imm_sp,arm_core_data_std_i}, // SUB{S}<c> <Rd>, SP, #<const>
{0x03300000,0x0FF00000,arm_cdata_teq_imm,arm_core_data_std_i}, // TEQ<c> <Rn>, #<const>
{0x03100000,0x0FF00000,arm_cdata_tst_imm,arm_core_data_std_i}, // TST<c> <Rn>, #<const>
{0x02A0F000,0x0FE0F000,arm_ret_adc_imm,arm_core_data_std_i}, // ADC{S}<c> PC, <Rn>, #<const>
{0x0280F000,0x0FE0F000,arm_ret_add_imm,arm_core_data_std_i}, // ADD{S}<c> PC, <Rn>, #<const>
{0x03C0F000,0x0FE0F000,arm_ret_bic_imm,arm_core_data_std_i}, // BIC{S}<c> PC, <Rn>, #<const>
{0x0220F000,0x0FE0F000,arm_ret_eor_imm,arm_core_data_std_i}, // EOR{S}<c> PC, <Rn>, #<const>
{0x03A0F000,0x0FE0F000,arm_ret_mov_imm,arm_core_data_std_i}, // MOV{S}<c> PC, #<const>
{0x03E0F000,0x0FE0F000,arm_ret_mvn_imm,arm_core_data_std_i}, // MVN{S}<c> PC, #<const>
{0x0260F000,0x0FE0F000,arm_ret_rsb_imm,arm_core_data_std_i}, // RSB{S}<c> PC, <Rn>, #<const>
{0x02E0F000,0x0FE0F000,arm_ret_rsc_imm,arm_core_data_std_i}, // RSC{S}<c> PC, <Rn>, #<const>
{0x02C0F000,0x0FE0F000,arm_ret_sbc_imm,arm_core_data_std_i}, // SBC{S}<c> PC, <Rn>, #<const>
{0x0240F000,0x0FE0F000,arm_ret_sub_imm,arm_core_data_std_i}, // SUB{S}<c> PC, <Rn>, #<const>
{0x00A0F000,0x0FE0F010,arm_ret_adc_r,arm_core_data_std_r}, // ADC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0080F000,0x0FE0F010,arm_ret_add_r,arm_core_data_std_r}, // ADD{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0000F000,0x0FE0F010,arm_ret_and_r,arm_core_data_std_r}, // AND{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x01C0F000,0x0FE0F010,arm_ret_bic_r,arm_core_data_std_r}, // BIC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0020F000,0x0FE0F010,arm_ret_eor_r,arm_core_data_std_r}, // EOR{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x01E0F000,0x0FE0F010,arm_ret_mvn_r,arm_core_data_std_r}, // MVN{S}<c> PC, <Rm>{, <sift>}
{0x0180F000,0x0FE0F010,arm_ret_orr_r,arm_core_data_std_r}, // ORR{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0060F000,0x0FE0F010,arm_ret_rsb_r,arm_core_data_std_r}, // RSB{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00E0F000,0x0FE0F010,arm_ret_rsc_r,arm_core_data_std_r}, // RSC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00C0F000,0x0FE0F010,arm_ret_sbc_r,arm_core_data_std_r}, // SBC{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x0040F000,0x0FE0F010,arm_ret_sub_r,arm_core_data_std_r}, // SUB{S}<c> PC, <Rn>, <Rm>{, <sift>}
{0x00A00000,0x0FE00010,arm_cdata_adc_r,arm_core_data_std_r}, // ADC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00800000,0x0FE00010,arm_cdata_add_r,arm_core_data_std_r}, // ADD{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x008D0000,0x0FEF0010,arm_cdata_add_r_sp,arm_core_data_std_r}, // ADD{S}<c> <Rd>, SP, <Rm>{, <shift>}
{0x00000000,0x0FE00010,arm_cdata_and_r,arm_core_data_std_r}, // AND{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x01C00000,0x0FE00010,arm_cdata_bic_r,arm_core_data_std_r}, // BIC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x01700000,0x0FF00010,arm_cdata_cmn_r,arm_core_data_std_r}, // CMN<c> <Rn>, <Rm>{, <shift>}
{0x01500000,0x0FF00010,arm_cdata_cmp_r,arm_core_data_std_r}, // CMP<c> <Rn>, <Rm>{, <shift>}
{0x00200000,0x0FE00010,arm_cdata_eor_r,arm_core_data_std_r}, // EOR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x01E00000,0x0FE00010,arm_cdata_mvn_r,arm_core_data_std_r}, // MVN{S}<c> <Rd>, <Rm>{, <shift>}
{0x01800000,0x0FE00010,arm_cdata_orr_r,arm_core_data_std_r}, // ORR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00600000,0x0FE00010,arm_cdata_rsb_r,arm_core_data_std_r}, // RSB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00E00000,0x0FE00010,arm_cdata_rsc_r,arm_core_data_std_r}, // RSC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00C00000,0x0FE00010,arm_cdata_sbc_r,arm_core_data_std_r}, // SBC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x00400000,0x0FE00010,arm_cdata_sub_r,arm_core_data_std_r}, // SUB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
{0x004D0000,0x0FEF0010,arm_cdata_sub_r_sp,arm_core_data_std_r}, // SUB{S}<c> <Rd>, SP, <Rm>{, <shift>}
{0x01300000,0x0FF00010,arm_cdata_teq_r,arm_core_data_std_r}, // TEQ<c> <Rn>, <Rm>{, <shift>}
{0x01100000,0x0FF00010,arm_cdata_tst_r,arm_core_data_std_r}, // TST<c> <Rn>, <Rm>{, <shift>}
{0x00A00010,0x0FE00090,arm_cdata_adc_rshr,arm_core_data_std_sh}, // ADC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00800010,0x0FE00090,arm_cdata_add_rshr,arm_core_data_std_sh}, // ADD{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00000010,0x0FE00090,arm_cdata_and_rshr,arm_core_data_std_sh}, // AND{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01C00010,0x0FE00090,arm_cdata_bic_rshr,arm_core_data_std_sh}, // BIC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01700010,0x0FF00090,arm_cdata_cmn_rshr,arm_core_data_std_sh}, // CMN<c> <Rn>, <Rm>, <type> <Rs>
{0x01500010,0x0FF00090,arm_cdata_cmp_rshr,arm_core_data_std_sh}, // CMP<c> <Rn>, <Rm>, <type> <Rs>
{0x00200010,0x0FE00090,arm_cdata_eor_rshr,arm_core_data_std_sh}, // EOR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01E00010,0x0FE00090,arm_cdata_mvn_rshr,arm_core_data_std_sh}, // MVN{S}<c> <Rd>, <Rm>, <type> <Rs>
{0x01800010,0x0FE00090,arm_cdata_orr_rshr,arm_core_data_std_sh}, // ORR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00600010,0x0FE00090,arm_cdata_rsb_rshr,arm_core_data_std_sh}, // RSB{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00E00010,0x0FE00090,arm_cdata_rsc_rshr,arm_core_data_std_sh}, // RSC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00C00010,0x0FE00090,arm_cdata_sbc_rshr,arm_core_data_std_sh}, // SBC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x00400010,0x0FE00090,arm_cdata_sub_rshr,arm_core_data_std_sh}, // SUB{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
{0x01300010,0x0FF00090,arm_cdata_teq_rshr,arm_core_data_std_sh}, // TEQ<c> <Rn>, <Rm>, <type> <Rs>
{0x01100010,0x0FF00090,arm_cdata_tst_rshr,arm_core_data_std_sh}, // TST<c> <Rn>, <Rm>, <type> <Rs>
{0x01600060,0x0FF000F0,arm_exc_eret,arm_core_exc}, // ERET<c>
{0x01200070,0x0FF000F0,arm_exc_bkpt,arm_core_exc}, // BKPT #<imm12>
{0x01400070,0x0FF000F0,arm_exc_hvc,arm_core_exc}, // HVC #<imm>
{0x01600070,0x0FF000F0,arm_exc_smc,arm_core_exc}, // SMC<c> #<imm4>
{0x0F000000,0x0F000000,arm_exc_svc,arm_core_exc}, // SVC<c> #<imm24>
{0xE7F000F0,0xFFF000F0,arm_exc_udf,arm_core_exc}, // UDF<c> #<imm12>
{0xF8100000,0xFE500000,arm_exc_rfe,arm_core_exc}, // RFE{<amode>} <Rn>{!}
{0xF8400000,0xFE500000,arm_exc_srs,arm_core_exc}, // SRS{<amode>} SP{!}, #<mode>
{0x04100000,0x0E500000,arm_cldst_ldr_imm,arm_core_ldst}, // LDR<c> <Rt>, [<Rn>{, #+/-<imm12>}] LDR<c> <Rt>, [<Rn>], #+/-<imm12> LDR<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x041F0000,0x0E5F0000,arm_cldst_ldr_lbl,arm_core_ldst}, // LDR<c> <Rt>, <label> LDR<c> <Rt>, [PC, #-0] Special case
{0x04500000,0x0E500000,arm_cldst_ldrb_imm,arm_core_ldst}, // LDRB<c> <Rt>, [<Rn>{, #+/-<imm12>}] LDRB<c> <Rt>, [<Rn>], #+/-<imm12> LDRB<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x045F0000,0x0E5F0000,arm_cldst_ldrb_lbl,arm_core_ldst}, // LDRB<c> <Rt>, <label> LDRB<c> <Rt>, [PC, #-0] Special case
{0x04700000,0x0F700000,arm_cldst_ldrbt_imm,arm_core_ldst}, // LDRBT<c> <Rt>, [<Rn>], #+/-<imm12>
{0x04300000,0x0F700000,arm_cldst_ldrt_imm,arm_core_ldst}, // LDRT<c> <Rt>, [<Rn>] {, #+/-<imm12>}
{0x04000000,0x0E500000,arm_cldst_str_imm,arm_core_ldst}, // STR<c> <Rt>, [<Rn>{, #+/-<imm12>}] STR<c> <Rt>, [<Rn>], #+/-<imm12> STR<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x04400000,0x0E500000,arm_cldst_strb_imm,arm_core_ldst}, // STRB<c> <Rt>, [<Rn>{, #+/-<imm12>}] STRB<c> <Rt>, [<Rn>], #+/-<imm12> STRB<c> <Rt>, [<Rn>, #+/-<imm12>]!
{0x04600000,0x0F700000,arm_cldst_strbt_imm,arm_core_ldst}, // STRBT<c> <Rt>, [<Rn>], #+/-<imm12>
{0x04200000,0x0F700000,arm_cldst_strt_imm,arm_core_ldst}, // STRT<c> <Rt>, [<Rn>] {, +/-<imm12>}
{0x06100000,0x0E500010,arm_cldst_ldr_r,arm_core_ldst}, // LDR<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} LDR<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06500000,0x0E500010,arm_cldst_ldrb_r,arm_core_ldst}, // LDRB<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} LDRB<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06700000,0x0F700010,arm_cldst_ldrbt_r,arm_core_ldst}, // LDRBT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06300000,0x0F700010,arm_cldst_ldrt_r,arm_core_ldst}, // LDRT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06000000,0x0E500010,arm_cldst_str_r,arm_core_ldst}, // STR<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} STR<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06400000,0x0E500010,arm_cldst_strb_r,arm_core_ldst}, // STRB<c> <Rt>, [<Rn>,+/-<Rm>{, <shift>}]{!} STRB<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06600000,0x0F700010,arm_cldst_strbt_r,arm_core_ldst}, // STRBT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x06200000,0x0F700010,arm_cldst_strt_r,arm_core_ldst}, // STRT<c> <Rt>, [<Rn>],+/-<Rm>{, <shift>}
{0x049D0004,0x0FFF0FFE,arm_cldstm_pop_r,arm_core_ldstm}, // POP<c> <registers>, <Rt>
{0x052D0004,0x0FFF0FFE,arm_cldstm_push_r,arm_core_ldstm}, // PUSH<c> <registers>, <Rt>
{0x08100000,0x0FD00000,arm_cldstm_dlmda,arm_core_ldstm}, // LDMDA<c> <Rn>{!}, <registers>
{0x08900000,0x0FD00000,arm_cldstm_ldm,arm_core_ldstm}, // LDM<c> <Rn>{!}, <registers>
{0x08508000,0x0E508000,arm_cldstm_ldm_pc,arm_core_ldstm}, // LDM{<amode>}<c> <Rn>{!}, <registers with pc>
{0x08500000,0x0E508000,arm_cldstm_ldm_usr,arm_core_ldstm}, // LDM{<amode>}<c> <Rn>, <registers without pc>
{0x09100000,0x0FD00000,arm_cldstm_ldmdb,arm_core_ldstm}, // LDMDB<c> <Rn>{!}, <registers>
{0x09900000,0x0FD00000,arm_cldstm_ldmib,arm_core_ldstm}, // LDMIB<c> <Rn>{!}, <registers>
{0x08BD0000,0x0FFF0000,arm_cldstm_pop,arm_core_ldstm}, // POP<c> <registers>
{0x092D0000,0x0FFF0000,arm_cldstm_push,arm_core_ldstm}, // PUSH<c> <registers>
{0x08800000,0x0FD00000,arm_cldstm_stm,arm_core_ldstm}, // STM<c> <Rn>{!}, <registers>
{0x08400000,0x0E500000,arm_cldstm_stm_usr,arm_core_ldstm}, // STM{<amode>}<c> <Rn>, <registers>
{0x08000000,0x0FD00000,arm_cldstm_stmda,arm_core_ldstm}, // STMDA<c> <Rn>{!}, <registers>
{0x09000000,0x0FD00000,arm_cldstm_stmdb,arm_core_ldstm}, // STMDB<c> <Rn>{!}, <registers>
{0x09800000,0x0FD00000,arm_cldstm_stmib,arm_core_ldstm}, // STMIB<c> <Rn>{!}, <registers>
{0x004000D0,0x0E5000F0,arm_cldstx_ldrd_imm,arm_core_ldstrd}, // LDRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm8>}] LDRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm8> LDRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm8>]!
{0x004F00D0,0x0E5F00F0,arm_cldstx_ldrd_lbl,arm_core_ldstrd}, // LDRD<c> <Rt>, <Rt2>, <label> LDRD<c> <Rt>, <Rt2>, [PC, #-0] Special case
{0x000000D0,0x0E5000F0,arm_cldstx_ldrd_r,arm_core_ldstrd}, // LDRD<c> <Rt>, <Rt2>, [<Rn>,+/-<Rm>]{!} LDRD<c> <Rt>, <Rt2>, [<Rn>],+/-<Rm>
{0x004000F0,0x0E5000F0,arm_cldstx_strd_imm,arm_core_ldstrd}, // STRD<c> <Rt>, <Rt2>, [<Rn>{, #+/-<imm8>}] STRD<c> <Rt>, <Rt2>, [<Rn>], #+/-<imm8> STRD<c> <Rt>, <Rt2>, [<Rn>, #+/-<imm8>]!
{0x000000F0,0x0E5000F0,arm_cldstx_strd_r,arm_core_ldstrd}, // STRD<c> <Rt>, <Rt2>, [<Rn>,+/-<Rm>]{!} STRD<c> <Rt>, <Rt2>, [<Rn>],+/-<Rm>
{0x01900090,0x0FF000F0,arm_sync_ldrex,arm_core_ldstrex}, // LDREX<c> <Rt>, [<Rn>]
{0x01D00090,0x0FF000F0,arm_sync_ldrexb,arm_core_ldstrex}, // LDREXB<c> <Rt>, [<Rn>]
{0x01B00090,0x0FF000F0,arm_sync_ldrexd,arm_core_ldstrex}, // LDREXD<c> <Rt>, <Rt2>, [<Rn>]
{0x01F00090,0x0FF000F0,arm_sync_ldrexh,arm_core_ldstrex}, // LDREXH<c> <Rt>, [<Rn>]
{0x01800090,0x0FF000F0,arm_sync_strex,arm_core_ldstrex}, // STREX<c> <Rd>, <Rt>, [<Rn>]
{0x01C00090,0x0FF000F0,arm_sync_strexb,arm_core_ldstrex}, // STREXB<c> <Rd>, <Rt>, [<Rn>]
{0x01A00090,0x0FF000F0,arm_sync_strexd,arm_core_ldstrex}, // STREXD<c> <Rd>, <Rt>, <Rt2>, [<Rn>]
{0x01E00090,0x0FF000F0,arm_sync_strexh,arm_core_ldstrex}, // STREXH<c> <Rd>, <Rt>, [<Rn>]
{0x005000B0,0x0E5000F0,arm_cldstx_ldrh_imm,arm_core_ldstrh}, // LDRH<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRH<c> <Rt>, [<Rn>], #+/-<imm8> LDRH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x005F00B0,0x0E5F00F0,arm_cldstx_ldrh_lbl,arm_core_ldstrh}, // LDRH<c> <Rt>, <label> LDRH<c> <Rt>, [PC, #-0] Special case
{0x001000B0,0x0E5000F0,arm_cldstx_ldrh_r,arm_core_ldstrh}, // LDRH<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRH<c> <Rt>, [<Rn>],+/-<Rm>
{0x007000B0,0x0F7000F0,arm_cldstx_ldrht_imm,arm_core_ldstrh}, // LDRHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x003000B0,0x0F7000F0,arm_cldstx_ldrht_r,arm_core_ldstrh}, // LDRHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x004000B0,0x0E5000F0,arm_cldstx_strh_imm,arm_core_ldstrh}, // STRH<c> <Rt>, [<Rn>{, #+/-<imm8>}] STRH<c> <Rt>, [<Rn>], #+/-<imm8> STRH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x000000B0,0x0E5000F0,arm_cldstx_strh_r,arm_core_ldstrh}, // STRH<c> <Rt>, [<Rn>,+/-<Rm>]{!} STRH<c> <Rt>, [<Rn>],+/-<Rm>
{0x006000B0,0x0F7000F0,arm_cldstx_strht_imm,arm_core_ldstrh}, // STRHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x002000B0,0x0F7000F0,arm_cldstx_strht_r,arm_core_ldstrh}, // STRHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x005000D0,0x0E5000F0,arm_cldstx_ldrsb_imm,arm_core_ldstrsb}, // LDRSB<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRSB<c> <Rt>, [<Rn>], #+/-<imm8> LDRSB<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x005F00D0,0x0E5F00F0,arm_cldstx_ldrsb_lbl,arm_core_ldstrsb}, // LDRSB<c> <Rt>, <label> LDRSB<c> <Rt>, [PC, #-0] Special case
{0x001000D0,0x0E5000F0,arm_cldstx_ldrsb_r,arm_core_ldstrsb}, // LDRSB<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRSB<c> <Rt>, [<Rn>],+/-<Rm>
{0x007000D0,0x0F7000F0,arm_cldstx_ldrsbt_imm,arm_core_ldstrsb}, // LDRSBT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x003000D0,0x0F7000F0,arm_cldstx_ldrsbt_r,arm_core_ldstrsb}, // LDRSBT<c> <Rt>, [<Rn>], +/-<Rm>
{0x005000F0,0x0E5000F0,arm_cldstx_ldrsh_imm,arm_core_ldstsh}, // LDRSH<c> <Rt>, [<Rn>{, #+/-<imm8>}] LDRSH<c> <Rt>, [<Rn>], #+/-<imm8> LDRSH<c> <Rt>, [<Rn>, #+/-<imm8>]!
{0x005F00F0,0x0E5F00F0,arm_cldstx_ldrsh_lbl,arm_core_ldstsh}, // LDRSH<c> <Rt>, <label> LDRSH<c> <Rt>, [PC, #-0] Special case
{0x001000F0,0x0E5000F0,arm_cldstx_ldrsh_r,arm_core_ldstsh}, // LDRSH<c> <Rt>, [<Rn>,+/-<Rm>]{!} LDRSH<c> <Rt>, [<Rn>],+/-<Rm>
{0x007000F0,0x0F7000F0,arm_cldstx_ldrsht_imm,arm_core_ldstsh}, // LDRSHT<c> <Rt>, [<Rn>] {, #+/-<imm8>}
{0x003000F0,0x0F7000F0,arm_cldstx_ldrsht_r,arm_core_ldstsh}, // LDRSHT<c> <Rt>, [<Rn>], +/-<Rm>
{0x01000090,0x0FB000F0,arm_misc_swp,arm_core_misc}, // SWP{B}<c> <Rt>, <Rt2>, [<Rn>]
{0x03200004,0x0FFF00FE,arm_misc_sev,arm_core_misc}, // SEV<c>
{0x032000F0,0x0FFF00F0,arm_misc_dbg,arm_core_misc}, // DBG<c> #<option>
{0xF1010000,0xFFF100F0,arm_misc_setend,arm_core_misc}, // SETEND <endian specifier>
{0xF5700010,0xFFF000F0,arm_misc_clrex,arm_core_misc}, // CLREX
{0xF5700050,0xFFF000F0,arm_misc_dmb,arm_core_misc}, // DMB <option>
{0xF5700040,0xFFF000F0,arm_misc_dsb,arm_core_misc}, // DSB <option>
{0xF5700060,0xFFF000F0,arm_misc_isb,arm_core_misc}, // ISB <option>
{0xF5100000,0xFF300000,arm_misc_pld_imm,arm_core_misc}, // PLD{W} [<Rn>, #+/-<imm12>]
{0xF51F0000,0xFF3F0000,arm_misc_pld_lbl,arm_core_misc}, // PLD <label> PLD [PC, #-0] Special case
{0xF4500000,0xFF700000,arm_misc_pli_lbl,arm_core_misc}, // PLI [<Rn>, #+/-<imm12>] PLI <label> PLI [PC, #-0] Special case
{0xF7100000,0xFF300010,arm_misc_pld_r,arm_core_misc}, // PLD{W} [<Rn>,+/-<Rm>{, <shift>}]
{0xF6500000,0xFF700010,arm_misc_pli_r,arm_core_misc}, // PLI [<Rn>,+/-<Rm>{, <shift>}]
{0x01000200,0x0FB002F0,arm_cstat_mrs_b,arm_core_status}, // MRS<c> <Rd>, <banked reg>
{0x01200200,0x0FB002F0,arm_cstat_msr_b,arm_core_status}, // MSR<c> <banked reg>, <Rn>
{0xF1000000,0xFFF10020,arm_cstat_cps,arm_core_status}, // CPS<effect> <iflags>{, #<mode>} CPS #<mode>
{0x0EB40A40,0x0FBF0E50,arm_fp_vcmp_r,arm_fp}, // VCMP{E}<c>.F64 <Dd>, <Dm> VCMP{E}<c>.F32 <Sd>, <Sm>
{0x0EB50A40,0x0FBF0E50,arm_fp_vcmp_z,arm_fp}, // VCMP{E}<c>.F64 <Dd>, #0.0 VCMP{E}<c>.F32 <Sd>, #0.0
{0x0EB20A40,0x0FBE0E50,arm_fp_vcvt_f3216,arm_fp}, // VCVT<y><c>.F32.F16 <Sd>, <Sm> VCVT<y><c>.F16.F32 <Sd>, <Sm>
{0x0EB70AC0,0x0FBF0ED0,arm_fp_vcvt_f6432,arm_fp}, // VCVT<c>.F64.F32 <Dd>, <Sm> VCVT<c>.F32.F64 <Sd>, <Dm>
{0x0EBA0A40,0x0FBA0E50,arm_fp_vcvt_f6432_bc,arm_fp}, // VCVT<c>.<Td>.F64 <Dd>, <Dd>, #<fbits> VCVT<c>.<Td>.F32 <Sd>, <Sd>, #<fbits> VCVT<c>.F64.<Td> <Dd>, <Dd>, #<fbits> VCVT<c>.F32.<Td> <Sd>, <Sd>, #<fbits>
{0x0EB80A40,0x0FB80E50,arm_fp_vcvt_sf6432,arm_fp}, // VCVT{R}<c>.S32.F64 <Sd>, <Dm> VCVT{R}<c>.S32.F32 <Sd>, <Sm> VCVT{R}<c>.U32.F64 <Sd>, <Dm> VCVT{R}<c>.U32.F32 <Sd>, <Sm> VCVT<c>.F64.<Tm> <Dd>, <Sm> VCVT<c>.F32.<Tm> <Sd>, <Sm>
{0x0E800A00,0x0FB00E50,arm_fp_vdiv,arm_fp}, // VDIV<c>.F64 <Dd>, <Dn>, <Dm> VDIV<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E900A00,0x0FB00E10,arm_fp_vfnm,arm_fp}, // VFNM<y><c>.F64 <Dd>, <Dn>, <Dm> VFNM<y><c>.F32 <Sd>, <Sn>, <Sm>
{0x01A00000,0x0FE00070,arm_mux_lsl_i_mov,arm_mux}, // LSL{S}<c> <Rd>, <Rm>, #<imm5>
{0x01A00000,0x0FE00FF0,arm_mux_lsl_i_mov,arm_mux}, // MOV{S}<c> <Rd>, <Rm>
{0x01A0F000,0x0FE0F070,arm_mux_lsl_i_mov_pc,arm_mux}, // LSL{S}<c> PC, <Rm>, #<imm5>
{0x01A0F000,0x0FE0FFF0,arm_mux_lsl_i_mov_pc,arm_mux}, // MOV{S}<c> PC, <Rm> (= LSL{S}<c> PC, <Rm>, #0)
{0x01000000,0x0FF002F0,arm_mux_mrs_r_pr,arm_mux}, // MRS<c> <Rd>, <spec reg>
{0x01000000,0x0FB002F0,arm_mux_mrs_r_pr,arm_mux}, // MRS<c> <Rd>, <spec reg>
{0x01200000,0x0FF302F0,arm_mux_msr_r_pr,arm_mux}, // MSR<c> <spec reg>, <Rn>
{0x01200000,0x0FB002F0,arm_mux_msr_r_pr,arm_mux}, // MSR<c> <spec reg>, <Rn>
{0x01A00060,0x0FE00FF0,arm_mux_ror_i_rrx,arm_mux}, // RRX{S}<c> <Rd>, <Rm>
{0x01A00060,0x0FE00070,arm_mux_ror_i_rrx,arm_mux}, // ROR{S}<c> <Rd>, <Rm>, #<imm>
{0x03200000,0x0FFF00FE,arm_mux_msr_i_pr_hints,arm_mux}, // NOP<c>
{0x03200000,0x0FFF00FE,arm_mux_msr_i_pr_hints,arm_mux}, // YIELD<c>
{0x03200002,0x0FFF00FE,arm_mux_wfe_wfi,arm_mux}, // WFE<c>
{0x03200002,0x0FFF00FE,arm_mux_wfe_wfi,arm_mux}, // WFI<c>
{0x03200000,0x0FF30000,arm_mux_msr_i_pr_hints,arm_mux}, // MSR<c> <spec reg>, #<const>
{0x03200000,0x0FB00000,arm_mux_msr_i_pr_hints,arm_mux}, // MSR<c> <spec reg>, #<const>
{0xF2800030,0xFEB800B0,arm_mux_vbic_vmvn,arm_mux}, // VBIC<c>.<dt> <Qd>, #<imm> VBIC<c>.<dt> <Dd>, #<imm>
{0xF2800030,0xFEB800B0,arm_mux_vbic_vmvn,arm_mux}, // VMVN<c>.<dt> <Qd>, #<imm> VMVN<c>.<dt> <Dd>, #<imm>
{0xF3B20200,0xFFB30FD0,arm_mux_vmovn_q,arm_mux}, // VMOVN<c>.<dt> <Dd>, <Qm>
{0xF3B20200,0xFFB30F10,arm_mux_vmovn_q,arm_mux}, // VQMOV{U}N<c>.<type><size> <Dd>, <Qm>
{0xF2800010,0xFEB800B0,arm_mux_vorr_i_vmov_i,arm_mux}, // VORR<c>.<dt> <Qd>, #<imm> VORR<c>.<dt> <Dd>, #<imm>
{0xF2800010,0xFEB80090,arm_mux_vorr_i_vmov_i,arm_mux}, // VMOV<c>.<dt> <Qd>, #<imm> VMOV<c>.<dt> <Dd>, #<imm>
{0xF2800010,0xFE800F10,arm_mux_vorr_i_vmov_i,arm_mux}, // VSHR<c>.<type><size> <Qd>, <Qm>, #<imm> VSHR<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2200110,0xFFB00F10,arm_mux_vorr_vmov_nm,arm_mux}, // VMOV<c> <Qd>, <Qm> VMOV<c> <Dd>, <Dm>
{0xF2200110,0xFFB00F10,arm_mux_vorr_vmov_nm,arm_mux}, // VORR<c> <Qd>, <Qn>, <Qm> VORR<c> <Dd>, <Dn>, <Dm>
{0xF2800850,0xFF800FD0,arm_mux_vrshrn_q_imm,arm_mux}, // VRSHRN<c>.I<size> <Dd>, <Qm>, #<imm>
{0xF2800850,0xFE800ED0,arm_mux_vrshrn_q_imm,arm_mux}, // VQRSHR{U}N<c>.<type><size> <Dd>, <Qm>, #<imm>
{0xF2800A10,0xFE870FD0,arm_mux_vshll_i_vmovl,arm_mux}, // VMOVL<c>.<dt> <Qd>, <Dm>
{0xF2800A10,0xFE800FD0,arm_mux_vshll_i_vmovl,arm_mux}, // VSHLL<c>.<type><size> <Qd>, <Dm>, #<imm> (0 < <imm> < <size>)
{0xF2800810,0xFF800FD0,arm_mux_vshrn_q_imm,arm_mux}, // VSHRN<c>.I<size> <Dd>, <Qm>, #<imm>
{0xF2800810,0xFE800ED0,arm_mux_vshrn_q_imm,arm_mux}, // VQSHR{U}N<c>.<type><size> <Dd>, <Qm>, #<imm>
{0xF4200000,0xFFB00000,arm_mux_vld_type,arm_mux}, // VLD1<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4200000,0xFFB00000,arm_mux_vld_type,arm_mux}, // VLD2<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4200000,0xFFB00000,arm_mux_vld_type,arm_mux}, // VLD3<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD3<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4200000,0xFFB00000,arm_mux_vld_type,arm_mux}, // VLD4<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4000000,0xFFB00000,arm_mux_vst_type,arm_mux}, // VST1<c>.<size> <list>, [<Rn>{:<align>}]{!} VST1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4000000,0xFFB00000,arm_mux_vst_type,arm_mux}, // VST2<c>.<size> <list>, [<Rn>{:<align>}]{!} VST2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4000000,0xFFB00000,arm_mux_vst_type,arm_mux}, // VST3<c>.<size> <list>, [<Rn>{:<align>}]{!} VST3<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4000000,0xFFB00000,arm_mux_vst_type,arm_mux}, // VST4<c>.<size> <list>, [<Rn>{:<align>}]{!} VST4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0x0EB00A00,0x0FB00E50,arm_vbits_vmov_6432_i,arm_v_bits}, // VMOV<c>.F64 <Dd>, #<imm> VMOV<c>.F32 <Sd>, #<imm>
{0x0EB00A40,0x0FBF0ED0,arm_vbits_vmov_6432_r,arm_v_bits}, // VMOV<c>.F64 <Dd>, <Dm> VMOV<c>.F32 <Sd>, <Sm>
{0xF2000110,0xFFB00F10,arm_vbits_vand,arm_v_bits}, // VAND<c> <Qd>, <Qn>, <Qm> VAND<c> <Dd>, <Dn>, <Dm>
{0xF2100110,0xFFB00F10,arm_vbits_vbic,arm_v_bits}, // VBIC<c> <Qd>, <Qn>, <Qm> VBIC<c> <Dd>, <Dn>, <Dm>
{0xF3300110,0xFFB00F10,arm_vbits_vbif,arm_v_bits}, // VBIF<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF3200110,0xFFB00F10,arm_vbits_vbit,arm_v_bits}, // VBIT<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF3100110,0xFFB00F10,arm_vbits_vbsl,arm_v_bits}, // VBSL<c> <Qd>, <Qn>, <Qm> V<op><c> <Dd>, <Dn>, <Dm>
{0xF3000110,0xFFB00F10,arm_vbits_veor,arm_v_bits}, // VEOR<c> <Qd>, <Qn>, <Qm> VEOR<c> <Dd>, <Dn>, <Dm>
{0xF3B00580,0xFFB30F90,arm_vbits_vmvn,arm_v_bits}, // VMVN<c> <Qd>, <Qm> VMVN<c> <Dd>, <Dm>
{0xF2300110,0xFFB00F10,arm_vbits_vorn,arm_v_bits}, // VORN<c> <Qd>, <Qn>, <Qm> VORN<c> <Dd>, <Dn>, <Dm>
{0xF3000E10,0xFFA00F10,arm_vcmp_vacge,arm_v_comp}, // VACGE<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF3200E10,0xFFA00F10,arm_vcmp_vacgt,arm_v_comp}, // VACGT<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000E00,0xFFA00F10,arm_vcmp_vceq,arm_v_comp}, // VCEQ<c>.F32 <Qd>, <Qn>, <Qm> VCEQ<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3000810,0xFF800F10,arm_vcmp_vceq_dt,arm_v_comp}, // VCEQ<c>.<dt> <Qd>, <Qn>, <Qm> VCEQ<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3B10100,0xFFB30B90,arm_vcmp_vceq_z,arm_v_comp}, // VCEQ<c>.<dt> <Qd>, <Qm>, #0 VCEQ<c>.<dt> <Dd>, <Dm>, #0
{0xF3000E00,0xFFA00F10,arm_vcmp_vcge,arm_v_comp}, // VCGE<c>.F32 <Qd>, <Qn>, <Qm> VCGE<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000310,0xFE800F10,arm_vcmp_vcge_dt,arm_v_comp}, // VCGE<c>.<dt> <Qd>, <Qn>, <Qm> VCGE<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3B10080,0xFFB30B90,arm_vcmp_vcge_z,arm_v_comp}, // VCGE<c>.<dt> <Qd>, <Qm>, #0 VCGE<c>.<dt> <Dd>, <Dm>, #0
{0xF2000300,0xFE800F10,arm_vcmp_vcgt_dt,arm_v_comp}, // VCGT<c>.<dt> <Qd>, <Qn>, <Qm> VCGT<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3200E00,0xFFA00F10,arm_vcmp_vcgt,arm_v_comp}, // VCGT<c>.F32 <Qd>, <Qn>, <Qm> VCGT<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B10000,0xFFB30B90,arm_vcmp_vcgt_z,arm_v_comp}, // VCGT<c>.<dt> <Qd>, <Qm>, #0 VCGT<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10180,0xFFB30B90,arm_vcmp_vcle_z,arm_v_comp}, // VCLE<c>.<dt> <Qd>, <Qm>, #0 VCLE<c>.<dt> <Dd>, <Dm>, #0
{0xF3B10200,0xFFB30B90,arm_vcmp_vclt_z,arm_v_comp}, // VCLT<c>.<dt> <Qd>, <Qm>, #0 VCLT<c>.<dt> <Dd>, <Dm>, #0
{0xF2000810,0xFF800F10,arm_vcmp_vtst,arm_v_comp}, // VTST<c>.<size> <Qd>, <Qn>, <Qm> VTST<c>.<size> <Dd>, <Dn>, <Dm>
{0x0EA00A00,0x0FB00E10,arm_vmac_vfm_f64,arm_v_mac}, // VFM<y><c>.F64 <Dd>, <Dn>, <Dm> VFM<y><c>.F32 <Sd>, <Sn>, <Sm>
{0x0E000A00,0x0FB00E50,arm_vmac_vmla_f64,arm_v_mac}, // VMLA<c>.F64 <Dd>, <Dn>, <Dm> VMLA<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E000A40,0x0FB00E50,arm_vmac_vmls_f64,arm_v_mac}, // VMLS<c>.F64 <Dd>, <Dn>, <Dm> VMLS<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E200A00,0x0FB00E50,arm_vmac_vmul_f64,arm_v_mac}, // VMUL<c>.F64 <Dd>, <Dn>, <Dm> VMUL<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E100A00,0x0FB00E10,arm_vmac_vnmlas_f64,arm_v_mac}, // VNMLA<c>.F64 <Dd>, <Dn>, <Dm> VNMLA<c>.F32 <Sd>, <Sn>, <Sm> VNMLS<c>.F64 <Dd>, <Dn>, <Dm> VNMLS<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E200A40,0x0FB00E50,arm_vmac_vnmul_f64,arm_v_mac}, // VNMUL<c>.F64 <Dd>, <Dn>, <Dm> VNMUL<c>.F32 <Sd>, <Sn>, <Sm>
{0xF2000C10,0xFF800F10,arm_vmac_vfm,arm_v_mac}, // VFM<y><c>.F32 <Qd>, <Qn>, <Qm> VFM<y><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000900,0xFF800F10,arm_vmac_vmla_dt,arm_v_mac}, // VMLA<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800040,0xFE800E50,arm_vmac_vmla_dtx,arm_v_mac}, // VMLA<c>.<dt> <Qd>, <Qn>, <Dm[x]> V<op><c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2000D10,0xFFA00F10,arm_vmac_vmla_f,arm_v_mac}, // VMLA<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800800,0xFE800F50,arm_vmac_vmlal_dt,arm_v_mac}, // VMLAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800240,0xFE800F50,arm_vmac_vmlal_dtx,arm_v_mac}, // VMLAL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF3000900,0xFF800F10,arm_vmac_vmls_dt,arm_v_mac}, // VMLS<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800440,0xFE800E50,arm_vmac_vmls_dtx,arm_v_mac}, // VMLS<c>.<dt> <Qd>, <Qn>, <Dm[x]> V<op><c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2200D10,0xFFA00F10,arm_vmac_vmls_f,arm_v_mac}, // VMLS<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800A00,0xFE800F50,arm_vmac_vmlsl_dt,arm_v_mac}, // VMLSL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800640,0xFE800F50,arm_vmac_vmlsl_dtx,arm_v_mac}, // VMLSL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2000910,0xFE800F10,arm_vmac_vmul_dt,arm_v_mac}, // VMUL<c>.<dt> <Qd>, <Qn>, <Qm> VMUL<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800840,0xFE800E50,arm_vmac_vmul_dtx,arm_v_mac}, // VMUL<c>.<dt> <Qd>, <Qn>, <Dm[x]> VMUL<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF3000D10,0xFFA00F10,arm_vmac_vmul_f,arm_v_mac}, // VMUL<c>.F32 <Qd>, <Qn>, <Qm> VMUL<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800C00,0xFE800D50,arm_vmac_vmull_dt,arm_v_mac}, // VMULL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800A40,0xFE800F50,arm_vmac_vmull_dtx,arm_v_mac}, // VMULL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800900,0xFF800F50,arm_vmac_vqdmlal_dt,arm_v_mac}, // VQDMLAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800340,0xFF800F50,arm_vmac_vqdmlal_dtx,arm_v_mac}, // VQDMLAL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2800B00,0xFF800F50,arm_vmac_vqdmlsl_dt,arm_v_mac}, // VQDMLSL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800740,0xFF800F50,arm_vmac_vqdmlsl_dtx,arm_v_mac}, // VQDMLSL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF2000B00,0xFF800F10,arm_vmac_vqdmulh_dt,arm_v_mac}, // VQDMULH<c>.<dt> <Qd>, <Qn>, <Qm> VQDMULH<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800C40,0xFE800F50,arm_vmac_vqdmulh_dtx,arm_v_mac}, // VQDMULH<c>.<dt> <Qd>, <Qn>, <Dm[x]> VQDMULH<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0xF2800D00,0xFF800F50,arm_vmac_vqdmull_dt,arm_v_mac}, // VQDMULL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2800B40,0xFF800F50,arm_vmac_vqdmull_dtx,arm_v_mac}, // VQDMULL<c>.<dt> <Qd>, <Dn>, <Dm[x]>
{0xF3000B00,0xFF800F10,arm_vmac_vqrdmulh_dt,arm_v_mac}, // VQRDMULH<c>.<dt> <Qd>, <Qn>, <Qm> VQRDMULH<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800D40,0xFE800F50,arm_vmac_vqrdmulh_dtx,arm_v_mac}, // VQRDMULH<c>.<dt> <Qd>, <Qn>, <Dm[x]> VQRDMULH<c>.<dt> <Dd>, <Dn>, <Dm[x]>
{0x0EB00AC0,0x0FBF0ED0,arm_vmisc_vabs_f64,arm_v_misc}, // VABS<c>.F64 <Dd>, <Dm> VABS<c>.F32 <Sd>, <Sm>
{0x0EB10A40,0x0FBF0ED0,arm_vmisc_vneg_f64,arm_v_misc}, // VNEG<c>.F64 <Dd>, <Dm> VNEG<c>.F32 <Sd>, <Sm>
{0x0EB10AC0,0x0FBF0ED0,arm_vmisc_vsqrt_f64,arm_v_misc}, // VSQRT<c>.F64 <Dd>, <Dm> VSQRT<c>.F32 <Sd>, <Sm>
{0xF2000710,0xFE800F10,arm_vmisc_vaba_dt,arm_v_misc}, // VABA<c>.<dt> <Qd>, <Qn>, <Qm> VABA<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2800500,0xFE800F50,arm_vmisc_vabal_dt,arm_v_misc}, // VABAL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF2000700,0xFE800F10,arm_vmisc_vabd_dt,arm_v_misc}, // VABD<c>.<dt> <Qd>, <Qn>, <Qm> VABD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3200D00,0xFFA00F10,arm_vmisc_vabd_f,arm_v_misc}, // VABD<c>.F32 <Qd>, <Qn>, <Qm> VABD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800700,0xFE800F50,arm_vmisc_vabdl_dt,arm_v_misc}, // VABDL<c>.<dt> <Qd>, <Dn>, <Dm>
{0xF3B10300,0xFFB30B90,arm_vmisc_vabs_dt,arm_v_misc}, // VABS<c>.<dt> <Qd>, <Qm> VABS<c>.<dt> <Dd>, <Dm>
{0xF3B00400,0xFFB30F90,arm_vmisc_vcls_dt,arm_v_misc}, // VCLS<c>.<dt> <Qd>, <Qm> VCLS<c>.<dt> <Dd>, <Dm>
{0xF3B00480,0xFFB30F90,arm_vmisc_vclz_dt,arm_v_misc}, // VCLZ<c>.<dt> <Qd>, <Qm> VCLZ<c>.<dt> <Dd>, <Dm>
{0xF3B00500,0xFFB30F90,arm_vmisc_vcnt,arm_v_misc}, // VCNT<c>.8 <Qd>, <Qm> VCNT<c>.8 <Dd>, <Dm>
{0xF3B20600,0xFFB30ED0,arm_vmisc_vcvt_f3216,arm_v_misc}, // VCVT<c>.F32.F16 <Qd>, <Dm> VCVT<c>.F16.F32 <Dd>, <Qm>
{0xF3B30600,0xFFB30E10,arm_vmisc_vcvt_tdtm,arm_v_misc}, // VCVT<c>.<Td>.<Tm> <Qd>, <Qm> VCVT<c>.<Td>.<Tm> <Dd>, <Dm>
{0xF2800E10,0xFE800E90,arm_vmisc_vcvt_tdtmb,arm_v_misc}, // VCVT<c>.<Td>.<Tm> <Qd>, <Qm>, #<fbits> VCVT<c>.<Td>.<Tm> <Dd>, <Dm>, #<fbits>
{0xF3B00C00,0xFFB00F90,arm_vmisc_vdup_x,arm_v_misc}, // VDUP<c>.<size> <Qd>, <Dm[x]> VDUP<c>.<size> <Dd>, <Dm[x]>
{0xF2B00000,0xFFB00010,arm_vmisc_vext_imm,arm_v_misc}, // VEXT<c>.8 <Qd>, <Qn>, <Qm>, #<imm> VEXT<c>.8 <Dd>, <Dn>, <Dm>, #<imm>
{0xF2000600,0xFE800F10,arm_vmisc_vmax_dt,arm_v_misc}, // VMAX<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000F00,0xFFA00F10,arm_vmisc_vmax_f,arm_v_misc}, // VMAX<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000610,0xFE800F10,arm_vmisc_vmin_dt,arm_v_misc}, // VMIN<c>.<dt> <Qd>, <Qn>, <Qm> V<op><c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2200F00,0xFFA00F10,arm_vmisc_vmin_f,arm_v_misc}, // VMIN<c>.F32 <Qd>, <Qn>, <Qm> V<op><c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B10380,0xFFB30B90,arm_vmisc_vneg_dt,arm_v_misc}, // VNEG<c>.<dt> <Qd>, <Qm> VNEG<c>.<dt> <Dd>, <Dm>
{0xF2000A00,0xFE800F10,arm_vmisc_vpmax_dt,arm_v_misc}, // VPMAX<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3000F00,0xFFA00F10,arm_vmisc_vpmax_f,arm_v_misc}, // VPMAX<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2000A10,0xFE800F10,arm_vmisc_vpmin_dt,arm_v_misc}, // VPMIN<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3200F00,0xFFA00F10,arm_vmisc_vpmin_f,arm_v_misc}, // VPMIN<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B00700,0xFFB30F90,arm_vmisc_vqabs_dt,arm_v_misc}, // VQABS<c>.<dt> <Qd>, <Qm> VQABS<c>.<dt> <Dd>, <Dm>
{0xF3B00780,0xFFB30F90,arm_vmisc_vqneg_dt,arm_v_misc}, // VQNEG<c>.<dt> <Qd>, <Qm> VQNEG<c>.<dt> <Dd>, <Dm>
{0xF3B30400,0xFFB30E90,arm_vmisc_vrecpe_dt,arm_v_misc}, // VRECPE<c>.<dt> <Qd>, <Qm> VRECPE<c>.<dt> <Dd>, <Dm>
{0xF2000F10,0xFFA00F10,arm_vmisc_vrecps_f,arm_v_misc}, // VRECPS<c>.F32 <Qd>, <Qn>, <Qm> VRECPS<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B00000,0xFFB30E10,arm_vmisc_vrev,arm_v_misc}, // VREV<n><c>.<size> <Qd>, <Qm> VREV<n><c>.<size> <Dd>, <Dm>
{0xF3B30480,0xFFB30E90,arm_vmisc_vrsqrte_dt,arm_v_misc}, // VRSQRTE<c>.<dt> <Qd>, <Qm> VRSQRTE<c>.<dt> <Dd>, <Dm>
{0xF2200F10,0xFFA00F10,arm_vmisc_vrsqrts_f,arm_v_misc}, // VRSQRTS<c>.F32 <Qd>, <Qn>, <Qm> VRSQRTS<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B20000,0xFFB30F90,arm_vmisc_vswp,arm_v_misc}, // VSWP<c> <Qd>, <Qm> VSWP<c> <Dd>, <Dm>
{0xF3B00800,0xFFB00C50,arm_vmisc_vtbl,arm_v_misc}, // VTBL<c>.8 <Dd>, <list>, <Dm>
{0xF3B00840,0xFFB00C50,arm_vmisc_vtbx,arm_v_misc}, // VTBX<c>.8 <Dd>, <list>, <Dm>
{0xF3B20080,0xFFB30F90,arm_vmisc_vtrn,arm_v_misc}, // VTRN<c>.<size> <Qd>, <Qm> VTRN<c>.<size> <Dd>, <Dm>
{0xF3B20100,0xFFB30F90,arm_vmisc_vuzp,arm_v_misc}, // VUZP<c>.<size> <Qd>, <Qm> VUZP<c>.<size> <Dd>, <Dm>
{0xF3B20180,0xFFB30F90,arm_vmisc_vzip,arm_v_misc}, // VZIP<c>.<size> <Qd>, <Qm> VZIP<c>.<size> <Dd>, <Dm>
{0x0E300A00,0x0FB00E50,arm_vpar_vadd_f64,arm_v_par}, // VADD<c>.F64 <Dd>, <Dn>, <Dm> VADD<c>.F32 <Sd>, <Sn>, <Sm>
{0x0E300A40,0x0FB00E50,arm_vpar_vsub_f64,arm_v_par}, // VSUB<c>.F64 <Dd>, <Dn>, <Dm> VSUB<c>.F32 <Sd>, <Sn>, <Sm>
{0xF2000800,0xFF800F10,arm_vpar_vadd_dt,arm_v_par}, // VADD<c>.<dt> <Qd>, <Qn>, <Qm> VADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000D00,0xFFA00F10,arm_vpar_vadd_f,arm_v_par}, // VADD<c>.F32 <Qd>, <Qn>, <Qm> VADD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800000,0xFE800E50,arm_vpar_vadd_lw_dt,arm_v_par}, // VADDL<c>.<dt> <Qd>, <Dn>, <Dm> VADDW<c>.<dt> <Qd>, <Qn>, <Dm>
{0xF2800400,0xFF800F50,arm_vpar_vaddhn_dt,arm_v_par}, // VADDHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF2000000,0xFE800F10,arm_vpar_vhadd,arm_v_par}, // VHADD<c> <Qd>, <Qn>, <Qm> VH<op><c> <Dd>, <Dn>, <Dm>
{0xF2000200,0xFE800F10,arm_vpar_vhsub,arm_v_par}, // VHSUB<c> <Qd>, <Qn>, <Qm> VH<op><c> <Dd>, <Dn>, <Dm>
{0xF3B00600,0xFFB30F10,arm_vpar_vpadal_dt,arm_v_par}, // VPADAL<c>.<dt> <Qd>, <Qm> VPADAL<c>.<dt> <Dd>, <Dm>
{0xF2000B10,0xFF800F10,arm_vpar_vpadd_dt,arm_v_par}, // VPADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF3000D00,0xFFA00F10,arm_vpar_vpadd_f,arm_v_par}, // VPADD<c>.F32 <Dd>, <Dn>, <Dm>
{0xF3B00200,0xFFB30F10,arm_vpar_vpaddl_dt,arm_v_par}, // VPADDL<c>.<dt> <Qd>, <Qm> VPADDL<c>.<dt> <Dd>, <Dm>
{0xF2000010,0xFE800F10,arm_vpar_vqadd_dt,arm_v_par}, // VQADD<c>.<dt> <Qd>, <Qn>, <Qm> VQADD<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2000210,0xFE800F10,arm_vpar_vqsub_dt,arm_v_par}, // VQSUB<c>.<type><size> <Qd>, <Qn>, <Qm> VQSUB<c>.<type><size> <Dd>, <Dn>, <Dm>
{0xF3800400,0xFF800F50,arm_vpar_vraddhn_dt,arm_v_par}, // VRADDHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF2000100,0xFE800F10,arm_vpar_vrhadd,arm_v_par}, // VRHADD<c> <Qd>, <Qn>, <Qm> VRHADD<c> <Dd>, <Dn>, <Dm>
{0xF3800600,0xFF800F50,arm_vpar_vrsubhn_dt,arm_v_par}, // VRSUBHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF3000800,0xFF800F10,arm_vpar_vsub_dt,arm_v_par}, // VSUB<c>.<dt> <Qd>, <Qn>, <Qm> VSUB<c>.<dt> <Dd>, <Dn>, <Dm>
{0xF2200D00,0xFFA00F10,arm_vpar_vsub_f,arm_v_par}, // VSUB<c>.F32 <Qd>, <Qn>, <Qm> VSUB<c>.F32 <Dd>, <Dn>, <Dm>
{0xF2800600,0xFF800F50,arm_vpar_vsubhn_dt,arm_v_par}, // VSUBHN<c>.<dt> <Dd>, <Qn>, <Qm>
{0xF2800200,0xFE800E50,arm_vpar_vsubl_dt,arm_v_par}, // VSUBL<c>.<dt> <Qd>, <Dn>, <Dm> VSUBW<c>.<dt> <Qd>, <Qn>, <Dm>
{0xF2000510,0xFE800F10,arm_vsh_vqrshl_dt,arm_v_shift}, // VQRSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VQRSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2000410,0xFE800F10,arm_vsh_vqshl_dt,arm_v_shift}, // VQSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VQSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2800610,0xFE800E10,arm_vsh_vqshl_dt_imm,arm_v_shift}, // VQSHL{U}<c>.<type><size> <Qd>, <Qm>, #<imm> VQSHL{U}<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2000500,0xFE800F10,arm_vsh_vrshl_dt,arm_v_shift}, // VRSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VRSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2800210,0xFE800F10,arm_vsh_vrshr_dt_imm,arm_v_shift}, // VRSHR<c>.<type><size> <Qd>, <Qm>, #<imm> VRSHR<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2800310,0xFE800F10,arm_vsh_vrsra_dt_imm,arm_v_shift}, // VRSRA<c>.<type><size> <Qd>, <Qm>, #<imm> VRSRA<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF2000400,0xFE800F10,arm_vsh_vshl_dt,arm_v_shift}, // VSHL<c>.<type><size> <Qd>, <Qm>, <Qn> VSHL<c>.<type><size> <Dd>, <Dm>, <Dn>
{0xF2800510,0xFF800F10,arm_vsh_vshl_imm,arm_v_shift}, // VSHL<c>.I<size> <Qd>, <Qm>, #<imm> VSHL<c>.I<size> <Dd>, <Dm>, #<imm>
{0xF3B20300,0xFFB30FD0,arm_vsh_vshll_imm,arm_v_shift}, // VSHLL<c>.<type><size> <Qd>, <Dm>, #<imm> (<imm> == <size>)
{0xF3800510,0xFF800F10,arm_vsh_vsli_imm,arm_v_shift}, // VSLI<c>.<size> <Qd>, <Qm>, #<imm> VSLI<c>.<size> <Dd>, <Dm>, #<imm>
{0xF2800110,0xFE800F10,arm_vsh_vsra_dt_imm,arm_v_shift}, // VSRA<c>.<type><size> <Qd>, <Qm>, #<imm> VSRA<c>.<type><size> <Dd>, <Dm>, #<imm>
{0xF3800410,0xFF800F10,arm_vsh_vsri_imm,arm_v_shift}, // VSRI<c>.<size> <Qd>, <Qm>, #<imm> VSRI<c>.<size> <Dd>, <Dm>, #<imm>
{0xF4A00C00,0xFFB00F00,arm_vldste_vld1_all,arm_vfp_ldst_elem}, // VLD1<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00000,0xFFB00300,arm_vldste_vld1_one,arm_vfp_ldst_elem}, // VLD1<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00D00,0xFFB00F00,arm_vldste_vld2_all,arm_vfp_ldst_elem}, // VLD2<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00100,0xFFB00300,arm_vldste_vld2_one,arm_vfp_ldst_elem}, // VLD2<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4A00E00,0xFFB00F00,arm_vldste_vld3_all,arm_vfp_ldst_elem}, // VLD3<c>.<size> <list>, [<Rn>]{!} VLD3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4A00200,0xFFB00300,arm_vldste_vld3_one,arm_vfp_ldst_elem}, // VLD3<c>.<size> <list>, [<Rn>]{!} VLD3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4A00F00,0xFFB00F00,arm_vldste_vld4_all,arm_vfp_ldst_elem}, // VLD4<c>.<size> <list>, [<Rn>{ :<align>}]{!} VLD4<c>.<size> <list>, [<Rn>{ :<align>}], <Rm>
{0xF4A00300,0xFFB00300,arm_vldste_vld4_one,arm_vfp_ldst_elem}, // VLD4<c>.<size> <list>, [<Rn>{:<align>}]{!} VLD4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4800000,0xFFB00300,arm_vldste_vst1_one,arm_vfp_ldst_elem}, // VST1<c>.<size> <list>, [<Rn>{:<align>}]{!} VST1<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4800100,0xFFB00300,arm_vldste_vst2_one,arm_vfp_ldst_elem}, // VST2<c>.<size> <list>, [<Rn>{:<align>}]{!} VST2<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0xF4800200,0xFFB00300,arm_vldste_vst3_one,arm_vfp_ldst_elem}, // VST3<c>.<size> <list>, [<Rn>]{!} VST3<c>.<size> <list>, [<Rn>], <Rm>
{0xF4800300,0xFFB00300,arm_vldste_vst4_one,arm_vfp_ldst_elem}, // VST4<c>.<size> <list>, [<Rn>{:<align>}]{!} VST4<c>.<size> <list>, [<Rn>{:<align>}], <Rm>
{0x0C100A00,0x0E100F00,arm_vldstx_vldm32,arm_vfp_ldst_ext}, // VLDM{mode}<c> <Rn>{!}, <list>
{0x0C100B00,0x0E100F00,arm_vldstx_vldm64,arm_vfp_ldst_ext}, // VLDM{mode}<c> <Rn>{!}, <list>
{0x0D100B00,0x0F300F00,arm_vldstx_vldr_d_imm,arm_vfp_ldst_ext}, // VLDR<c> <Dd>, [<Rn>{, #+/-<imm>}] VLDR<c> <Dd>, <label> VLDR<c> <Dd>, [PC, #-0] Special case
{0x0D100A00,0x0F300F00,arm_vldstx_vldr_s_imm,arm_vfp_ldst_ext}, // VLDR<c> <Sd>, [<Rn>{, #+/-<imm>}] VLDR<c> <Sd>, <label> VLDR<c> <Sd>, [PC, #-0] Special case
{0x0CBD0A00,0x0FBF0F00,arm_vldstx_vpop32,arm_vfp_ldst_ext}, // VPOP <list>
{0x0CBD0B00,0x0FBF0F00,arm_vldstx_vpop64,arm_vfp_ldst_ext}, // VPOP <list>
{0x0D2D0A00,0x0FBF0F00,arm_vldstx_vpush32,arm_vfp_ldst_ext}, // VPUSH<c> <list>
{0x0D2D0B00,0x0FBF0F00,arm_vldstx_vpush64,arm_vfp_ldst_ext}, // VPUSH<c> <list>
{0x0C000A00,0x0E100F00,arm_vldstx_vstm32,arm_vfp_ldst_ext}, // VSTM{mode}<c> <Rn>{!}, <list>
{0x0C000B00,0x0E100F00,arm_vldstx_vstm64,arm_vfp_ldst_ext}, // VSTM{mode}<c> <Rn>{!}, <list>
{0x0D000B00,0x0F300F00,arm_vldstx_vstr_d_imm,arm_vfp_ldst_ext}, // VSTR<c> <Dd>, [<Rn>{, #+/-<imm>}]
{0x0D000A00,0x0F300F00,arm_vldstx_vstr_s_imm,arm_vfp_ldst_ext}, // VSTR<c> <Sd>, [<Rn>{, #+/-<imm>}]
{0x0C400B10,0x0FE00FD0,arm_vfpxfer_vmov_d,arm_vfp_xfer_reg}, // VMOV<c> <Dm>, <Rt>, <Rt2> VMOV<c> <Rt>, <Rt2>, <Dm>
{0x0C400A10,0x0FE00FD0,arm_vfpxfer_vmov_ss,arm_vfp_xfer_reg}, // VMOV<c> <Sm>, <Sm1>, <Rt>, <Rt2> VMOV<c> <Rt>, <Rt2>, <Sm>, <Sm1>
{0x0E800B10,0x0F900F50,arm_vfpxfer_vdup,arm_vfp_xfer_reg}, // VDUP<c>.<size> <Qd>, <Rt> VDUP<c>.<size> <Dd>, <Rt>
{0x0E100B10,0x0F100F10,arm_vfpxfer_vmov_dt_dx,arm_vfp_xfer_reg}, // VMOV<c>.<dt> <Rt>, <Dn[x]>
{0x0E000B10,0x0F900F10,arm_vfpxfer_vmov_dx,arm_vfp_xfer_reg}, // VMOV<c>.<size> <Dd[x]>, <Rt>
{0x0E000A10,0x0FE00F10,arm_vfpxfer_vmov_s,arm_vfp_xfer_reg}, // VMOV<c> <Sn>, <Rt> VMOV<c> <Rt>, <Sn>
{0x0EF10A10,0x0FFF0F10,arm_vfpxfer_vmrs_fpscr,arm_vfp_xfer_reg}, // VMRS<c> <Rt>, FPSCR
{0x0EF00A10,0x0FF00F10,arm_vfpxfer_vmrs_r,arm_vfp_xfer_reg}, // VMRS<c> <Rt>, <spec reg>
{0x0EE10A10,0x0FFF0F10,arm_vfpxfer_vmsr_fpscr,arm_vfp_xfer_reg}, // VMSR<c> FPSCR, <Rt>
{0x0EE00A10,0x0FF00F10,arm_vfpxfer_vmsr_r,arm_vfp_xfer_reg} // VMSR<c> <spec reg>, <Rt>
