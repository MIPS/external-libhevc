/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Imagination Technologies
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ihevc_typedefs.h"
#include "iv.h"
#include "ivd.h"
#include "ihevc_defs.h"
#include "ihevc_debug.h"
#include "ihevc_structs.h"
#include "ihevc_macros.h"
#include "ihevc_platform_macros.h"
#include "ihevc_cabac_tables.h"
#include "ihevc_disp_mgr.h"
#include "ihevc_buf_mgr.h"
#include "ihevc_dpb_mgr.h"
#include "ihevc_error.h"
#include "ihevcd_defs.h"
#include "ihevcd_function_selector.h"
#include "ihevcd_structs.h"

void ihevcd_init_function_ptr_msa(codec_t *ps_codec)
{
    ps_codec->s_func_selector.ihevc_inter_pred_luma_copy_fptr = &ihevc_inter_pred_luma_copy_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_horz_fptr = &ihevc_inter_pred_luma_horz_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_vert_fptr = &ihevc_inter_pred_luma_vert_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_copy_w16out_fptr = &ihevc_inter_pred_luma_copy_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_horz_w16out_fptr = &ihevc_inter_pred_luma_horz_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_vert_w16out_fptr = &ihevc_inter_pred_luma_vert_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_vert_w16inp_fptr = &ihevc_inter_pred_luma_vert_w16inp_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_luma_vert_w16inp_w16out_fptr = &ihevc_inter_pred_luma_vert_w16inp_w16out_msa;

    ps_codec->s_func_selector.ihevc_inter_pred_chroma_copy_fptr = &ihevc_inter_pred_chroma_copy_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_horz_fptr = &ihevc_inter_pred_chroma_horz_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_vert_fptr = &ihevc_inter_pred_chroma_vert_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_copy_w16out_fptr = &ihevc_inter_pred_chroma_copy_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_horz_w16out_fptr = &ihevc_inter_pred_chroma_horz_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_vert_w16out_fptr = &ihevc_inter_pred_chroma_vert_w16out_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_vert_w16inp_fptr = &ihevc_inter_pred_chroma_vert_w16inp_msa;
    ps_codec->s_func_selector.ihevc_inter_pred_chroma_vert_w16inp_w16out_fptr = &ihevc_inter_pred_chroma_vert_w16inp_w16out_msa;

    ps_codec->s_func_selector.ihevc_intra_pred_luma_planar_fptr =  &ihevc_intra_pred_luma_planar_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_dc_fptr = &ihevc_intra_pred_luma_dc_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_horz_fptr = &ihevc_intra_pred_luma_horz_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_ver_fptr = &ihevc_intra_pred_luma_ver_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode2_fptr = &ihevc_intra_pred_luma_mode2_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode_18_34_fptr = &ihevc_intra_pred_luma_mode_18_34_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode_27_to_33_fptr = &ihevc_intra_pred_luma_mode_27_to_33_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode_3_to_9_fptr = &ihevc_intra_pred_luma_mode_3_to_9_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode_11_to_17_fptr = &ihevc_intra_pred_luma_mode_11_to_17_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_luma_mode_19_to_25_fptr = &ihevc_intra_pred_luma_mode_19_to_25_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_ref_filtering_fptr = &ihevc_intra_pred_ref_filtering_msa;

    ps_codec->s_func_selector.ihevc_intra_pred_chroma_planar_fptr = &ihevc_intra_pred_chroma_planar_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_dc_fptr = &ihevc_intra_pred_chroma_dc_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_horz_fptr = &ihevc_intra_pred_chroma_horz_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_ver_fptr = &ihevc_intra_pred_chroma_ver_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode2_fptr = &ihevc_intra_pred_chroma_mode2_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode_18_34_fptr = &ihevc_intra_pred_chroma_mode_18_34_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode_27_to_33_fptr = &ihevc_intra_pred_chroma_mode_27_to_33_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode_3_to_9_fptr = &ihevc_intra_pred_chroma_mode_3_to_9_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode_11_to_17_fptr = &ihevc_intra_pred_chroma_mode_11_to_17_msa;
    ps_codec->s_func_selector.ihevc_intra_pred_chroma_mode_19_to_25_fptr = &ihevc_intra_pred_chroma_mode_19_to_25_msa;

    ps_codec->s_func_selector.ihevc_itrans_recon_4x4_ttype1_fptr = &ihevc_itrans_recon_4x4_ttype1_msa;
    ps_codec->s_func_selector.ihevc_itrans_recon_4x4_fptr = &ihevc_itrans_recon_4x4_msa;
    ps_codec->s_func_selector.ihevc_itrans_recon_8x8_fptr = &ihevc_itrans_recon_8x8_msa;
    ps_codec->s_func_selector.ihevc_itrans_recon_16x16_fptr = &ihevc_itrans_recon_16x16_msa;
    ps_codec->s_func_selector.ihevc_itrans_recon_32x32_fptr = &ihevc_itrans_recon_32x32_msa;
    ps_codec->s_func_selector.ihevc_chroma_itrans_recon_4x4_fptr = &ihevc_chroma_itrans_recon_4x4_msa;
    ps_codec->s_func_selector.ihevc_chroma_itrans_recon_8x8_fptr = &ihevc_chroma_itrans_recon_8x8_msa;
    ps_codec->s_func_selector.ihevc_chroma_itrans_recon_16x16_fptr = &ihevc_chroma_itrans_recon_16x16_msa;
    ps_codec->s_func_selector.ihevcd_itrans_recon_dc_luma_fptr = &ihevcd_itrans_recon_dc_luma_msa;
    ps_codec->s_func_selector.ihevcd_itrans_recon_dc_chroma_fptr = &ihevcd_itrans_recon_dc_chroma_msa;

    ps_codec->s_func_selector.ihevc_recon_4x4_ttype1_fptr = &ihevc_recon_4x4_msa;
    ps_codec->s_func_selector.ihevc_recon_4x4_fptr = &ihevc_recon_4x4_msa;
    ps_codec->s_func_selector.ihevc_recon_8x8_fptr = &ihevc_recon_8x8_msa;
    ps_codec->s_func_selector.ihevc_recon_16x16_fptr = &ihevc_recon_16x16_msa;
    ps_codec->s_func_selector.ihevc_recon_32x32_fptr = &ihevc_recon_32x32_msa;
    ps_codec->s_func_selector.ihevc_chroma_recon_4x4_fptr = &ihevc_chroma_recon_4x4_msa;
    ps_codec->s_func_selector.ihevc_chroma_recon_8x8_fptr = &ihevc_chroma_recon_8x8_msa;
    ps_codec->s_func_selector.ihevc_chroma_recon_16x16_fptr = &ihevc_chroma_recon_16x16_msa;

    ps_codec->s_func_selector.ihevc_weighted_pred_bi_fptr = &ihevc_weighted_pred_bi_msa;
    ps_codec->s_func_selector.ihevc_weighted_pred_bi_default_fptr = &ihevc_weighted_pred_bi_default_msa;
    ps_codec->s_func_selector.ihevc_weighted_pred_uni_fptr = &ihevc_weighted_pred_uni_msa;
    ps_codec->s_func_selector.ihevc_weighted_pred_chroma_bi_fptr = &ihevc_weighted_pred_chroma_bi_msa;
    ps_codec->s_func_selector.ihevc_weighted_pred_chroma_bi_default_fptr = &ihevc_weighted_pred_chroma_bi_default_msa;
    ps_codec->s_func_selector.ihevc_weighted_pred_chroma_uni_fptr = &ihevc_weighted_pred_chroma_uni_msa;

    ps_codec->s_func_selector.ihevc_pad_left_luma_fptr = &ihevc_pad_left_luma_msa;
    ps_codec->s_func_selector.ihevc_pad_left_chroma_fptr = &ihevc_pad_left_chroma_msa;
    ps_codec->s_func_selector.ihevc_pad_right_luma_fptr = &ihevc_pad_right_luma_msa;
    ps_codec->s_func_selector.ihevc_pad_right_chroma_fptr = &ihevc_pad_right_chroma_msa;

    ps_codec->s_func_selector.ihevc_memset_16bit_mul_8_fptr = &ihevc_memset_16bit_mul_8_msa;
    ps_codec->s_func_selector.ihevc_memset_16bit_fptr = &ihevc_memset_16bit_msa;

    ps_codec->s_func_selector.ihevcd_fmt_conv_420sp_to_rgb565_fptr = &ihevcd_fmt_conv_420sp_to_rgb565_msa;
    ps_codec->s_func_selector.ihevcd_fmt_conv_420sp_to_420sp_fptr = &ihevcd_fmt_conv_420sp_to_420sp_msa;
    ps_codec->s_func_selector.ihevcd_fmt_conv_420sp_to_420p_fptr = &ihevcd_fmt_conv_420sp_to_420p_msa;

    ps_codec->s_func_selector.ihevc_deblk_chroma_horz_fptr = &ihevc_deblk_chroma_horz_msa;
    ps_codec->s_func_selector.ihevc_deblk_chroma_vert_fptr = &ihevc_deblk_chroma_vert_msa;
    ps_codec->s_func_selector.ihevc_deblk_luma_vert_fptr = &ihevc_deblk_luma_vert_msa;
    ps_codec->s_func_selector.ihevc_deblk_luma_horz_fptr = &ihevc_deblk_luma_horz_msa;

    ps_codec->s_func_selector.ihevc_sao_band_offset_luma_fptr = &ihevc_sao_band_offset_luma_msa;
    ps_codec->s_func_selector.ihevc_sao_band_offset_chroma_fptr = &ihevc_sao_band_offset_chroma_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class0_fptr = &ihevc_sao_edge_offset_class0_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class0_chroma_fptr = &ihevc_sao_edge_offset_class0_chroma_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class1_fptr = &ihevc_sao_edge_offset_class1_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class1_chroma_fptr = &ihevc_sao_edge_offset_class1_chroma_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class2_fptr = &ihevc_sao_edge_offset_class2_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class2_chroma_fptr = &ihevc_sao_edge_offset_class2_chroma_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class3_fptr = &ihevc_sao_edge_offset_class3_msa;
    ps_codec->s_func_selector.ihevc_sao_edge_offset_class3_chroma_fptr = &ihevc_sao_edge_offset_class3_chroma_msa;

    return;
}
