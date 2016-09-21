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

#include "ihevc_typedefs.h"
#include "ihevc_intra_pred.h"
#include "ihevc_macros.h"
#include "ihevc_platform_macros.h"
#include "ihevc_common_tables.h"
#include "ihevc_macros_msa.h"

#define TRANSPOSE16x4_UB(in0, in1, in2, in3, in4, in5, in6, in7,  \
                         out0, out1, out2, out3)                  \
{                                                                 \
    v8i16 in10_r, in32_r;                                         \
    v8i16 in10_l, in32_l;                                         \
                                                                  \
    ILVEV_B2_SH(in0, in2, in4, in6, in10_r, in32_r);              \
    ILVEV_B2_SH(in1, in3, in5, in7, in10_l, in32_l);              \
                                                                  \
    ILVR_H2_SH(in32_r, in10_r, in32_l, in10_l, out0, out2);       \
    ILVL_H2_SH(in32_r, in10_r, in32_l, in10_l, out1, out3);       \
}

#define HEVC_INTRA_2TAP_FILT_4X4(src, pos, ang)                        \
( {                                                                    \
    WORD32 idx_m, fract_m;                                            \
    v16i8 mask_m = { 0, 1, 1, 2, 2, 3, 3, 4,                           \
                     16, 17, 17, 18, 18, 19, 19, 20 };                 \
    v16i8 src0_m, src1_m, src2_m, src3_m; ;                            \
    v16i8 filt0_m, filt1_m, filt2_m, filt3_m;                          \
    v8u16 dst0_m, dst1_m;                                              \
                                                                       \
    idx_m = pos >> 5;                                                  \
    fract_m = pos & (31);                                              \
    pos += ang;                                                        \
    fract_m = ((32 - fract_m) & 0x000000FF) | (fract_m << 8);          \
    filt0_m = (v16i8) __msa_fill_h(fract_m);                           \
    src0_m = LD_SB(src + idx_m);                                       \
                                                                       \
    idx_m = pos >> 5;                                                  \
    fract_m = pos & (31);                                              \
    pos += ang;                                                        \
    fract_m = ((32 - fract_m) & 0x000000FF) | (fract_m << 8);          \
    filt1_m = (v16i8) __msa_fill_h(fract_m);                           \
    src1_m = LD_SB(src + idx_m);                                       \
                                                                       \
    idx_m = pos >> 5;                                                  \
    fract_m = pos & (31);                                              \
    pos += ang;                                                        \
    fract_m = ((32 - fract_m) & 0x000000FF) | (fract_m << 8);          \
    filt2_m = (v16i8) __msa_fill_h(fract_m);                           \
    src2_m = LD_SB(src + idx_m);                                       \
                                                                       \
    idx_m = pos >> 5;                                                  \
    fract_m = pos & (31);                                              \
    pos += ang;                                                        \
    fract_m = ((32 - fract_m) & 0x000000FF) | (fract_m << 8);          \
    filt3_m = (v16i8) __msa_fill_h(fract_m);                           \
    src3_m = LD_SB(src + idx_m);                                       \
                                                                       \
    ILVR_D2_SB(filt1_m, filt0_m, filt3_m, filt2_m, filt0_m, filt1_m);  \
    VSHF_B2_SB(src0_m, src1_m, src2_m, src3_m, mask_m, mask_m,         \
               src0_m, src1_m);                                        \
    DOTP_UB2_UH(src0_m, src1_m, filt0_m, filt1_m, dst0_m, dst1_m);     \
    SRARI_H2_UH(dst0_m, dst1_m, 5);                                    \
    dst0_m = (v8u16) __msa_pckev_b((v16i8) dst1_m, (v16i8) dst0_m);    \
                                                                       \
    dst0_m;                                                            \
} )

#define HEVC_INTRA_2TAP_FILT_8X4(OFFSET)                              \
{                                                                     \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt0 = (v16i8) __msa_fill_h(fract);                              \
    src0 = LD_SB(src + OFFSET * idx);                                 \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt1 = (v16i8) __msa_fill_h(fract);                              \
    src1 = LD_SB(src + OFFSET * idx);                                 \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt2 = (v16i8) __msa_fill_h(fract);                              \
    src2 = LD_SB(src + OFFSET * idx);                                 \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt3 = (v16i8) __msa_fill_h(fract);                              \
    src3 = LD_SB(src + OFFSET * idx);                                 \
                                                                      \
    VSHF_B2_SB(src0, src0, src1, src1, mask, mask, src0, src1);       \
    VSHF_B2_SB(src2, src2, src3, src3, mask, mask, src2, src3);       \
                                                                      \
    DOTP_UB4_UH(src0, src1, src2, src3, filt0, filt1, filt2, filt3,   \
                dst0, dst1, dst2, dst3);                              \
    SRARI_H4_UH(dst0, dst1, dst2, dst3, 5);                           \
}

#define HEVC_INTRA_2TAP_FILT_16x4(OFFSET)                             \
{                                                                     \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt0 = (v16i8) __msa_fill_h(fract);                              \
                                                                      \
    src0 = LD_SB(src + OFFSET * idx);                                 \
    src1 = LD_SB(src + OFFSET * idx + 8);                             \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt1 = (v16i8) __msa_fill_h(fract);                              \
                                                                      \
    src2 = LD_SB(src + OFFSET * idx);                                 \
    src3 = LD_SB(src + OFFSET * idx + 8);                             \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt2 = (v16i8) __msa_fill_h(fract);                              \
                                                                      \
    src4 = LD_SB(src + OFFSET * idx);                                 \
    src5 = LD_SB(src + OFFSET * idx + 8);                             \
                                                                      \
    idx = pos >> 5;                                                   \
    fract = pos & (31);                                               \
    pos += ang;                                                       \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);               \
    filt3 = (v16i8) __msa_fill_h(fract);                              \
                                                                      \
    src6 = LD_SB(src + OFFSET * idx);                                 \
    src7 = LD_SB(src + OFFSET * idx + 8);                             \
                                                                      \
    VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask, mask, mask,  \
               src0, src1, src2);                                     \
    VSHF_B3_SB(src3, src3, src4, src4, src5, src5, mask, mask, mask,  \
               src3, src4, src5);                                     \
    VSHF_B2_SB(src6, src6, src7, src7, mask, mask, src6, src7);       \
    DOTP_UB4_UH(src0, src1, src2, src3, filt0, filt0, filt1, filt1,   \
                dst0, dst1, dst2, dst3);                              \
    DOTP_UB4_UH(src4, src5, src6, src7, filt2, filt2, filt3, filt3,   \
                dst4, dst5, dst6, dst7);                              \
    SRARI_H4_UH(dst0, dst1, dst2, dst3, 5);                           \
    SRARI_H4_UH(dst4, dst5, dst6, dst7, 5);                           \
}

#define HEVC_INTRA_2TAP_FILT_32x8(OFFSET)                                   \
{                                                                           \
    idx0 = pos >> 5;                                                        \
    fract = pos & (31);                                                     \
    pos += ang;                                                             \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);                     \
    filt0 = (v16i8) __msa_fill_h(fract);                                    \
                                                                            \
    src0 = LD_SB(src + OFFSET * idx0);                                      \
    src2 = LD_SB(src + OFFSET * idx0 + 16);                                 \
    src3 = LD_SB(src + OFFSET * idx0 + 24);                                 \
    src1 = __msa_sldi_b(src2, src0, 8);                                     \
                                                                            \
    idx1 = pos >> 5;                                                        \
    fract = pos & (31);                                                     \
    pos += ang;                                                             \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);                     \
    filt1 = (v16i8) __msa_fill_h(fract);                                    \
                                                                            \
    src4 = LD_SB(src + OFFSET * idx1);                                      \
    src6 = LD_SB(src + OFFSET * idx1 + 16);                                 \
    src7 = LD_SB(src + OFFSET * idx1 + 24);                                 \
    src5 = __msa_sldi_b(src6, src4, 8);                                     \
                                                                            \
    idx2 = pos >> 5;                                                        \
    fract = pos & (31);                                                     \
    pos += ang;                                                             \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);                     \
    filt2 = (v16i8) __msa_fill_h(fract);                                    \
                                                                            \
    src8  = LD_SB(src + OFFSET * idx2);                                     \
    src10 = LD_SB(src + OFFSET * idx2 + 16);                                \
    src11 = LD_SB(src + OFFSET * idx2 + 24);                                \
    src9 = __msa_sldi_b(src10, src8, 8);                                    \
                                                                            \
    idx3 = pos >> 5;                                                        \
    fract = pos & (31);                                                     \
    pos += ang;                                                             \
    fract = ((32 - fract) & 0x000000FF) | (fract << 8);                     \
    filt3 = (v16i8) __msa_fill_h(fract);                                    \
                                                                            \
    src12 = LD_SB(src + OFFSET * idx3);                                     \
    src14 = LD_SB(src + OFFSET * idx3 + 16);                                \
    src15 = LD_SB(src + OFFSET * idx3 + 24);                                \
    src13 = __msa_sldi_b(src14, src12, 8);                                  \
                                                                            \
    VSHF_B3_SB(src0, src0, src1, src1, src2, src2, mask, mask, mask,        \
               src0, src1, src2);                                           \
    VSHF_B3_SB(src3, src3, src4, src4, src5, src5, mask, mask, mask,        \
               src3, src4, src5);                                           \
    VSHF_B3_SB(src6, src6, src7, src7, src8, src8, mask, mask, mask,        \
               src6, src7, src8);                                           \
    VSHF_B3_SB(src9, src9, src10, src10, src11, src11, mask, mask, mask,    \
               src9, src10, src11);                                         \
    VSHF_B3_SB(src12, src12, src13, src13, src14, src14, mask, mask, mask,  \
               src12, src13, src14);                                        \
    src15 = __msa_vshf_b(mask, src15, src15);                               \
                                                                            \
    DOTP_UB4_UH(src0, src1, src2, src3, filt0, filt0, filt0, filt0,         \
                dst0, dst1, dst2, dst3);                                    \
    DOTP_UB4_UH(src4, src5, src6, src7, filt1, filt1, filt1, filt1,         \
                dst4, dst5, dst6, dst7);                                    \
    DOTP_UB4_UH(src8, src9, src10, src11, filt2, filt2, filt2, filt2,       \
                dst8, dst9, dst10, dst11);                                  \
    DOTP_UB4_UH(src12, src13, src14, src15, filt3, filt3, filt3, filt3,     \
                dst12, dst13, dst14, dst15);                                \
    SRARI_H4_UH(dst0, dst1, dst2, dst3, 5);                                 \
    SRARI_H4_UH(dst4, dst5, dst6, dst7, 5);                                 \
    SRARI_H4_UH(dst8, dst9, dst10, dst11, 5);                               \
    SRARI_H4_UH(dst12, dst13, dst14, dst15, 5);                             \
}

#define HEVC_INTRA_PLANAR_16PIX(INDEX, RND, TYPE, VTYPE)            \
{                                                                   \
    tmp_r = tmp_r - src_tr + bottom_left;                           \
    tmp_l = tmp_l - src_tl + bottom_left;                           \
    left = (v16u8) __msa_splati_##TYPE((VTYPE) src_l, INDEX);       \
    row_r = (v8i16) __msa_ilvr_b((v16i8) top_right, (v16i8) left);  \
    row_l = (v8i16) __msa_ilvl_b((v16i8) top_right, (v16i8) left);  \
    row_r = (v8i16) __msa_dpadd_u_h(tmp_r, mult0, (v16u8) row_r);   \
    row_l = (v8i16) __msa_dpadd_u_h(tmp_l, mult1, (v16u8) row_l);   \
    row_r = (v8i16) __msa_srari_h((v8i16) row_r, RND);              \
    row_l = (v8i16) __msa_srari_h((v8i16) row_l, RND);              \
    row_r = (v8i16) __msa_pckev_b((v16i8) row_l, (v16i8) row_r);    \
    ST_SH(row_r, dst);                                              \
    dst += stride;                                                  \
}

#define HEVC_INTRA_PLANAR_16X16(RND)             \
{                                                \
    HEVC_INTRA_PLANAR_16PIX(0, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(1, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(2, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(3, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(4, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(5, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(6, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(7, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(8, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(9, RND, b, v16i8);   \
    HEVC_INTRA_PLANAR_16PIX(10, RND, b, v16i8);  \
    HEVC_INTRA_PLANAR_16PIX(11, RND, b, v16i8);  \
    HEVC_INTRA_PLANAR_16PIX(12, RND, b, v16i8);  \
    HEVC_INTRA_PLANAR_16PIX(13, RND, b, v16i8);  \
    HEVC_INTRA_PLANAR_16PIX(14, RND, b, v16i8);  \
    HEVC_INTRA_PLANAR_16PIX(15, RND, b, v16i8);  \
}

#define HEVC_ILV_INTRA_PLANAR_16X8(RND)         \
{                                               \
    HEVC_INTRA_PLANAR_16PIX(7, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(6, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(5, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(4, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(3, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(2, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(1, RND, h, v8i16);  \
    HEVC_INTRA_PLANAR_16PIX(0, RND, h, v8i16);  \
}

#define SLD_ST8X4(src0, dst, dst_stride)     \
{                                            \
    v16i8 src1_m, src2_m, src3_m;            \
                                             \
    src1_m = __msa_sldi_b(src0, src0, 2);    \
    src2_m = __msa_sldi_b(src0, src0, 4);    \
    src3_m = __msa_sldi_b(src0, src0, 6);    \
                                             \
    ST8x1_UB(src0, dst);                     \
    ST8x1_UB(src1_m, dst + 1 * dst_stride);  \
    ST8x1_UB(src2_m, dst + 2 * dst_stride);  \
    ST8x1_UB(src3_m, dst + 3 * dst_stride);  \
}

#define SLD_ST16X8(src0, src1, src2, src3, src4, src5, src6, src7, src8,  \
                   dst, dst_stride)                                       \
{                                                                         \
    src1 = __msa_sldi_b(src8, src0, 2);                                   \
    src2 = __msa_sldi_b(src8, src0, 4);                                   \
    src3 = __msa_sldi_b(src8, src0, 6);                                   \
    src4 = __msa_sldi_b(src8, src0, 8);                                   \
    src5 = __msa_sldi_b(src8, src0, 10);                                  \
    src6 = __msa_sldi_b(src8, src0, 12);                                  \
    src7 = __msa_sldi_b(src8, src0, 14);                                  \
                                                                          \
    ST_SB4(src0, src1, src2, src3, dst, dst_stride);                      \
    ST_SB4(src4, src5, src6, src7, dst + 4 * dst_stride, dst_stride);     \
}

#define SLD_ST16X16(src0, src1, src2, src3, src4, src5, src6, src7, src8,   \
                    src9, src10, src11, src12, src13, src14, src15, src16,  \
                    dst, dst_stride)                                        \
{                                                                           \
    src1 = __msa_sldi_b(src16, src0, 1);                                    \
    src2 = __msa_sldi_b(src16, src0, 2);                                    \
    src3 = __msa_sldi_b(src16, src0, 3);                                    \
    src4 = __msa_sldi_b(src16, src0, 4);                                    \
    src5 = __msa_sldi_b(src16, src0, 5);                                    \
    src6 = __msa_sldi_b(src16, src0, 6);                                    \
    src7 = __msa_sldi_b(src16, src0, 7);                                    \
    src8 = __msa_sldi_b(src16, src0, 8);                                    \
    src9 = __msa_sldi_b(src16, src0, 9);                                    \
    src10 = __msa_sldi_b(src16, src0, 10);                                  \
    src11 = __msa_sldi_b(src16, src0, 11);                                  \
    src12 = __msa_sldi_b(src16, src0, 12);                                  \
    src13 = __msa_sldi_b(src16, src0, 13);                                  \
    src14 = __msa_sldi_b(src16, src0, 14);                                  \
    src15 = __msa_sldi_b(src16, src0, 15);                                  \
                                                                            \
    ST_SB4(src1, src2, src3, src4, dst, dst_stride);                        \
    ST_SB4(src5, src6, src7, src8, dst + 4 * dst_stride, dst_stride);       \
    ST_SB4(src9, src10, src11, src12, dst + 8 * dst_stride, dst_stride);    \
    ST_SB4(src13, src14, src15, src16, dst + 12 * dst_stride, dst_stride);  \
}

#define COPY(src, dst, size, four_nt)                              \
{                                                                  \
     v16i8 src0_m, src1_m, src2_m, src3_m;                         \
     v16i8 src4_m, src5_m, src6_m, src7_m;                         \
                                                                   \
     switch (size)                                                 \
     {                                                             \
        case 4:                                                    \
            src0_m = LD_SB(src);                                   \
            ST_SB(src0_m, dst);                                    \
        break;                                                     \
                                                                   \
        case 8:                                                    \
            LD_SB2(src, 16, src0_m, src1_m);                       \
            ST_SB2(src0_m, src1_m, dst, 16);                       \
        break;                                                     \
                                                                   \
        case 16:                                                   \
            LD_SB4(src, 16, src0_m, src1_m, src2_m, src3_m);       \
            ST_SB4(src0_m, src1_m, src2_m, src3_m, dst, 16);       \
        break;                                                     \
                                                                   \
        case 32:                                                   \
            LD_SB4(src, 16, src0_m, src1_m, src2_m, src3_m);       \
            LD_SB4(src + 64, 16, src4_m, src5_m, src6_m, src7_m);  \
            ST_SB4(src0_m, src1_m, src2_m, src3_m, dst, 16);       \
            ST_SB4(src4_m, src5_m, src6_m, src7_m, dst + 64, 16);  \
     }                                                             \
                                                                   \
     dst[four_nt] = src[four_nt];                                  \
}

#define IPRED_SUBS_UH2_UH(in0, in1, out0, out1)  \
{                                                \
    out0 = __msa_subs_u_h(out0, in0);            \
    out1 = __msa_subs_u_h(out1, in1);            \
}

static void intra_predict_vert_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                       WORD32 dst_stride)
{
    UWORD32 out = LW(src);

    SW4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_8x4_msa(UWORD8 *src, UWORD8 *dst,
                                       WORD32 dst_stride)
{
    ULWORD64 out = LD(src);

    SD4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                       WORD32 dst_stride)
{
    ULWORD64 out = LD(src);

    SD4(out, out, out, out, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_16x8_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    v16u8 out = LD_UB(src);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                         WORD32 dst_stride)
{
    v16u8 out = LD_UB(src);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_vert_32x16_msa(UWORD8 *src, UWORD8 *dst,
                                         WORD32 dst_stride)
{
    v16u8 out0 = LD_UB(src);
    v16u8 out1 = LD_UB(src + 16);

    ST_UB8(out0, out0, out0, out0, out0, out0, out0, out0, dst, dst_stride);
    ST_UB8(out1, out1, out1, out1, out1, out1, out1, out1, dst + 16, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out0, out0, out0, out0, out0, out0, out0, out0, dst, dst_stride);
    ST_UB8(out1, out1, out1, out1, out1, out1, out1, out1, dst + 16, dst_stride);
}

static void intra_predict_vert_32x32_msa(UWORD8 *src, UWORD8 *dst,
                                         WORD32 dst_stride)
{
    UWORD32 row;
    v16u8 src0, src1;

    LD_UB2(src, 16, src0, src1);

    for(row = 16; row--;)
    {
        ST_UB2(src0, src1, dst, 16);
        dst += dst_stride;
        ST_UB2(src0, src1, dst, 16);
        dst += dst_stride;
    }
}

static void intra_predict_horiz_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    UWORD32 out0, out1, out2, out3;

    out0 = src[0] * 0x01010101;
    out1 = src[1] * 0x01010101;
    out2 = src[2] * 0x01010101;
    out3 = src[3] * 0x01010101;

    SW4(out0, out1, out2, out3, dst, dst_stride);
}

static void intra_ilv_predict_horiz_8x4_msa(UWORD8 *src, UWORD8 *dst,
                                            WORD32 dst_stride)
{
    UWORD16 *src_cbcr;
    ULWORD64 inp0, inp1, inp2, inp3;

    src_cbcr = (UWORD16 *)src;

    inp0 = src_cbcr[0] * 0x0001000100010001ull;
    inp1 = src_cbcr[1] * 0x0001000100010001ull;
    inp2 = src_cbcr[2] * 0x0001000100010001ull;
    inp3 = src_cbcr[3] * 0x0001000100010001ull;

    SD(inp0, dst);
    SD(inp1, dst + dst_stride);
    SD(inp2, dst + 2 * dst_stride);
    SD(inp3, dst + 3 * dst_stride);
}

static void intra_predict_horiz_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                        WORD32 dst_stride)
{
    ULWORD64 out0, out1, out2, out3, out4, out5, out6, out7;

    out0 = src[0] * 0x0101010101010101ull;
    out1 = src[1] * 0x0101010101010101ull;
    out2 = src[2] * 0x0101010101010101ull;
    out3 = src[3] * 0x0101010101010101ull;
    out4 = src[4] * 0x0101010101010101ull;
    out5 = src[5] * 0x0101010101010101ull;
    out6 = src[6] * 0x0101010101010101ull;
    out7 = src[7] * 0x0101010101010101ull;

    SD4(out0, out1, out2, out3, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(out4, out5, out6, out7, dst, dst_stride);
}

static void intra_ilv_predict_horiz_16x8_msa(UWORD8 *src, UWORD8 *dst,
                                             WORD32 dst_stride)
{
    WORD32 row;
    UWORD16 *src_cbcr;
    UWORD16 inp0, inp1, inp2, inp3;
    v8u16 src0, src1, src2, src3;

    src_cbcr = (UWORD16 *)src;
    for(row = 2; row--;)
    {
        inp0 = src_cbcr[0];
        inp1 = src_cbcr[1];
        inp2 = src_cbcr[2];
        inp3 = src_cbcr[3];
        src_cbcr += 4;

        src0 = (v8u16) __msa_fill_h(inp0);
        src1 = (v8u16) __msa_fill_h(inp1);
        src2 = (v8u16) __msa_fill_h(inp2);
        src3 = (v8u16) __msa_fill_h(inp3);

        ST_UH2(src0, src1, dst, dst_stride);
        dst += 2 * dst_stride;
        ST_UH2(src2, src3, dst, dst_stride);
        dst += 2 * dst_stride;
    }
}

static void intra_predict_horiz_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                          WORD32 dst_stride)
{
    UWORD32 row;
    UWORD8 inp0, inp1, inp2, inp3;
    v16u8 src0, src1, src2, src3;

    for(row = 4; row--;)
    {
        inp0 = src[0];
        inp1 = src[1];
        inp2 = src[2];
        inp3 = src[3];
        src += 4;

        src0 = (v16u8) __msa_fill_b(inp0);
        src1 = (v16u8) __msa_fill_b(inp1);
        src2 = (v16u8) __msa_fill_b(inp2);
        src3 = (v16u8) __msa_fill_b(inp3);

        ST_UB4(src0, src1, src2, src3, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void intra_ilv_predict_horiz_32x16_msa(UWORD8 *src, UWORD8 *dst,
                                              WORD32 dst_stride)
{
    WORD32 row;
    UWORD16 *src_cbcr;
    UWORD16 inp0, inp1, inp2, inp3;
    v8u16 src0, src1, src2, src3;

    src_cbcr = (UWORD16 *)src;
    for(row = 4; row--;)
    {
        inp0 = src_cbcr[0];
        inp1 = src_cbcr[1];
        inp2 = src_cbcr[2];
        inp3 = src_cbcr[3];
        src_cbcr += 4;

        src0 = (v8u16) __msa_fill_h(inp0);
        src1 = (v8u16) __msa_fill_h(inp1);
        src2 = (v8u16) __msa_fill_h(inp2);
        src3 = (v8u16) __msa_fill_h(inp3);

        ST_UH2(src0, src0, dst, 16);
        dst += dst_stride;
        ST_UH2(src1, src1, dst, 16);
        dst += dst_stride;
        ST_UH2(src2, src2, dst, 16);
        dst += dst_stride;
        ST_UH2(src3, src3, dst, 16);
        dst += dst_stride;
    }
}

static void intra_predict_horiz_32x32_msa(UWORD8 *src, UWORD8 *dst,
                                          WORD32 dst_stride)
{
    UWORD32 row;
    UWORD8 inp0, inp1, inp2, inp3;
    v16u8 src0, src1, src2, src3;

    for(row = 8; row--;)
    {
        inp0 = src[0];
        inp1 = src[1];
        inp2 = src[2];
        inp3 = src[3];
        src += 4;

        src0 = (v16u8) __msa_fill_b(inp0);
        src1 = (v16u8) __msa_fill_b(inp1);
        src2 = (v16u8) __msa_fill_b(inp2);
        src3 = (v16u8) __msa_fill_b(inp3);

        ST_UB2(src0, src0, dst, 16);
        dst += dst_stride;
        ST_UB2(src1, src1, dst, 16);
        dst += dst_stride;
        ST_UB2(src2, src2, dst, 16);
        dst += dst_stride;
        ST_UB2(src3, src3, dst, 16);
        dst += dst_stride;
    }
}

static void intra_predict_dc_4x4_msa(UWORD8 *src_top, UWORD8 *src_left,
                                     UWORD8 *dst, WORD32 dst_stride)
{
    UWORD32 val0, val1;
    v16i8 store, src = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LW(src_top);
    val1 = LW(src_left);
    INSERT_W2_SB(val0, val1, src);
    sum_h = __msa_hadd_u_h((v16u8) src, (v16u8) src);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 3);
    store = __msa_splati_b((v16i8) sum_w, 0);
    val0 = __msa_copy_u_w((v4i32) store, 0);

    SW4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_8x8_msa(UWORD8 *src_top, UWORD8 *src_left,
                                     UWORD8 *dst, WORD32 dst_stride)
{
    ULWORD64 val0, val1;
    v16i8 store;
    v16u8 src = { 0 };
    v8u16 sum_h;
    v4u32 sum_w;
    v2u64 sum_d;

    val0 = LD(src_top);
    val1 = LD(src_left);
    INSERT_D2_UB(val0, val1, src);
    sum_h = __msa_hadd_u_h(src, src);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_pckev_w((v4i32) sum_d, (v4i32) sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 4);
    store = __msa_splati_b((v16i8) sum_w, 0);
    val0 = __msa_copy_u_d((v2i64) store, 0);

    SD4(val0, val0, val0, val0, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(val0, val0, val0, val0, dst, dst_stride);
}

static void intra_predict_dc_16x16_msa(UWORD8 *src_top, UWORD8 *src_left,
                                       UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 top, left, out;
    v8u16 sum_h, sum_top, sum_left;
    v4u32 sum_w;
    v2u64 sum_d;

    top = LD_UB(src_top);
    left = LD_UB(src_left);
    HADD_UB2_UH(top, left, sum_top, sum_left);
    sum_h = sum_top + sum_left;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_pckev_w((v4i32) sum_d, (v4i32) sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 5);
    out = (v16u8) __msa_splati_b((v16i8) sum_w, 0);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_predict_dc_32x32_msa(UWORD8 *src_top, UWORD8 *src_left,
                                       UWORD8 *dst, WORD32 dst_stride)
{
    UWORD32 row;
    v16u8 top0, top1, left0, left1, out;
    v8u16 sum_h, sum_top0, sum_top1, sum_left0, sum_left1;
    v4u32 sum_w;
    v2u64 sum_d;

    LD_UB2(src_top, 16, top0, top1);
    LD_UB2(src_left, 16, left0, left1);
    HADD_UB2_UH(top0, top1, sum_top0, sum_top1);
    HADD_UB2_UH(left0, left1, sum_left0, sum_left1);
    sum_h = sum_top0 + sum_top1;
    sum_h += sum_left0 + sum_left1;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_pckev_w((v4i32) sum_d, (v4i32) sum_d);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_w = (v4u32) __msa_srari_w((v4i32) sum_d, 6);
    out = (v16u8) __msa_splati_b((v16i8) sum_w, 0);

    for(row = 16; row--;)
    {
        ST_UB2(out, out, dst, 16);
        dst += dst_stride;
        ST_UB2(out, out, dst, 16);
        dst += dst_stride;
    }
}

static void intra_ilv_predict_dc_8x4_msa(UWORD8 *src_top, UWORD8 *src_left,
                                         UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 top, left, out;
    v8u16 sum_h;
    v16i8 mask0 = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    v16i8 mask1 = {0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8};
    v4u32 sum_w;
    v2u64 sum_d;

    top = LD_UB(src_top);
    left = LD_UB(src_left);
    top = (v16u8) __msa_ilvr_d((v2i64) left, (v2i64) top);
    top = (v16u8) __msa_vshf_b(mask0, (v16i8) top, (v16i8) top);
    sum_h = (v8u16) __msa_hadd_u_h((v16u8) top, (v16u8) top);
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_d = (v2u64) __msa_srari_d((v2i64) sum_d, 3);
    out = (v16u8) __msa_vshf_b(mask1, (v16i8) sum_d, (v16i8) sum_d);

    ST8x4_UB(out, out, dst, dst_stride);
}

static void intra_ilv_predict_dc_16x8_msa(UWORD8 *src_top, UWORD8 *src_left,
                                          UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 top, left, out;
    v8u16 sum_h, sum_top, sum_left;
    v16i8 mask0 = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    v16i8 mask1 = {0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8};
    v4u32 sum_w;
    v2u64 sum_d;

    top = LD_UB(src_top);
    left = LD_UB(src_left);
    top = (v16u8) __msa_vshf_b(mask0, (v16i8) top, (v16i8) top);
    left = (v16u8) __msa_vshf_b(mask0, (v16i8) left, (v16i8) left);
    HADD_UB2_UH(top, left, sum_top, sum_left);
    sum_h = sum_top + sum_left;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_d = (v2u64) __msa_srari_d((v2i64) sum_d, 4);
    out = (v16u8) __msa_vshf_b(mask1, (v16i8) sum_d, (v16i8) sum_d);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
}

static void intra_ilv_predict_dc_32x16_msa(UWORD8 *src_top, UWORD8 *src_left,
                                           UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 top0, left0, top1, left1, out;
    v8u16 sum_h, sum_top, sum_left;
    v16i8 mask0 = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    v16i8 mask1 = {0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8};
    v4u32 sum_w;
    v2u64 sum_d;

    LD_UB2(src_top, 16, top0, top1);
    LD_UB2(src_left, 16, left0, left1);
    top0 = (v16u8) __msa_vshf_b(mask0, (v16i8) top0, (v16i8) top0);
    top1 = (v16u8) __msa_vshf_b(mask0, (v16i8) top1, (v16i8) top1);
    left0 = (v16u8) __msa_vshf_b(mask0, (v16i8) left0, (v16i8) left0);
    left1 = (v16u8) __msa_vshf_b(mask0, (v16i8) left1, (v16i8) left1);
    HADD_UB2_UH(top0, left0, sum_top, sum_left);
    sum_h = sum_top + sum_left;
    HADD_UB2_UH(top1, left1, sum_top, sum_left);
    sum_h += sum_top + sum_left;
    sum_w = __msa_hadd_u_w(sum_h, sum_h);
    sum_d = __msa_hadd_u_d(sum_w, sum_w);
    sum_d = (v2u64) __msa_srari_d((v2i64) sum_d, 5);
    out = (v16u8) __msa_vshf_b(mask1, (v16i8) sum_d, (v16i8) sum_d);

    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst + 16, dst_stride);
    dst += 8 * dst_stride;
    ST_UB8(out, out, out, out, out, out, out, out, dst, dst_stride);
    ST_UB8(out, out, out, out, out, out, out, out, dst + 16, dst_stride);
}

static void hevc_post_intra_pred_filt_vert_4_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 stride)
{
    UWORD32 src_data;
    v16i8 zero = { 0 };
    v8i16 vec0, vec1, vec2 = { 0 };

    src_data = LW(src_left);

    vec2 = (v8i16) __msa_insert_w((v4i32) vec2, 0, src_data);

    vec0 = __msa_fill_h(src_top[-1]);
    vec1 = __msa_fill_h(src_top[0]);

    vec2 = (v8i16) __msa_ilvr_b(zero, (v16i8) vec2);
    vec2 -= vec0;
    vec2 = SRAI_H(vec2, 1);
    vec2 += vec1;
    vec2 = CLIP_SH_0_255(vec2);

    dst[0] = __msa_copy_u_b((v16i8) vec2, 0);
    dst[stride] = __msa_copy_u_b((v16i8) vec2, 2);
    dst[2 * stride] = __msa_copy_u_b((v16i8) vec2, 4);
    dst[3 * stride] = __msa_copy_u_b((v16i8) vec2, 6);
}

static void hevc_post_intra_pred_filt_vert_8_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 stride)
{
    UWORD16 val0, val1, val2, val3;
    ULWORD64 src_data1;
    v8i16 vec0, vec1, vec2;
    v16i8 zero = { 0 };

    src_data1 = LD(src_left);

    vec2 = (v8i16) __msa_insert_d((v2i64) zero, 0, src_data1);

    vec0 = __msa_fill_h(src_top[-1]);
    vec1 = __msa_fill_h(src_top[0]);

    vec2 = (v8i16) __msa_ilvr_b(zero, (v16i8) vec2);
    vec2 -= vec0;
    vec2 = SRAI_H(vec2, 1);
    vec2 += vec1;
    vec2 = CLIP_SH_0_255(vec2);

    val0 = vec2[0];
    val1 = vec2[1];
    val2 = vec2[2];
    val3 = vec2[3];

    dst[0] = val0;
    dst[stride] = val1;
    dst[2 * stride] = val2;
    dst[3 * stride] = val3;

    val0 = vec2[4];
    val1 = vec2[5];
    val2 = vec2[6];
    val3 = vec2[7];

    dst[4 * stride] = val0;
    dst[5 * stride] = val1;
    dst[6 * stride] = val2;
    dst[7 * stride] = val3;
}

static void hevc_post_intra_pred_filt_vert_16_msa(UWORD8 *src_top,
                                                  UWORD8 *src_left,
                                                  UWORD8 *dst, WORD32 stride)
{
    WORD32 col;
    v16u8 src;
    v8i16 vec0, vec1, vec2, vec3;

    src = LD_UB(src_left);

    vec0 = __msa_fill_h(src_top[-1]);
    vec1 = __msa_fill_h(src_top[0]);

    UNPCK_UB_SH(src, vec2, vec3);
    SUB2(vec2, vec0, vec3, vec0, vec2, vec3);
    SRAI_H2_SH(vec2, vec3, 1);
    ADD2(vec2, vec1, vec3, vec1, vec2, vec3);
    CLIP_SH2_0_255(vec2, vec3);

    src = (v16u8) __msa_pckev_b((v16i8) vec3, (v16i8) vec2);

    for(col = 0; col < 16; col++)
    {
        dst[stride * col] = src[col];
    }
}

static void hevc_post_intra_pred_filt_horiz_4_msa(UWORD8 *src_top,
                                                  UWORD8 *src_left,
                                                  UWORD8 *dst)
{
    UWORD32 val0;
    v8i16 src0_r, src_top_val, src_left_val;
    v16i8 src0 = { 0 };
    v16i8 zero = { 0 };

    val0 = LW(src_top);
    src0 = (v16i8) __msa_insert_w((v4i32) src0, 0, val0);
    src_top_val = __msa_fill_h(src_top[-1]);
    src_left_val = __msa_fill_h(src_left[0]);

    src0_r = (v8i16) __msa_ilvr_b(zero, src0);

    src0_r -= src_top_val;
    src0_r = SRAI_H(src0_r, 1);
    src0_r += src_left_val;
    src0_r = CLIP_SH_0_255(src0_r);
    src0 = __msa_pckev_b((v16i8) src0_r, (v16i8) src0_r);
    val0 = __msa_copy_s_w((v4i32) src0, 0);
    SW(val0, dst);
}

static void hevc_post_intra_pred_filt_horiz_8_msa(UWORD8 *src_top,
                                                  UWORD8 *src_left,
                                                  UWORD8 *dst)
{
    ULWORD64 val0;
    v16i8 src0 = { 0 };
    v8i16 src0_r, src_top_val, src_left_val;
    v16i8 zero = { 0 };

    val0 = LD(src_top);
    src0 = (v16i8) __msa_insert_d((v2i64) src0, 0, val0);
    src_top_val = __msa_fill_h(src_top[-1]);
    src_left_val = __msa_fill_h(src_left[0]);

    src0_r = (v8i16) __msa_ilvr_b(zero, src0);

    src0_r -= src_top_val;
    src0_r = SRAI_H(src0_r, 1);
    src0_r += src_left_val;
    src0_r = CLIP_SH_0_255(src0_r);
    src0 = __msa_pckev_b((v16i8) src0_r, (v16i8) src0_r);
    ST8x1_UB(src0, dst);
}

static void hevc_post_intra_pred_filt_horiz_16_msa(UWORD8 *src_top,
                                                   UWORD8 *src_left,
                                                   UWORD8 *dst)
{
    v16i8 src0;
    v8i16 src0_r, src0_l, src_left_val, src_top_val;

    src_left_val = __msa_fill_h(src_left[0]);

    src0 = LD_SB(src_top);
    src_top_val = __msa_fill_h(src_top[-1]);

    UNPCK_UB_SH(src0, src0_r, src0_l);
    SUB2(src0_r, src_top_val, src0_l, src_top_val, src0_r, src0_l);
    SRAI_H2_SH(src0_r, src0_l, 1);
    ADD2(src0_r, src_left_val, src0_l, src_left_val, src0_r, src0_l);
    CLIP_SH2_0_255(src0_r, src0_l);
    src0 = __msa_pckev_b((v16i8) src0_l, (v16i8) src0_r);
    ST_SB(src0, dst);
}

static void hevc_post_intra_pred_filt_dc_4x4_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 stride,
                                                 UWORD8 dc_val)
{
    UWORD8 *tmp_dst = dst;
    UWORD32 val0, val1;
    v16i8 src = { 0 };
    v16u8 store;
    v16i8 zero = { 0 };
    v8u16 vec0, vec1;

    val0 = LW(src_top);
    val1 = LW(src_left);
    INSERT_W2_SB(val0, val1, src);
    vec0 = (v8u16) __msa_fill_h(dc_val);
    vec1 = (v8u16) __msa_ilvr_b(zero, src);

    vec1 += vec0;
    vec0 += vec0;
    vec1 += vec0;

    vec1 = (v8u16) __msa_srari_h((v8i16) vec1, 2);
    store = (v16u8) __msa_pckev_b((v16i8) vec1, (v16i8) vec1);
    val1 = (src_left[0] + 2 * dc_val + src_top[0] + 2) >> 2;
    store = (v16u8) __msa_insert_b((v16i8) store, 0, val1);
    val0 = __msa_copy_u_w((v4i32) store, 0);
    SW(val0, tmp_dst);

    tmp_dst[stride * 1] = store[5];
    tmp_dst[stride * 2] = store[6];
    tmp_dst[stride * 3] = store[7];
}

static void hevc_post_intra_pred_filt_dc_8x8_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 stride,
                                                 UWORD8 dc_val)
{
    UWORD8 *tmp_dst = dst;
    UWORD32 col, val;
    ULWORD64 val0, val1;
    v16u8 src = { 0 };
    v16u8 store;
    v8u16 vec0, vec1, vec2;
    v16i8 zero = { 0 };

    val0 = LD(src_top);
    val1 = LD(src_left);
    INSERT_D2_UB(val0, val1, src);
    vec0 = (v8u16) __msa_fill_h(dc_val);
    ILVRL_B2_UH(zero, src, vec1, vec2);

    vec1 += vec0;
    vec2 += vec0;
    vec0 += vec0;
    vec1 += vec0;
    vec2 += vec0;
    SRARI_H2_UH(vec1, vec2, 2);
    store = (v16u8) __msa_pckev_b((v16i8) vec2, (v16i8) vec1);
    val = (src_left[0] + 2 * dc_val + src_top[0] + 2) >> 2;
    store = (v16u8) __msa_insert_b((v16i8) store, 0, val);
    ST8x1_UB(store, tmp_dst);

    for(col = 1; col < 8; col++)
    {
        tmp_dst[stride * col] = store[col + 8];
    }
}

static void hevc_post_intra_pred_filt_dc_16x16_msa(UWORD8 *src_top,
                                                   UWORD8 *src_left,
                                                   UWORD8 *dst, WORD32 stride,
                                                   UWORD8 dc_val)
{
    UWORD8 *tmp_dst = dst;
    UWORD32 col, val;
    v16u8 src_above1, store0, src_left1, store1;
    v8u16 vec0, vec1, vec2, vec3, vec4;
    v16i8 zero = { 0 };

    src_above1 = LD_UB(src_top);
    src_left1 = LD_UB(src_left);
    vec0 = (v8u16) __msa_fill_h(dc_val);

    ILVRL_B2_UH(zero, src_above1, vec1, vec2);
    ILVRL_B2_UH(zero, src_left1, vec3, vec4);
    ADD2(vec1, vec0, vec2, vec0, vec1, vec2);
    ADD2(vec3, vec0, vec4, vec0, vec3, vec4);
    vec0 += vec0;
    ADD2(vec1, vec0, vec2, vec0, vec1, vec2);
    ADD2(vec3, vec0, vec4, vec0, vec3, vec4);
    SRARI_H4_UH(vec1, vec2, vec3, vec4, 2);
    PCKEV_B2_UB(vec2, vec1, vec4, vec3, store0, store1);
    val = (src_left[0] + 2 * dc_val + src_top[0] + 2) >> 2;
    store0 = (v16u8) __msa_insert_b((v16i8) store0, 0, val);
    ST_UB(store0, tmp_dst);

    for(col = 1; col < 16; col++)
    {
        tmp_dst[stride * col] = store1[col];
    }
}

static void hevc_intra_pred_plane_4x4_msa(UWORD8 *src_top,
                                          UWORD8 *src_left,
                                          UWORD8 *dst, WORD32 stride)
{
    UWORD32 src0, src1;
    v16u8 src_t, src_l, top_right, zero = {0};
    v8i16 row0, row1, row2, row3;
    v16u8 left0, left1, left2, left3;
    v16u8 mult = {3, 1, 2, 2, 1, 3, 0, 4, 3, 1, 2, 2, 1, 3, 0, 4};
    v8u16 tmp, src_tr, bottom_left;
    v8u16 const3 = (v8u16) __msa_ldi_h(3);

    src0 = LW(src_top);
    src1 = LW(src_left);

    src_t = (v16u8) __msa_insert_w((v4i32) zero, 0, src0);
    src_l = (v16u8) __msa_insert_w((v4i32) zero, 0, src1);

    bottom_left = (v8u16) __msa_fill_h(src_left[4]);
    top_right = (v16u8) __msa_fill_b(src_top[4]);

    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    tmp = src_tr * const3;
    tmp += bottom_left;

    left0 = (v16u8) __msa_splati_b((v16i8) src_l, 0);
    left1 = (v16u8) __msa_splati_b((v16i8) src_l, 1);
    left2 = (v16u8) __msa_splati_b((v16i8) src_l, 2);
    left3 = (v16u8) __msa_splati_b((v16i8) src_l, 3);

    ILVR_B4_SH(top_right, left0, top_right, left1, top_right, left2,
               top_right, left3, row0, row1, row2, row3);

    row0 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row0);
    tmp = tmp - src_tr + bottom_left;
    row1 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row1);
    tmp = tmp - src_tr + bottom_left;
    row2 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row2);
    tmp = tmp - src_tr + bottom_left;
    row3 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row3);

    SRARI_H4_SH(row0, row1, row2, row3, 3);
    PCKEV_B2_SH(row1, row0, row3, row2, row0, row1);

    ST4x4_UB(row0, row1, 0, 2, 0, 2, dst, stride);
}

static void hevc_intra_pred_plane_8x8_msa(UWORD8 *src_top, UWORD8 *src_left,
                                          UWORD8 *dst, WORD32 stride)
{
    ULWORD64 src0, src1;
    v16u8 src_t, src_l, top_right, zero = {0};
    v8i16 row0, row1, row2, row3, row4, row5, row6, row7;
    v16u8 left0, left1, left2, left3, left4, left5, left6, left7;
    v16u8 mult = {7, 1, 6, 2, 5, 3, 4, 4, 3, 5, 2, 6, 1, 7, 0, 8};
    v8u16 tmp, src_tr, bottom_left;
    v8u16 const7 = (v8u16) __msa_ldi_h(7);

    src0 = LD(src_top);
    src1 = LD(src_left);

    src_t = (v16u8) __msa_insert_d((v2i64) zero, 0, src0);
    src_l = (v16u8) __msa_insert_d((v2i64) zero, 0, src1);

    bottom_left = (v8u16) __msa_fill_h(src_left[8]);
    top_right = (v16u8) __msa_fill_b(src_top[8]);

    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    tmp = src_tr * const7;
    tmp += bottom_left;

    left0 = (v16u8) __msa_splati_b((v16i8) src_l, 0);
    left1 = (v16u8) __msa_splati_b((v16i8) src_l, 1);
    left2 = (v16u8) __msa_splati_b((v16i8) src_l, 2);
    left3 = (v16u8) __msa_splati_b((v16i8) src_l, 3);
    left4 = (v16u8) __msa_splati_b((v16i8) src_l, 4);
    left5 = (v16u8) __msa_splati_b((v16i8) src_l, 5);
    left6 = (v16u8) __msa_splati_b((v16i8) src_l, 6);
    left7 = (v16u8) __msa_splati_b((v16i8) src_l, 7);

    ILVR_B4_SH(top_right, left0, top_right, left1, top_right, left2, top_right,
               left3, row0, row1, row2, row3);
    ILVR_B4_SH(top_right, left4, top_right, left5, top_right, left6, top_right,
               left7, row4, row5, row6, row7);

    row0 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row0);
    tmp = tmp - src_tr + bottom_left;
    row1 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row1);
    tmp = tmp - src_tr + bottom_left;
    row2 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row2);
    tmp = tmp - src_tr + bottom_left;
    row3 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row3);
    tmp = tmp - src_tr + bottom_left;
    row4 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row4);
    tmp = tmp - src_tr + bottom_left;
    row5 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row5);
    tmp = tmp - src_tr + bottom_left;
    row6 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row6);
    tmp = tmp - src_tr + bottom_left;
    row7 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row7);

    SRARI_H4_SH(row0, row1, row2, row3, 4);
    SRARI_H4_SH(row4, row5, row6, row7, 4);
    PCKEV_B4_SH(row1, row0, row3, row2, row5, row4, row7, row6,
                row0, row1, row2, row3);

    ST8x8_UB(row0, row1, row2, row3, dst, stride);
}

static void hevc_intra_pred_plane_16x16_msa(UWORD8 *src_top, UWORD8 *src_left,
                                            UWORD8 *dst, WORD32 stride)
{
    v16u8 src_t, src_l, top_right, zero = {0};
    v8i16 row_r, row_l;
    v16u8 left;
    v16u8 mult0 = {15, 1, 14, 2, 13, 3, 12, 4, 11, 5, 10, 6, 9, 7, 8, 8};
    v16u8 mult1 = {7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15, 0, 16};
    v8u16 tmp_r, tmp_l, src_tl, src_tr, bottom_left;
    v8u16 const16 = (v8u16) __msa_ldi_h(16);

    src_t = LD_UB(src_top);
    src_l = LD_UB(src_left);

    bottom_left = (v8u16) __msa_fill_h(src_left[16]);
    top_right = (v16u8) __msa_fill_b(src_top[16]);

    ILVRL_B2_UH(zero, src_t, src_tr, src_tl);
    tmp_r = src_tr * const16;
    tmp_l = src_tl * const16;

    HEVC_INTRA_PLANAR_16X16(5);
}

static void hevc_intra_pred_plane_32x32_msa(UWORD8 *src_top, UWORD8 *src_left,
                                            UWORD8 *dst, WORD32 stride)
{
    UWORD8 *dst_tmp = dst + 16;
    v16u8 src_t, src_l, top_right, zero = {0};
    v8i16 row_r, row_l;
    v16u8 left, mult1;
    v16u8 mult0 = {31, 1, 30, 2, 29, 3, 28, 4, 27, 5, 26, 6, 25, 7, 24, 8};
    v16u8 sub = {-8, 8, -8, 8, -8, 8, -8, 8, -8, 8, -8, 8, -8, 8, -8, 8};
    v8u16 tmp_r, tmp_l, src_tl, src_tr, bottom_left;
    v8u16 const32 = (v8u16) __msa_ldi_h(32);

    bottom_left = (v8u16) __msa_fill_h(src_left[32]);
    top_right = (v16u8) __msa_fill_b(src_top[32]);

    src_t = LD_UB(src_top);
    src_l = LD_UB(src_left);
    ILVRL_B2_UH(zero, src_t, src_tr, src_tl);
    tmp_r = src_tr * const32;
    tmp_l = src_tl * const32;

    mult1 = mult0 + sub;
    HEVC_INTRA_PLANAR_16X16(6);
    src_l = LD_UB(src_left + 16);
    HEVC_INTRA_PLANAR_16X16(6);

    src_t = LD_UB(src_top + 16);
    src_l = LD_UB(src_left);
    ILVRL_B2_UH(zero, src_t, src_tr, src_tl);
    tmp_r = src_tr * const32;
    tmp_l = src_tl * const32;

    dst = dst_tmp;
    mult0 = mult1 + sub;
    mult1 = mult0 + sub;
    HEVC_INTRA_PLANAR_16X16(6);
    src_l = LD_UB(src_left + 16);
    HEVC_INTRA_PLANAR_16X16(6);
}

static void hevc_intra_pred_mode2_4width_msa(UWORD8 *src_left,
                                             UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src = {0};
    v16i8 mask = {1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6, 4, 5, 6, 7};

    src = LD_SB(src_left);
    src = __msa_vshf_b(mask, src, src);

    ST4x4_UB(src, src, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_intra_pred_mode2_8width_msa(UWORD8 *src_left,
                                             UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 mask0 = {1, 2, 3, 4, 5, 6, 7, 8, 2, 3, 4, 5, 6, 7, 8, 9};
    v16i8 src0, vec0, vec1, mask1;

    mask1 = ADDVI_B(mask0, 2);

    src0 = LD_SB(src_left);
    VSHF_B2_SB(src0, src0, src0, src0, mask0, mask1, vec0, vec1);
    ST8x4_UB(vec0, vec1, dst, dst_stride);
    dst += (4 * dst_stride);

    VSHF_B2_SB(src0, src0, src0, src0, ADDVI_B(mask0, 4),
               ADDVI_B(mask1, 4), vec0, vec1);
    ST8x4_UB(vec0, vec1, dst, dst_stride);
}

static void hevc_intra_pred_mode2_16width_msa(UWORD8 *src_left,
                                              UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15, src16;

    LD_SB2(src_left, 16, src0, src16);
    SLD_ST16X16(src0, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src16,
                dst, dst_stride);
}

static void hevc_intra_pred_mode2_32width_msa(UWORD8 *src_left,
                                              UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 src16, src32, src48;

    LD_SB4(src_left, 16, src0, src16, src32, src48);
    SLD_ST16X16(src0, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src16,
                dst, dst_stride);
    SLD_ST16X16(src16, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src32,
                dst + 16, dst_stride);

    ST_SB4(src1, src2, src3, src4, dst + 16 * dst_stride, dst_stride);
    ST_SB4(src5, src6, src7, src8, dst + 20 * dst_stride, dst_stride);
    ST_SB4(src9, src10, src11, src12, dst + 24 * dst_stride, dst_stride);
    ST_SB4(src13, src14, src15, src32, dst + 28 * dst_stride, dst_stride);

    SLD_ST16X16(src32, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src48,
                dst + 16 * dst_stride + 16, dst_stride);
}

static void hevc_intra_pred_mode18_34_4width_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 dst_stride,
                                                 WORD32 mode)
{
    WORD32 res0, res1, res2, res3;
    UWORD8 ref_array[16];
    UWORD8 *ref = ref_array;
    v16i8 src;
    v16i8 mask = {4, 5, 6, 7, 3, 4, 5, 6, 2, 3, 4, 5, 1, 2, 3, 4};

    if(18 == mode)
    {
        ref[0] = src_left[3];
        ref[1] = src_left[2];
        ref[2] = src_left[1];
        ref[3] = src_left[0];
        *(ULWORD64 *) (ref + 4) = *((ULWORD64 *) (src_top - 1));

        src = LD_SB(ref);
        src = __msa_vshf_b(mask, src, src);

        res0 = __msa_copy_u_w((v4i32) src, 0);
        res1 = __msa_copy_u_w((v4i32) src, 1);
        res2 = __msa_copy_u_w((v4i32) src, 2);
        res3 = __msa_copy_u_w((v4i32) src, 3);
    }
    else
    {
        src = LD_SB(src_top);
        src = __msa_vshf_b(mask, src, src);

        res3 = __msa_copy_u_w((v4i32) src, 0);
        res2 = __msa_copy_u_w((v4i32) src, 1);
        res1 = __msa_copy_u_w((v4i32) src, 2);
        res0 = __msa_copy_u_w((v4i32) src, 3);
    }

    SW4(res0, res1, res2, res3, dst, dst_stride);
}

static void hevc_intra_pred_mode18_34_8width_msa(UWORD8 *src_top,
                                                 UWORD8 *src_left,
                                                 UWORD8 *dst, WORD32 dst_stride,
                                                 WORD32 mode)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 mask = {6, 5, 4, 3, 2, 1, 0, 16, 17, 18, 19, 20, 21, 22, 23, 24};

    if(18 == mode)
    {
        src0 = LD_SB(src_left);
        src1 = LD_SB(src_top - 1);
        src0 = __msa_vshf_b(mask, src1, src0);

        dst += 7 * dst_stride;
        dst_stride = -dst_stride;
    }
    else
    {
        src0 = LD_SB(src_top + 1);
    }

    src1 = __msa_sldi_b(src0, src0, 1);
    src2 = __msa_sldi_b(src0, src0, 2);
    src3 = __msa_sldi_b(src0, src0, 3);
    src4 = __msa_sldi_b(src0, src0, 4);
    src5 = __msa_sldi_b(src0, src0, 5);
    src6 = __msa_sldi_b(src0, src0, 6);
    src7 = __msa_sldi_b(src0, src0, 7);

    ST8x1_UB(src0, dst + 0 * dst_stride);
    ST8x1_UB(src1, dst + 1 * dst_stride);
    ST8x1_UB(src2, dst + 2 * dst_stride);
    ST8x1_UB(src3, dst + 3 * dst_stride);
    ST8x1_UB(src4, dst + 4 * dst_stride);
    ST8x1_UB(src5, dst + 5 * dst_stride);
    ST8x1_UB(src6, dst + 6 * dst_stride);
    ST8x1_UB(src7, dst + 7 * dst_stride);
}

static void hevc_intra_pred_mode18_34_16width_msa(UWORD8 *src_top,
                                                  UWORD8 *src_left,
                                                  UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  WORD32 mode)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15, src16;
    v16i8 mask = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    if(18 == mode)
    {
        src0 = LD_SB(src_left);
        src0 = __msa_vshf_b(mask, src0, src0);
        src16 = LD_SB(src_top - 1);

        dst += (15 * dst_stride);
        dst_stride = -dst_stride;
    }
    else
    {
        LD_SB2(src_top, 16, src0, src16);
    }

    SLD_ST16X16(src0, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src16,
                dst, dst_stride);
}

static void hevc_intra_pred_mode18_34_32width_msa(UWORD8 *src_top,
                                                  UWORD8 *src_left,
                                                  UWORD8 *dst,
                                                  WORD32 dst_stride,
                                                  WORD32 mode)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 src16, src32, src48;
    v16i8 mask = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    if(18 == mode)
    {
        LD_SB2(src_left, 16, src16, src0);
        LD_SB2(src_top - 1, 16, src32, src48);
        VSHF_B2_SB(src0, src0, src16, src16, mask, mask, src0, src16);

        dst += (31 * dst_stride);
        dst_stride = -dst_stride;
    }
    else
    {
        LD_SB4(src_top, 16, src0, src16, src32, src48);
    }

    SLD_ST16X16(src0, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src16,
                dst, dst_stride);
    SLD_ST16X16(src16, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src32,
                dst + 16, dst_stride);

    ST_SB4(src1, src2, src3, src4, dst + 16 * dst_stride, dst_stride);
    ST_SB4(src5, src6, src7, src8, dst + 20 * dst_stride, dst_stride);
    ST_SB4(src9, src10, src11, src12, dst + 24 * dst_stride, dst_stride);
    ST_SB4(src13, src14, src15, src32, dst + 28 * dst_stride, dst_stride);

    SLD_ST16X16(src32, src1, src2, src3, src4, src5, src6, src7, src8,
                src9, src10, src11, src12, src13, src14, src15, src48,
                dst + 16 * dst_stride + 16, dst_stride);
}

static void hevc_intra_pred_mode_3_to_9_4width_msa(UWORD8 *src, UWORD8 *dst,
                                                   WORD32 dst_stride,
                                                   WORD32 mode)
{
    WORD32 pos, ang;
    v16i8 mask1 = { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 };
    v8u16 dst0;

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    dst0 = HEVC_INTRA_2TAP_FILT_4X4(src, pos, ang);
    dst0 = (v8u16) __msa_vshf_b((v16i8) mask1, (v16i8) dst0, (v16i8) dst0);
    ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_intra_pred_mode_11_to_17_4width_msa(UWORD8 *src_top,
                                                     UWORD8 *src_left,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 1], *src;
    WORD32 pos, col, ref_idx, size = 4;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 mask1 = { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 };
    v8u16 dst0;

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    tmp_array[size - 1] = src_top[0];
    col = LW(src_left);
    SW(col, tmp_array + size);

    src = tmp_array + size - 1;
    ref_idx = (size * ang) >> 5;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_top[inv_ang_sum >> 8];
    }

    src = src + 1;
    pos = ang;

    dst0 = HEVC_INTRA_2TAP_FILT_4X4(src, pos, ang);
    dst0 = (v8u16) __msa_vshf_b((v16i8) mask1, (v16i8) dst0, (v16i8) dst0);
    ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_intra_pred_mode_27_to_33_4width_msa(UWORD8 *src, UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    WORD32 pos, ang;
    v8u16 dst0;

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;
    dst0 = HEVC_INTRA_2TAP_FILT_4X4(src, pos, ang);
    ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_intra_pred_mode_19_to_25_4width_msa(UWORD8 *src_top,
                                                     UWORD8 *src_left,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    UWORD8 *src;
    WORD32 col, ang, size = 4;
    WORD32 ref_idx, inv_ang, inv_ang_sum, pos;
    UWORD8 tmp_array[(2 * 64) + 1];
    v8u16 dst0;

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 12];

    src = tmp_array + size - 1;

    col = LW(src_top);
    SW(col, tmp_array + size - 1);
    tmp_array[2 * size -1] = src_top[size];

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_left[(inv_ang_sum >> 8) - 1];
    }

    src = src + 1;
    pos = ang;

    dst0 = HEVC_INTRA_2TAP_FILT_4X4(src, pos, ang);
    ST4x4_UB(dst0, dst0, 0, 1, 2, 3, dst, dst_stride);
}

static void hevc_intra_pred_mode_3_to_9_8width_msa(UWORD8 *src, UWORD8 *dst,
                                                   WORD32 dst_stride,
                                                   WORD32 mode)
{

    WORD32 idx, fract, pos, ang;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v8i16 out0, out1, out2, out3;
    v8i16 out10_r, out32_r;
    v8i16 out10_l, out32_l;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(1);
    ILVEV_B2_SH(dst0, dst1, dst2, dst3, out0, out1);
    HEVC_INTRA_2TAP_FILT_8X4(1);
    ILVEV_B2_SH(dst0, dst1, dst2, dst3, out2, out3);
    /* transpose */
    ILVR_H2_SH(out1, out0, out3, out2, out10_r, out32_r);
    ILVL_H2_SH(out1, out0, out3, out2, out10_l, out32_l);
    ILVR_W2_SH(out32_r, out10_r, out32_l, out10_l, out0, out2);
    ILVL_W2_SH(out32_r, out10_r, out32_l, out10_l, out1, out3);
    ST8x8_UB(out0, out1, out2, out3, dst, dst_stride);
}

static void hevc_intra_pred_mode_11_to_17_8width_msa(UWORD8 *src_top,
                                                     UWORD8 *src_left,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 1], *src;
    ULWORD64 temp;
    WORD32 fract, pos, col, idx, ref_idx, size = 8;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v8i16 out0, out1, out2, out3;
    v8i16 out10_r, out32_r, out10_l, out32_l;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    tmp_array[size - 1] = src_top[0];
    temp = LD(src_left);
    SD(temp, tmp_array + size);

    src = tmp_array + size - 1;
    ref_idx = (size * ang) >> 5;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_top[(inv_ang_sum >> 8)];
    }

    src = src + 1;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(1);
    ILVEV_B2_SH(dst0, dst1, dst2, dst3, out0, out1);
    HEVC_INTRA_2TAP_FILT_8X4(1);
    ILVEV_B2_SH(dst0, dst1, dst2, dst3, out2, out3);
    /* transpose */
    ILVR_H2_SH(out1, out0, out3, out2, out10_r, out32_r);
    ILVL_H2_SH(out1, out0, out3, out2, out10_l, out32_l);
    ILVR_W2_SH(out32_r, out10_r, out32_l, out10_l, out0, out2);
    ILVL_W2_SH(out32_r, out10_r, out32_l, out10_l, out1, out3);
    ST8x8_UB(out0, out1, out2, out3, dst, dst_stride);
}

static void hevc_intra_pred_mode_27_to_33_8width_msa(UWORD8 *src, UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{

    WORD32 idx, fract, pos, ang;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v8i16 out0, out1;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(1);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    ST8x4_UB(out0, out1, dst, dst_stride);
    HEVC_INTRA_2TAP_FILT_8X4(1);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    ST8x4_UB(out0, out1, dst + 4 * dst_stride, dst_stride);
}

static void hevc_intra_pred_mode_19_to_25_8width_msa(UWORD8 *src_top,
                                                     UWORD8 *src_left,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    WORD32 col;
    ULWORD64 temp;
    WORD32 ang, idx, size = 8;
    WORD32 inv_ang, inv_ang_sum, pos, fract;
    WORD32 ref_idx;
    UWORD8 tmp_array[(2 * 64) + 1];
    UWORD8 *src;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v8i16 out0, out1;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 12];

    src = tmp_array + size - 1;

    temp = LD(src_top);
    SD(temp, tmp_array + size - 1);
    tmp_array[2 * size -1] = src_top[size];

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_left[(inv_ang_sum >> 8) - 1];
    }

    src = src + 1;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(1);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    ST8x4_UB(out0, out1, dst, dst_stride);
    HEVC_INTRA_2TAP_FILT_8X4(1);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    ST8x4_UB(out0, out1, dst + 4 * dst_stride, dst_stride);
}

static void hevc_intra_pred_mode_3_to_9_16width_msa(UWORD8 *src, UWORD8 *dst,
                                                    WORD32 dst_stride,
                                                    WORD32 mode)
{
    WORD32 idx;
    WORD32 fract;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_16x4(1);
        TRANSPOSE16x4_UB(dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst, dst_stride);
        ST4x8_UB(out2, out3, dst + 8 * dst_stride, dst_stride);
        dst += 4;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_11_to_17_16width_msa(UWORD8 *src_top,
                                                      UWORD8 *src_left,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 1], *src;
    WORD32 fract, pos, col, idx, ref_idx, size = 16;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    tmp_array[size - 1] = src_top[0];
    src0 = LD_SB(src_left);
    ST_SB(src0, tmp_array + size);

    src = tmp_array + size - 1;
    ref_idx = (size * ang) >> 5;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_top[(inv_ang_sum >> 8)];
    }

    src = src + 1;
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_16x4(1);
        TRANSPOSE16x4_UB(dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst, dst_stride);
        ST4x8_UB(out2, out3, dst + 8 * dst_stride, dst_stride);
        dst += 4;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_27_to_33_16width_msa(UWORD8 *src, UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 idx;
    WORD32 fract;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_16x4(1);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH4(out0, out1, out2, out3, dst, dst_stride);
        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_19_to_25_16width_msa(UWORD8 *src_top,
                                                      UWORD8 *src_left,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 col;
    WORD32 ang, idx, size = 16;
    WORD32 inv_ang, inv_ang_sum, pos, fract;
    WORD32 ref_idx;
    UWORD8 tmp_array[(2 * 64) + 1];
    UWORD8 *src;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 12];

    src = tmp_array + size - 1;

    src0 = LD_SB(src_top);
    ST_SB(src0, tmp_array + size - 1);
    tmp_array[2 * size -1] = src_top[size];

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_left[(inv_ang_sum >> 8) - 1];
    }

    src = src + 1;
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_16x4(1);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH4(out0, out1, out2, out3, dst, dst_stride);
        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_3_to_9_32width_msa(UWORD8 *src, UWORD8 *dst,
                                                    WORD32 dst_stride,
                                                    WORD32 mode)
{
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 fract;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    for(col = 0; col < 8; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(1);
        TRANSPOSE16x4_UB(dst0, dst1, dst4, dst5, dst8, dst9, dst12, dst13,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst, dst_stride);
        ST4x8_UB(out2, out3, dst + 8 * dst_stride, dst_stride);

        TRANSPOSE16x4_UB(dst2, dst3, dst6, dst7, dst10, dst11, dst14, dst15,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst + 16 * dst_stride, dst_stride);
        ST4x8_UB(out2, out3, dst + 24 * dst_stride, dst_stride);

        dst += 4;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_11_to_17_32width_msa(UWORD8 *src_top,
                                                      UWORD8 *src_left,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 1], *src;
    WORD32 fract, pos, col, ref_idx, size = 32;
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    tmp_array[size - 1] = src_top[0];
    LD_SB2(src_left, 16, src0, src1);
    ST_SB2(src0, src1, tmp_array + size, 16);

    src = tmp_array + size - 1;
    ref_idx = (size * ang) >> 5;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_top[(inv_ang_sum >> 8)];
    }

    src = src + 1;
    pos = ang;

    for(col = 0; col < 8; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(1);
        TRANSPOSE16x4_UB(dst0, dst1, dst4, dst5, dst8, dst9, dst12, dst13,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst, dst_stride);
        ST4x8_UB(out2, out3, dst + 8 * dst_stride, dst_stride);

        TRANSPOSE16x4_UB(dst2, dst3, dst6, dst7, dst10, dst11, dst14, dst15,
                         out0, out1, out2, out3);
        ST4x8_UB(out0, out1, dst + 16 * dst_stride, dst_stride);
        ST4x8_UB(out2, out3, dst + 24 * dst_stride, dst_stride);

        dst += 4;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_27_to_33_32width_msa(UWORD8 *src, UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 fract;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    for(col = 0; col < 8; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(1);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH2(out0, out1, dst, 16);
        ST_SH2(out2, out3, dst + dst_stride, 16);

        PCKEV_B2_SH(dst9, dst8, dst11, dst10, out0, out1);
        PCKEV_B2_SH(dst13, dst12, dst15, dst14, out2, out3);
        ST_SH2(out0, out1, dst + 2 * dst_stride, 16);
        ST_SH2(out2, out3, dst + 3 * dst_stride, 16);

        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_intra_pred_mode_19_to_25_32width_msa(UWORD8 *src_top,
                                                      UWORD8 *src_left,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 col;
    WORD32 ang, size = 32;
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 inv_ang, inv_ang_sum, pos, fract;
    WORD32 ref_idx;
    UWORD8 tmp_array[(2 * 64) + 1];
    UWORD8 *src;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8};

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 12];

    src = tmp_array + size - 1;

    LD_SB2(src_top, 16, src0, src1);
    ST_SB2(src0, src1, tmp_array + size - 1, 16);
    tmp_array[2 * size -1] = src_top[size];

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;

    for(col = -1; col > ref_idx; col--)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_left[(inv_ang_sum >> 8) - 1];
    }

    src = src + 1;
    pos = ang;

    for(col = 0; col < 8; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(1);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH2(out0, out1, dst, 16);
        ST_SH2(out2, out3, dst + dst_stride, 16);

        PCKEV_B2_SH(dst9, dst8, dst11, dst10, out0, out1);
        PCKEV_B2_SH(dst13, dst12, dst15, dst14, out2, out3);
        ST_SH2(out0, out1, dst + 2 * dst_stride, 16);
        ST_SH2(out2, out3, dst + 3 * dst_stride, 16);

        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_ilv_intra_pred_mode_18_34_4x4_msa(UWORD8 *src,
                                                   UWORD8 *dst,
                                                   WORD32 dst_stride,
                                                   WORD32 mode)
{
    WORD32 idx, size = 4;
    v16i8 src0, src1, src2, src3;

    if(18 == mode)
    {
        idx = -1 * size;
        src0 = LD_SB(src + 4 * size + 2 * idx + 2);
        dst += ((size - 1) * dst_stride);
        dst_stride = -dst_stride;
    }
    else
    {
        idx = 1;
        src0 = LD_SB(src + 4 * size + 2 * idx + 2);
    }

    SLD_ST8X4(src0, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_18_34_8x8_msa(UWORD8 *src,
                                                   UWORD8 *dst,
                                                   WORD32 dst_stride,
                                                   WORD32 mode)
{
    WORD32 idx, size = 8;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;

    if(18 == mode)
    {
        idx = -1 * size;
        LD_SB2(src + 4 * size + 2 * idx + 2, 16, src0, src8);
        dst += ((size - 1) * dst_stride);
        dst_stride = -dst_stride;
    }
    else
    {
        idx = 1;
        LD_SB2(src + 4 * size + 2 * idx + 2, 16, src0, src8);
    }

    SLD_ST16X8(src0, src1, src2, src3, src4, src5, src6, src7, src8,
               dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_18_34_16x16_msa(UWORD8 *src,
                                                     UWORD8 *dst,
                                                     WORD32 dst_stride,
                                                     WORD32 mode)
{
    WORD32 idx, size = 16;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src16, src24;

    if(18 == mode)
    {
        idx = -1 * size;
        LD_SB4(src + 4 * size + 2 * idx + 2, 16, src0, src8, src16, src24);
        dst += ((size - 1) * dst_stride);
        dst_stride = -dst_stride;
    }
    else
    {
        idx = 1;
        LD_SB4(src + 4 * size + 2 * idx + 2, 16, src0, src8, src16, src24);
    }

    SLD_ST16X8(src0, src1, src2, src3, src4, src5, src6, src7, src8,
               dst, dst_stride);
    SLD_ST16X8(src8, src1, src2, src3, src4, src5, src6, src7, src16,
               dst + 16, dst_stride);
    ST_SB4(src8, src1, src2, src3, dst + 8 * dst_stride, dst_stride);
    ST_SB4(src4, src5, src6, src7, dst + 12 * dst_stride, dst_stride);
    SLD_ST16X8(src16, src1, src2, src3, src4, src5, src6, src7, src24,
               dst + 8 * dst_stride + 16, dst_stride);
}

static void hevc_ilv_intra_pred_mode2_4x4_msa(UWORD8 *src,
                                              UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src0, src1, src2, src3;
    v8i16 mask = {7, 6, 5, 4, 3, 2, 1, 0};

    src0 = LD_SB(src - 2);
    /* reverse the left array */
    src0 = (v16i8) __msa_vshf_h(mask, (v8i16) src0, (v8i16) src0);
    SLD_ST8X4(src0, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode2_8x8_msa(UWORD8 *source,
                                              UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v8i16 mask = {7, 6, 5, 4, 3, 2, 1, 0};

    LD_SB2(source - 2, 16, src8, src0);
    /* reverse the left array */
    src0 = (v16i8) __msa_vshf_h(mask, (v8i16) src0, (v8i16) src0);
    src8 = (v16i8) __msa_vshf_h(mask, (v8i16) src8, (v8i16) src8);
    SLD_ST16X8(src0, src1, src2, src3, src4, src5, src6, src7, src8,
               dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode2_16x16_msa(UWORD8 *source,
                                                UWORD8 *dst, WORD32 dst_stride)
{
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7, src8;
    v16i8 src16, src24;
    v8i16 mask = {7, 6, 5, 4, 3, 2, 1, 0};

    LD_SB4(source - 2, 16, src24, src16, src8, src0);
    /* reverse the left array */
    src0 = (v16i8) __msa_vshf_h(mask, (v8i16) src0, (v8i16) src0);
    src8 = (v16i8) __msa_vshf_h(mask, (v8i16) src8, (v8i16) src8);
    src16 = (v16i8) __msa_vshf_h(mask, (v8i16) src16, (v8i16) src16);
    src24 = (v16i8) __msa_vshf_h(mask, (v8i16) src24, (v8i16) src24);

    SLD_ST16X8(src0, src1, src2, src3, src4, src5, src6, src7, src8,
               dst, dst_stride);
    SLD_ST16X8(src8, src1, src2, src3, src4, src5, src6, src7,
               src16, dst + 16, dst_stride);
    ST_SB4(src8, src1, src2, src3, dst + 8 * dst_stride, dst_stride);
    ST_SB4(src4, src5, src6, src7, dst + 12 * dst_stride, dst_stride);
    SLD_ST16X8(src16, src1, src2, src3, src4, src5, src6, src7, src24,
               dst + 8 * dst_stride + 16, dst_stride);
}
static void hevc_ilv_intra_pred_mode_3_to_9_4x4_msa(UWORD8 *source,
                                                    UWORD8 *dst,
                                                    WORD32 dst_stride,
                                                    WORD32 mode)
{
    UWORD8 src[4 * 4];
    WORD32 idx, fract, pos, ang, size = 4, col;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8i16 out0, out1, out2, out3;
    v8u16 dst0, dst1, dst2, dst3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    for(col = 0; col < 4 * size; col += 2)
    {
        src[col + 1] = source[4 * size - 1 - col];
        src[col] = source[4 * size - 1 - (col + 1)];
    }

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;
    HEVC_INTRA_2TAP_FILT_8X4(2);
    /* transpose */
    PCKEV_B2_SH(dst2, dst0, dst3, dst1, out0, out1);
    ILVRL_H2_SH(out1, out0, out2, out3);
    ILVRL_W2_SH(out3, out2, out0, out1);
    ST8x4_UB(out0, out1, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_3_to_9_8x8_msa(UWORD8 *source,
                                                    UWORD8 *dst,
                                                    WORD32 dst_stride,
                                                    WORD32 mode)
{
    UWORD8 src[8 * 4];
    WORD32 idx, fract, pos, ang, size = 8, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 op0, op1, op2, op3, op4, op5, op6, op7;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    for(col = 0; col < 4 * size; col += 2)
    {
        src[col + 1] = source[4 * size - 1 - col];
        src[col] = source[4 * size - 1 - (col + 1)];
    }

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, op0, op1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, op2, op3);
    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, op4, op5);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, op6, op7);

    /* transpose */
    ILVR_H4_SH(op1, op0, op3, op2, op5, op4, op7, op6, out0, out1, out2, out3);
    ILVR_W2_SH(out1, out0, out3, out2, out4, out5);
    ILVL_W2_SH(out1, out0, out3, out2, out6, out7);
    ILVR_D2_SH(out5, out4, out7, out6, out0, out2);
    out1 = (v8i16) __msa_ilvl_d((v2i64) out5, (v2i64) out4);
    out3 = (v8i16) __msa_ilvl_d((v2i64) out7, (v2i64) out6);
    ST_SH4(out0, out1, out2, out3, dst, dst_stride);

    /* transpose */
    ILVL_H4_SH(op1, op0, op3, op2, op5, op4, op7, op6, out0, out1, out2, out3);
    ILVR_W2_SH(out1, out0, out3, out2, out4, out5);
    ILVL_W2_SH(out1, out0, out3, out2, out6, out7);
    ILVR_D2_SH(out5, out4, out7, out6, out0, out2);
    out1 = (v8i16) __msa_ilvl_d((v2i64) out5, (v2i64) out4);
    out3 = (v8i16) __msa_ilvl_d((v2i64) out7, (v2i64) out6);
    ST_SH4(out0, out1, out2, out3, dst + 4 * dst_stride, dst_stride);
}

static void hevc_ilv_intra_pred_mode_3_to_9_16x16_msa(UWORD8 *source,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 src[16 * 4];
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 fract, size = 16;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    for(col = 0; col < 4 * size; col += 2)
    {
        src[col + 1] = source[4 * size - 1 - col];
        src[col] = source[4 * size - 1 - (col + 1)];
    }

    ang = gai4_ihevc_ang_table[mode];
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(2);
        PCKEV_B2_SB(dst1, dst0, dst3, dst2, src0, src1);
        PCKEV_B2_SB(dst5, dst4, dst7, dst6, src2, src3);
        PCKEV_B2_SB(dst9, dst8, dst11, dst10, src4, src5);
        PCKEV_B2_SB(dst13, dst12, dst15, dst14, src6, src7);

        ILVR_H2_SH(src2, src0, src6, src4, out0, out1);
        ILVL_H2_SH(src2, src0, src6, src4, out2, out3);
        ILVR_W2_SH(out1, out0, out3, out2, out4, out6);
        ILVL_W2_SH(out1, out0, out3, out2, out5, out7);
        ST8x8_UB(out4, out5, out6, out7, dst, dst_stride);

        ILVR_H2_SH(src3, src1, src7, src5, out0, out1);
        ILVL_H2_SH(src3, src1, src7, src5, out2, out3);
        ILVR_W2_SH(out1, out0, out3, out2, out4, out6);
        ILVL_W2_SH(out1, out0, out3, out2, out5, out7);
        ST8x8_UB(out4, out5, out6, out7, dst + 8 * dst_stride, dst_stride);

        dst += 8;
        fract += 4;
    }
}

static void hevc_ilv_intra_pred_mode_11_to_17_4x4_msa(UWORD8 *src_uv,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, idx, ref_idx, size = 4;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8i16 out0, out1, out2, out3;
    v8u16 dst0, dst1, dst2, dst3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    for(col = 0; col < (2 * (size + 1)); col += 2)
    {
        tmp_array[col + (2 * (size - 1))] = src_uv[(4 * size) - col];
        tmp_array[col + 1 + (2 * (size - 1))] = src_uv[(4 * size) - col + 1];
    }

    src = tmp_array + (2 * (size - 1));
    ref_idx = (size * ang) >> 5;

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) + ((inv_ang_sum >> 8) << 1)];
        src[col + 1] = src_uv[((4 * size) + 1) + ((inv_ang_sum >> 8) << 1)];
    }

    src = src + 2;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(2);
    /* transpose */
    PCKEV_B2_SH(dst2, dst0, dst3, dst1, out0, out1);
    ILVRL_H2_SH(out1, out0, out2, out3);
    ILVRL_W2_SH(out3, out2, out0, out1);
    ST8x4_UB(out0, out1, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_11_to_17_8x8_msa(UWORD8 *src_uv,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, idx, ref_idx, size = 8;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v8i16 op0, op1, op2, op3, op4, op5, op6, op7;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    for(col = 0; col < (2 * (size + 1)); col += 2)
    {
        tmp_array[col + (2 * (size - 1))] = src_uv[(4 * size) - col];
        tmp_array[col + 1 + (2 * (size - 1))] = src_uv[(4 * size) - col + 1];
    }

    src = tmp_array + (2 * (size - 1));
    ref_idx = (size * ang) >> 5;

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) + ((inv_ang_sum >> 8) << 1)];
        src[col + 1] = src_uv[((4 * size) + 1) + ((inv_ang_sum >> 8) << 1)];
    }

    src = src + 2;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, op0, op1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, op2, op3);
    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, op4, op5);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, op6, op7);

    ILVR_H4_SH(op1, op0, op3, op2, op5, op4, op7, op6, out0, out1, out2, out3);
    ILVR_W2_SH(out1, out0, out3, out2, out4, out5);
    ILVL_W2_SH(out1, out0, out3, out2, out6, out7);
    ILVR_D2_SH(out5, out4, out7, out6, out0, out2);
    out1 = (v8i16) __msa_ilvl_d((v2i64) out5, (v2i64) out4);
    out3 = (v8i16) __msa_ilvl_d((v2i64) out7, (v2i64) out6);
    ST_SH4(out0, out1, out2, out3, dst, dst_stride);

    /* transpose */
    ILVL_H4_SH(op1, op0, op3, op2, op5, op4, op7, op6, out0, out1, out2, out3);
    ILVR_W2_SH(out1, out0, out3, out2, out4, out5);
    ILVL_W2_SH(out1, out0, out3, out2, out6, out7);
    ILVR_D2_SH(out5, out4, out7, out6, out0, out2);
    out1 = (v8i16) __msa_ilvl_d((v2i64) out5, (v2i64) out4);
    out3 = (v8i16) __msa_ilvl_d((v2i64) out7, (v2i64) out6);
    ST_SH4(out0, out1, out2, out3, dst + 4 * dst_stride, dst_stride);
}

static void hevc_ilv_intra_pred_mode_11_to_17_16x16_msa(UWORD8 *src_uv,
                                                        UWORD8 *dst,
                                                        WORD32 dst_stride,
                                                        WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, ref_idx, size = 16;
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3, out4, out5, out6, out7;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table[mode];
    inv_ang = gai4_ihevc_inv_ang_table[mode - 11];

    for(col = 0; col < (2 * (size + 1)); col += 2)
    {
        tmp_array[col + (2 * (size - 1))] = src_uv[(4 * size) - col];
        tmp_array[col + 1 + (2 * (size - 1))] = src_uv[(4 * size) - col + 1];
    }

    src = tmp_array + (2 * (size - 1));
    ref_idx = (size * ang) >> 5;

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) + ((inv_ang_sum >> 8) << 1)];
        src[col + 1] = src_uv[((4 * size) + 1) + ((inv_ang_sum >> 8) << 1)];
    }

    src = src + 2;
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(2);
        PCKEV_B2_SB(dst1, dst0, dst3, dst2, src0, src1);
        PCKEV_B2_SB(dst5, dst4, dst7, dst6, src2, src3);
        PCKEV_B2_SB(dst9, dst8, dst11, dst10, src4, src5);
        PCKEV_B2_SB(dst13, dst12, dst15, dst14, src6, src7);

        ILVR_H2_SH(src2, src0, src6, src4, out0, out1);
        ILVL_H2_SH(src2, src0, src6, src4, out2, out3);
        ILVR_W2_SH(out1, out0, out3, out2, out4, out6);
        ILVL_W2_SH(out1, out0, out3, out2, out5, out7);
        ST8x8_UB(out4, out5, out6, out7, dst, dst_stride);

        ILVR_H2_SH(src3, src1, src7, src5, out0, out1);
        ILVL_H2_SH(src3, src1, src7, src5, out2, out3);
        ILVR_W2_SH(out1, out0, out3, out2, out4, out6);
        ILVL_W2_SH(out1, out0, out3, out2, out5, out7);
        ST8x8_UB(out4, out5, out6, out7, dst + 8 * dst_stride, dst_stride);

        dst += 8;
        fract += 4;
    }
}

static void hevc_ilv_intra_pred_mode_19_to_25_4x4_msa(UWORD8 *src_uv,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    ULWORD64 tmp;
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, idx, ref_idx, size = 4;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table_chroma[mode];
    inv_ang = gai4_ihevc_inv_ang_table_chroma[mode - 12];

    tmp_array[2 * (size - 1)] = src_uv[(4 * size)];
    tmp_array[1 + (2 * (size - 1))] = src_uv[(4 * size) + 1];

    tmp = LD(src_uv + (4 * size) + 2);
    SD(tmp,  tmp_array + 2 + (2 * (size - 1)));

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;
    src = tmp_array + (2 * (size - 1));

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) - (inv_ang_sum >> 8) * 2];
        src[col + 1] = src_uv[((4 * size) + 1) - (inv_ang_sum >> 8) * 2];
    }

    src = src + 2;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_8X4(2);
    dst0 = (v8u16) __msa_pckev_b((v16i8) dst1, (v16i8) dst0);
    dst1 = (v8u16) __msa_pckev_b((v16i8) dst3, (v16i8) dst2);
    ST8x4_UB(dst0, dst1, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_19_to_25_8x8_msa(UWORD8 *src_uv,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, idx, ref_idx, size = 8;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table_chroma[mode];
    inv_ang = gai4_ihevc_inv_ang_table_chroma[mode - 12];

    tmp_array[2 * (size - 1)] = src_uv[(4 * size)];
    tmp_array[1 + (2 * (size - 1))] = src_uv[(4 * size) + 1];

    src0 = LD_SB(src_uv + (4 * size) + 2);
    ST_SB(src0,  tmp_array + 2 + (2 * (size - 1)));

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;
    src = tmp_array + (2 * (size - 1));

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) - (inv_ang_sum >> 8) * 2];
        src[col + 1] = src_uv[((4 * size) + 1) - (inv_ang_sum >> 8) * 2];
    }

    src = src + 2;
    pos = ang;

    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
    ST_SH4(out0, out1, out2, out3, dst, dst_stride);
    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
    ST_SH4(out0, out1, out2, out3, dst + 4 * dst_stride, dst_stride);
}

static void hevc_ilv_intra_pred_mode_19_to_25_16x16_msa(UWORD8 *src_uv,
                                                        UWORD8 *dst,
                                                        WORD32 dst_stride,
                                                        WORD32 mode)
{
    UWORD8 tmp_array[2 * 64 + 2], *src;
    WORD32 fract, pos, col, ref_idx, size = 16;
    WORD32 idx0, idx1, idx2, idx3;
    WORD32 ang, inv_ang, inv_ang_sum = 128;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    ang = gai4_ihevc_ang_table_chroma[mode];
    inv_ang = gai4_ihevc_inv_ang_table_chroma[mode - 12];

    tmp_array[2 * (size - 1)] = src_uv[(4 * size)];
    tmp_array[1 + (2 * (size - 1))] = src_uv[(4 * size) + 1];

    LD_SB2(src_uv + (4 * size) + 2, 16, src0, src1);
    ST_SB2(src0,  src1, tmp_array + 2 + (2 * (size - 1)), 16);

    ref_idx = (size * ang) >> 5;
    inv_ang_sum = 128;
    src = tmp_array + (2 * (size - 1));

    for(col = -2; col > (2 * ref_idx); col -= 2)
    {
        inv_ang_sum += inv_ang;
        src[col] = src_uv[(4 * size) - (inv_ang_sum >> 8) * 2];
        src[col + 1] = src_uv[((4 * size) + 1) - (inv_ang_sum >> 8) * 2];
    }

    src = src + 2;
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(2);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH2(out0, out1, dst, 16);
        ST_SH2(out2, out3, dst + dst_stride, 16);
        PCKEV_B2_SH(dst9, dst8, dst11, dst10, out0, out1);
        PCKEV_B2_SH(dst13, dst12, dst15, dst14, out2, out3);
        ST_SH2(out0, out1, dst + 2 * dst_stride, 16);
        ST_SH2(out2, out3, dst + 3 * dst_stride, 16);
        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_ilv_intra_pred_mode_27_to_33_4x4_msa(UWORD8 *src,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 idx, fract, pos, ang, size = 4;
    v16i8 src0, src1, src2, src3;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    src = src + 4 * size + 2;
    ang = gai4_ihevc_ang_table_chroma[mode];
    pos = ang;
    HEVC_INTRA_2TAP_FILT_8X4(2);
    dst0 = (v8u16) __msa_pckev_b((v16i8) dst1, (v16i8) dst0);
    dst1 = (v8u16) __msa_pckev_b((v16i8) dst3, (v16i8) dst2);
    ST8x4_UB(dst0, dst1, dst, dst_stride);
}

static void hevc_ilv_intra_pred_mode_27_to_33_8x8_msa(UWORD8 *src,
                                                      UWORD8 *dst,
                                                      WORD32 dst_stride,
                                                      WORD32 mode)
{
    WORD32 idx, fract, pos, ang, size = 8;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    src = src + 4 * size + 2;
    ang = gai4_ihevc_ang_table_chroma[mode];
    pos = ang;
    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
    ST_SH4(out0, out1, out2, out3, dst, dst_stride);
    HEVC_INTRA_2TAP_FILT_16x4(2);
    PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
    PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
    ST_SH4(out0, out1, out2, out3, dst + 4 * dst_stride, dst_stride);
}

static void hevc_ilv_intra_pred_mode_27_to_33_16x16_msa(UWORD8 *src,
                                                        UWORD8 *dst,
                                                        WORD32 dst_stride,
                                                        WORD32 mode)
{
    WORD32 idx0, idx1, idx2, idx3, size = 16;
    WORD32 fract;
    WORD32 pos, ang, col;
    v16i8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16i8 src8, src9, src10, src11, src12, src13, src14, src15;
    v16i8 filt0, filt1, filt2, filt3;
    v8u16 dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7;
    v8u16 dst8, dst9, dst10, dst11, dst12, dst13, dst14, dst15;
    v8i16 out0, out1, out2, out3;
    v16i8 mask = { 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9 };

    src = src + 4 * size + 2;
    ang = gai4_ihevc_ang_table_chroma[mode];
    pos = ang;

    for(col = 0; col < 4; col++)
    {
        HEVC_INTRA_2TAP_FILT_32x8(2);
        PCKEV_B2_SH(dst1, dst0, dst3, dst2, out0, out1);
        PCKEV_B2_SH(dst5, dst4, dst7, dst6, out2, out3);
        ST_SH2(out0, out1, dst, 16);
        ST_SH2(out2, out3, dst + dst_stride, 16);
        PCKEV_B2_SH(dst9, dst8, dst11, dst10, out0, out1);
        PCKEV_B2_SH(dst13, dst12, dst15, dst14, out2, out3);
        ST_SH2(out0, out1, dst + 2 * dst_stride, 16);
        ST_SH2(out2, out3, dst + 3 * dst_stride, 16);
        dst += 4 * dst_stride;
        fract += 4;
    }
}

static void hevc_ilv_intra_pred_plane_4x4_msa(UWORD8 *src, UWORD8 *dst,
                                              WORD32 stride)
{
    WORD32 size = 4;
    UWORD8 *src_top = src + 4 * size + 2;
    UWORD8 *src_left = src + 4 * size - (2 * size + 2);
    ULWORD64 src0, src1;
    v16u8 src_t, src_l, top_right_u, top_right_v, zero = {0};
    v8i16 row0, row1, row2, row3;
    v16u8 left0, left1, left2, left3;
    v16u8 mult = {3, 1, 3, 1, 2, 2, 2, 2, 1, 3, 1, 3, 0, 4, 0, 4};
    v8u16 tmp, src_tr, bottom_left_u, bottom_left_v;
    v8u16 const3 = (v8u16) __msa_ldi_h(3);

    src0 = LD(src_top);
    src1 = LD(src_left + 2);

    src_t = (v16u8) __msa_insert_d((v2i64) zero, 0, src0);
    src_l = (v16u8) __msa_insert_d((v2i64) zero, 0, src1);

    bottom_left_u = (v8u16) __msa_fill_h(src_left[0]);
    bottom_left_v = (v8u16) __msa_fill_h(src_left[1]);
    bottom_left_u = (v8u16) __msa_ilvr_h((v8i16) bottom_left_v,
                                         (v8i16) bottom_left_u);

    top_right_u = (v16u8) __msa_fill_b(src_top[8]);
    top_right_v = (v16u8) __msa_fill_b(src_top[9]);
    top_right_u = (v16u8) __msa_ilvr_b((v16i8) top_right_v,
                                       (v16i8) top_right_u);

    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    tmp = src_tr * const3;
    tmp += bottom_left_u;

    left0 = (v16u8) __msa_splati_h((v8i16) src_l, 3);
    left1 = (v16u8) __msa_splati_h((v8i16) src_l, 2);
    left2 = (v16u8) __msa_splati_h((v8i16) src_l, 1);
    left3 = (v16u8) __msa_splati_h((v8i16) src_l, 0);

    ILVR_B4_SH(top_right_u, left0, top_right_u, left1, top_right_u, left2,
               top_right_u, left3, row0, row1, row2, row3);

    row0 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row0);
    tmp = tmp - src_tr + bottom_left_u;
    row1 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row1);
    tmp = tmp - src_tr + bottom_left_u;
    row2 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row2);
    tmp = tmp - src_tr + bottom_left_u;
    row3 = (v8i16) __msa_dpadd_u_h(tmp, mult, (v16u8) row3);

    SRARI_H4_SH(row0, row1, row2, row3, 3);
    PCKEV_B2_SH(row1, row0, row3, row2, row0, row1);
    ST8x4_UB(row0, row1, dst, stride);
}

static void hevc_ilv_intra_pred_plane_8x8_msa(UWORD8 *src, UWORD8 *dst,
                                              WORD32 stride)
{
    WORD32 size = 8;
    UWORD8 *src_top = src + 4 * size + 2;
    UWORD8 *src_left = src + 4 * size - (2 * size + 2);
    v16u8 src_t, src_l, zero = {0};
    v16u8 top_right, top_right_u, top_right_v;
    v8i16 row_r, row_l;
    v16u8 left;
    v16u8 mult0 = {7, 1, 7, 1, 6, 2, 6, 2, 5, 3, 5, 3, 4, 4, 4, 4};
    v16u8 mult1 = {3, 5, 3, 5, 2, 6, 2, 6, 1, 7, 1, 7, 0, 8, 0, 8};
    v8u16 tmp_r, tmp_l, src_tl, src_tr;
    v8u16 bottom_left, bottom_left_u, bottom_left_v;
    v8u16 const8 = (v8u16) __msa_ldi_h(8);

    src_t = LD_UB(src_top);
    src_l = LD_UB(src_left + 2);

    bottom_left_u = (v8u16) __msa_fill_h(src_left[0]);
    bottom_left_v = (v8u16) __msa_fill_h(src_left[1]);
    bottom_left = (v8u16) __msa_ilvr_h((v8i16) bottom_left_v,
                                       (v8i16) bottom_left_u);

    top_right_u = (v16u8) __msa_fill_b(src_top[16]);
    top_right_v = (v16u8) __msa_fill_b(src_top[17]);
    top_right = (v16u8) __msa_ilvr_b((v16i8) top_right_v, (v16i8) top_right_u);

    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    src_tl = (v8u16) __msa_ilvl_b((v16i8) zero, (v16i8) src_t);
    tmp_r = src_tr * const8;
    tmp_l = src_tl * const8;

    HEVC_ILV_INTRA_PLANAR_16X8(4);
}

static void hevc_ilv_intra_pred_plane_16x16_msa(UWORD8 *src, UWORD8 *dst,
                                                WORD32 stride)
{
    WORD32 size = 16;
    UWORD8 *src_top = src + 4 * size + 2;
    UWORD8 *src_left = src + 4 * size - (2 * size + 2);
    UWORD8 *dst_tmp = dst + 16;
    v16u8 src_t, src_l, zero = {0};
    v16u8 top_right, top_right_u, top_right_v;
    v8i16 row_r, row_l;
    v16u8 left, mult1;
    v16u8 mult0 = { 15, 1, 15, 1, 14, 2, 14, 2, 13, 3, 13, 3, 12, 4, 12, 4 };
    v16u8 sub = { -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4, -4, 4 };
    v8u16 tmp_r, tmp_l, src_tl, src_tr;
    v8u16 bottom_left, bottom_left_u, bottom_left_v;
    v8u16 const16 = (v8u16) __msa_ldi_h(16);

    bottom_left_u = (v8u16) __msa_fill_h(src_left[0]);
    bottom_left_v = (v8u16) __msa_fill_h(src_left[1]);
    bottom_left = (v8u16) __msa_ilvr_h((v8i16) bottom_left_v,
                                       (v8i16) bottom_left_u);

    top_right_u = (v16u8) __msa_fill_b(src_top[32]);
    top_right_v = (v16u8) __msa_fill_b(src_top[33]);
    top_right = (v16u8) __msa_ilvr_b((v16i8) top_right_v, (v16i8) top_right_u);

    src_left += 2;
    src_t = LD_UB(src_top);
    src_l = LD_UB(src_left + 16);
    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    src_tl = (v8u16) __msa_ilvl_b((v16i8) zero, (v16i8) src_t);
    tmp_r = src_tr * const16;
    tmp_l = src_tl * const16;

    mult1 = mult0 + sub;
    HEVC_ILV_INTRA_PLANAR_16X8(5);
    src_l = LD_UB(src_left);
    HEVC_ILV_INTRA_PLANAR_16X8(5);

    src_t = LD_UB(src_top + 16);
    src_l = LD_UB(src_left + 16);
    src_tr = (v8u16) __msa_ilvr_b((v16i8) zero, (v16i8) src_t);
    src_tl = (v8u16) __msa_ilvl_b((v16i8) zero, (v16i8) src_t);
    tmp_r = src_tr * const16;
    tmp_l = src_tl * const16;

    dst = dst_tmp;
    mult0 = mult1 + sub;
    mult1 = mult0 + sub;
    HEVC_ILV_INTRA_PLANAR_16X8(5);
    src_l = LD_UB(src_left);
    HEVC_ILV_INTRA_PLANAR_16X8(5);
}

static void hevc_intra_pred_ref_filtering_msa(UWORD8 *src,
                                              UWORD8 *dst,
                                              WORD32 size,
                                              WORD32 mode,
                                              WORD32 smoothing_flag)
{
    WORD32 filter_flag;
    WORD32 four_nt = 4 * size;
    UWORD8 au1_flt[(4 * 64) + 1];
    WORD32 bi_linear_int_flag = 0;
    WORD32 abs_cond_left_flag = 0;
    WORD32 abs_cond_top_flag = 0;
    WORD32 dc_val = 1 << (8 - 5);

    filter_flag = gau1_intra_pred_ref_filter[mode] & (1 << (CTZ(size) - 2));
    if(0 == filter_flag)
    {
        if(src == dst)
        {
            return;
        }
        else
        {
            COPY(src, dst, size, four_nt);
        }
    }
    else
    {
        /* If strong intra smoothin is enabled and transform size is 32 */
        if((1 == smoothing_flag) && (32 == size))
        {
            /* Strong Intra Filtering */
            abs_cond_top_flag = (ABS(src[2 * size] + src[4 * size]
                               - (2 * src[3 * size]))) < dc_val;
            abs_cond_left_flag = (ABS(src[2 * size] + src[0]
                                - (2 * src[size]))) < dc_val;

            bi_linear_int_flag = ((1 == abs_cond_left_flag)
                                && (1 == abs_cond_top_flag));
        }

        /* Extremities Untouched*/
        au1_flt[0] = src[0];
        au1_flt[4 * size] = src[4 * size];

        /* Strong filtering of reference samples */
        if(1 == bi_linear_int_flag)
        {
            UWORD16 temp0, temp1;
            v16i8 src0, src1, op0, op1;

            au1_flt[2 * size] = src[2 * size];

            temp0 = src[0] | src[2 * size] << 8;
            temp1 = src[2 * size] | src[4 * size] << 8;
            src0 = (v16i8) __msa_fill_h(temp0);
            src1 = (v16i8) __msa_fill_h(temp1);

            switch(size)
            {
                case 4:
                    {
                        v8u16 out0, out1;
                        v16u8 mult0 = { 7, 1, 6, 2, 5, 3, 4, 4,
                                        3, 5, 2, 6, 1, 7, 0, 8 };

                        DOTP_UB2_UH(src0, src1, mult0, mult0, out0, out1);
                        SRARI_H2_UH(out0, out1, 6);
                        src0 = __msa_pckev_b((v16i8) out1, (v16i8) out0);
                        ST_SB(src0, au1_flt + 1);
                    }
                    break;
                case 8:
                    {
                        v8u16 out0, out1, out2, out3;
                        v16u8 mult0 = { 15, 1, 14, 2, 13, 3, 12, 4,
                                        11, 5, 10, 6, 9, 7, 8, 8};
                        v16u8 mult1 = { 7, 9, 6, 10, 5, 11, 4, 12,
                                        3, 13, 2, 14, 1, 15, 0, 16 };

                        DOTP_UB4_UH(src0, src0, src1, src1, mult0, mult1,
                                    mult0, mult1, out0, out1, out2, out3);
                        SRARI_H4_UH(out0, out1, out2, out3, 6);
                        PCKEV_B2_SB(out1, out0, out3, out2, src0, src1);
                        ST_SB2(src0, src1, au1_flt + 1, 16);
                    }
                    break;
                case 16:
                    {
                        v8u16 out0, out1, out2, out3;
                        v16i8 mult0 = { 31, 1, 30, 2, 29, 3, 28, 4,
                                        27, 5, 26, 6, 25, 7, 24, 8 };
                        v16i8 offset = { -8, 8, -8, 8, -8, 8, -8, 8,
                                         -8, 8, -8, 8, -8, 8, -8, 8 };
                        v16i8 mult1, mult2, mult3;

                        mult1 = mult0 + offset;
                        mult2 = mult1 + offset;
                        mult3 = mult2 + offset;

                        DOTP_UB4_UH(src0, src0, src0, src0, mult0, mult1,
                                    mult2, mult3, out0, out1, out2, out3);
                        SRARI_H4_UH(out0, out1, out2, out3, 6);
                        PCKEV_B2_SB(out1, out0, out3, out2, op0, op1);
                        ST_SB2(op0, op1, au1_flt + 1, 16);

                        DOTP_UB4_UH(src1, src1, src1, src1,
                                    mult0, mult1, mult2, mult3,
                                    out0, out1, out2, out3);
                        SRARI_H4_UH(out0, out1, out2, out3, 6);
                        PCKEV_B2_SB(out1, out0, out3, out2, op0, op1);
                        ST_SB2(op0, op1, au1_flt + 1 + 2 * size, 16);
                    }
                    break;
                case 32:
                    {
                        v8u16 out0, out1, out2, out3, out4, out5, out6, out7;
                        v16i8 op2, op3;
                        v16i8 mult1, mult2, mult3, mult4, mult5, mult6, mult7;
                        v16i8 mult0 = { 63, 1, 62, 2, 61, 3, 60, 4,
                                        59, 5, 58, 6, 57, 7, 56, 8 };
                        v16i8 offset = { -8, 8, -8, 8, -8, 8, -8, 8,
                                         -8, 8, -8, 8, -8, 8, -8, 8 };

                        mult1 = mult0 + offset;
                        mult2 = mult1 + offset;
                        mult3 = mult2 + offset;
                        mult4 = mult3 + offset;
                        mult5 = mult4 + offset;
                        mult6 = mult5 + offset;
                        mult7 = mult6 + offset;

                        DOTP_UB4_UH(src0, src0, src0, src0, mult0, mult1,
                                    mult2, mult3, out0, out1, out2, out3);
                        DOTP_UB4_UH(src0, src0, src0, src0, mult4, mult5,
                                    mult6, mult7, out4, out5, out6, out7);
                        SRARI_H4_UH(out0, out1, out2, out3, 6);
                        SRARI_H4_UH(out4, out5, out6, out7, 6);
                        PCKEV_B2_SB(out1, out0, out3, out2, op0, op1);
                        PCKEV_B2_SB(out5, out4, out7, out6, op2, op3);
                        ST_SB4(op0, op1, op2, op3, au1_flt + 1, 16);

                        DOTP_UB4_UH(src1, src1, src1, src1, mult0, mult1,
                                    mult2, mult3, out0, out1, out2, out3);
                        DOTP_UB4_UH(src1, src1, src1, src1, mult4, mult5,
                                    mult6, mult7, out4, out5, out6, out7);
                        SRARI_H4_UH(out0, out1, out2, out3, 6);
                        SRARI_H4_UH(out4, out5, out6, out7, 6);
                        PCKEV_B2_SB(out1, out0, out3, out2, op0, op1);
                        PCKEV_B2_SB(out5, out4, out7, out6, op2, op3);
                        ST_SB4(op0, op1, op2, op3, au1_flt + 1 + 2 * size, 16);
                    }
            }

            au1_flt[2 * size] = src[2 * size];
        }
        else
        {
            switch(size)
            {
                case 4:
                    {
                        v8u16 out0, out1;
                        v16i8 src0, src1;
                        v16i8 vec0_m, vec1_m;
                        v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4,
                                        4, 5, 5, 6, 6, 7, 7, 8 };
                        v16i8 filt = { 1, 1, 1, 1, 1, 1, 1, 1,
                                       1, 1, 1, 1, 1, 1, 1, 1 };
                        v16i8 mask1 = ADDVI_B(mask0, 1);

                        LD_SB2(src, 8, src0, src1);

                        VSHF_B2_SB(src0, src0, src1, src1, mask0, mask0,
                                   vec0_m, vec1_m);
                        DOTP_UB2_UH(vec0_m, vec1_m, filt, filt, out0, out1);
                        VSHF_B2_SB(src0, src0, src1, src1, mask1, mask1,
                                   vec0_m, vec1_m);
                        DPADD_UB2_UH(vec0_m, vec1_m, filt, filt, out0, out1);
                        SRARI_H2_UH(out0, out1, 2);
                        src0 = __msa_pckev_b((v16i8) out1, (v16i8) out0);
                        ST_SB(src0, au1_flt + 1);
                    }
                    break;
                case 8:
                    {
                        v8u16 out0, out1, out2, out3;
                        v16i8 src0, src1, src2, src3;
                        v16i8 vec0_m, vec1_m, vec2_m, vec3_m;
                        v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4,
                                        4, 5, 5, 6, 6, 7, 7, 8 };
                        v16i8 filt = { 1, 1, 1, 1, 1, 1, 1, 1,
                                       1, 1, 1, 1, 1, 1, 1, 1 };
                        v16i8 mask1, mask2, mask3;

                        mask1 = ADDVI_B(mask0, 1);
                        mask2 = ADDVI_B(mask0, 8);
                        mask3 = ADDVI_B(mask1, 8);

                        LD_SB3(src, 16, src0, src2, src3);

                        VSHF_B2_SB(src0, src0, src0, src2, mask0, mask2,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src3, mask0, mask2,
                                   vec2_m, vec3_m);
                        DOTP_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                    filt, filt, out0, out1, out2, out3);
                        VSHF_B2_SB(src0, src0, src0, src2, mask1, mask3,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src3, mask1, mask3,
                                   vec2_m, vec3_m);
                        DPADD_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                     filt, filt, out0, out1, out2, out3);

                        SRARI_H4_UH(out0, out1, out2, out3, 2);
                        PCKEV_B2_SB(out1, out0, out3, out2, src0, src1);
                        ST_SB2(src0, src1, au1_flt + 1, 16);
                    }
                    break;
                case 16:
                    {
                        v8u16 out0, out1, out2, out3, out4, out5, out6, out7;
                        v16i8 src0, src2, src4, src6, src7, mask1, mask2, mask3;
                        v16i8 vec0_m, vec1_m, vec2_m, vec3_m;
                        v16i8 vec4_m, vec5_m, vec6_m, vec7_m;
                        v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4,
                                        4, 5, 5, 6, 6, 7, 7, 8 };
                        v16i8 filt = { 1, 1, 1, 1, 1, 1, 1, 1,
                                       1, 1, 1, 1, 1, 1, 1, 1 };

                        mask1 = ADDVI_B(mask0, 1);
                        mask2 = ADDVI_B(mask0, 8);
                        mask3 = ADDVI_B(mask1, 8);

                        LD_SB5(src, 16, src0, src2, src4, src6, src7);

                        VSHF_B2_SB(src0, src0, src0, src2, mask0, mask2,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src4, mask0, mask2,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src4, src4, src4, src6, mask0, mask2,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src6, src6, src6, src7, mask0, mask2,
                                   vec6_m, vec7_m);

                        DOTP_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                    filt, filt, out0, out1, out2, out3);
                        DOTP_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                    filt, filt, out4, out5, out6, out7);

                        VSHF_B2_SB(src0, src0, src0, src2, mask1, mask3,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src4, mask1, mask3,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src4, src4, src4, src6, mask1, mask3,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src6, src6, src6, src7, mask1, mask3,
                                   vec6_m, vec7_m);
                        DPADD_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                     filt, filt, out0, out1, out2, out3);
                        DPADD_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                     filt, filt, out4, out5, out6, out7);

                        SRARI_H4_UH(out0, out1, out2, out3, 2);
                        SRARI_H4_UH(out4, out5, out6, out7, 2);
                        PCKEV_B4_SB(out1, out0, out3, out2, out5, out4, out7,
                                    out6, src0, src2, src4, src6);
                        ST_SB4(src0, src2, src4, src6, au1_flt + 1, 16);
                    }
                    break;
                case 32:
                    {
                        v8u16 out0, out1, out2, out3, out4, out5, out6, out7;
                        v16i8 src0, src2, src4, src6, src8, src10, src12, src14;
                        v16i8 src15, vec0_m, vec1_m, vec2_m, vec3_m, vec4_m;
                        v16i8 vec5_m, vec6_m, vec7_m, mask1, mask2, mask3;
                        v16i8 mask0 = { 0, 1, 1, 2, 2, 3, 3, 4,
                                        4, 5, 5, 6, 6, 7, 7, 8 };
                        v16i8 filt = { 1, 1, 1, 1, 1, 1, 1, 1,
                                       1, 1, 1, 1, 1, 1, 1, 1 };

                        mask1 = ADDVI_B(mask0, 1);
                        mask2 = ADDVI_B(mask0, 8);
                        mask3 = ADDVI_B(mask1, 8);

                        LD_SB5(src, 16, src0, src2, src4, src6, src8);
                        LD_SB4(src + 80, 16, src10, src12, src14, src15);

                        VSHF_B2_SB(src0, src0, src0, src2, mask0, mask2,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src4, mask0, mask2,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src4, src4, src4, src6, mask0, mask2,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src6, src6, src6, src8, mask0, mask2,
                                   vec6_m, vec7_m);

                        DOTP_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                    filt, filt, out0, out1, out2, out3);
                        DOTP_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                    filt, filt, out4, out5, out6, out7);

                        VSHF_B2_SB(src0, src0, src0, src2, mask1, mask3,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src2, src2, src2, src4, mask1, mask3,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src4, src4, src4, src6, mask1, mask3,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src6, src6, src6, src8, mask1, mask3,
                                   vec6_m, vec7_m);
                        DPADD_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                     filt, filt, out0, out1, out2, out3);
                        DPADD_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                     filt, filt, out4, out5, out6, out7);

                        SRARI_H4_UH(out0, out1, out2, out3, 2);
                        SRARI_H4_UH(out4, out5, out6, out7, 2);
                        PCKEV_B4_SB(out1, out0, out3, out2, out5, out4, out7,
                                    out6, src0, src2, src4, src6);
                        ST_SB4(src0, src2, src4, src6, au1_flt + 1, 16);

                        VSHF_B2_SB(src8, src8, src8, src10, mask0, mask2,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src10, src10, src10, src12, mask0, mask2,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src12, src12, src12, src14, mask0, mask2,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src14, src14, src14, src15, mask0, mask2,
                                   vec6_m, vec7_m);

                        DOTP_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                    filt, filt, out0, out1, out2, out3);
                        DOTP_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                    filt, filt, out4, out5, out6, out7);

                        VSHF_B2_SB(src8, src8, src8, src10, mask1, mask3,
                                   vec0_m, vec1_m);
                        VSHF_B2_SB(src10, src10, src10, src12, mask1, mask3,
                                   vec2_m, vec3_m);
                        VSHF_B2_SB(src12, src12, src12, src14, mask1, mask3,
                                   vec4_m, vec5_m);
                        VSHF_B2_SB(src14, src14, src14, src15, mask1, mask3,
                                   vec6_m, vec7_m);
                        DPADD_UB4_UH(vec0_m, vec1_m, vec2_m, vec3_m, filt, filt,
                                     filt, filt, out0, out1, out2, out3);
                        DPADD_UB4_UH(vec4_m, vec5_m, vec6_m, vec7_m, filt, filt,
                                     filt, filt, out4, out5, out6, out7);

                        SRARI_H4_UH(out0, out1, out2, out3, 2);
                        SRARI_H4_UH(out4, out5, out6, out7, 2);
                        PCKEV_B4_SB(out1, out0, out3, out2, out5, out4, out7,
                                    out6, src0, src2, src4, src6);
                        ST_SB4(src0, src2, src4, src6, au1_flt + 1 + 64, 16);
                    }
            }
        }

        au1_flt[4 * size] = src[4 * size];
        COPY(au1_flt, dst, size, four_nt);
    }
}

void ihevc_intra_pred_luma_planar_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                      UWORD8 *pu1_dst, WORD32 dst_strd,
                                      WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UWORD8 left_array[32 + 1];
    UNUSED(mode);
    UNUSED(src_strd);

    for(row = 0; row < (nt + 1); row++)
    {
        left_array[row] = pu1_ref[2 * nt - 1 - row];
    }

    switch(nt)
    {
        case 4:
            hevc_intra_pred_plane_4x4_msa(pu1_ref + 2 * nt + 1, left_array,
                                          pu1_dst, dst_strd);
            break;
        case 8:
            hevc_intra_pred_plane_8x8_msa(pu1_ref + 2 * nt + 1, left_array,
                                          pu1_dst, dst_strd);
            break;
        case 16:
            hevc_intra_pred_plane_16x16_msa(pu1_ref + 2 * nt + 1, left_array,
                                            pu1_dst, dst_strd);
            break;
        case 32:
            hevc_intra_pred_plane_32x32_msa(pu1_ref + 2 * nt + 1, left_array,
                                            pu1_dst, dst_strd);
    }
}

void ihevc_intra_pred_luma_dc_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                  UWORD8 *pu1_dst, WORD32 dst_strd,
                                  WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UNUSED(mode);
    UNUSED(src_strd);

    if(32 == nt)
    {
        intra_predict_dc_32x32_msa(pu1_ref + nt, pu1_ref + 2 * nt + 1,
                                   pu1_dst, dst_strd);
    }
    else
    {
        UWORD8 left_array[16];

        for(row = 0; row < nt; row++)
        {
            left_array[row] = pu1_ref[2 * nt - 1 - row];
        }

        switch(nt)
        {
            case 4:
                intra_predict_dc_4x4_msa(pu1_ref + nt, pu1_ref + 2 * nt + 1,
                                         pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_dc_4x4_msa(pu1_ref + 2 * nt + 1,
                                                     left_array, pu1_dst,
                                                     dst_strd, pu1_dst[0]);
                break;
            case 8:
                intra_predict_dc_8x8_msa(pu1_ref + nt, pu1_ref + 2 * nt + 1,
                                         pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_dc_8x8_msa(pu1_ref + 2 * nt + 1,
                                                     left_array, pu1_dst,
                                                     dst_strd, pu1_dst[0]);
                break;
            case 16:
                intra_predict_dc_16x16_msa(pu1_ref + nt, pu1_ref + 2 * nt + 1,
                                           pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_dc_16x16_msa(pu1_ref + 2 * nt + 1,
                                                       left_array, pu1_dst,
                                                       dst_strd, pu1_dst[0]);
        }
    }
}

void ihevc_intra_pred_luma_horz_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                    UWORD8 *pu1_dst, WORD32 dst_strd,
                                    WORD32 nt, WORD32 mode)
{
    UNUSED(mode);
    UNUSED(src_strd);

    if(32 == nt)
    {
        intra_predict_horiz_32x32_msa(pu1_ref + 2 * nt - 1 - (nt - 1),
                                      pu1_dst + (nt - 1) * dst_strd, -dst_strd);
    }
    else
    {
        switch(nt)
        {
            case 4:
                intra_predict_horiz_4x4_msa(pu1_ref + 2 * nt - 1 - (nt - 1),
                                            pu1_dst + (nt - 1) * dst_strd,
                                            -dst_strd);
                hevc_post_intra_pred_filt_horiz_4_msa(pu1_ref + 2 * nt + 1,
                                                      pu1_ref + 2 * nt - 1,
                                                      pu1_dst);
                break;
            case 8:
                intra_predict_horiz_8x8_msa(pu1_ref + 2 * nt - 1 - (nt - 1),
                                            pu1_dst + (nt - 1) * dst_strd,
                                            -dst_strd);
                hevc_post_intra_pred_filt_horiz_8_msa(pu1_ref + 2 * nt + 1,
                                                      pu1_ref + 2 * nt - 1,
                                                      pu1_dst);
                break;
            case 16:
                intra_predict_horiz_16x16_msa(pu1_ref + 2 * nt - 1 - (nt - 1),
                                              pu1_dst + (nt - 1) * dst_strd,
                                              -dst_strd);
                hevc_post_intra_pred_filt_horiz_16_msa(pu1_ref + 2 * nt + 1,
                                                       pu1_ref + 2 * nt - 1,
                                                       pu1_dst);
        }
    }
}

void ihevc_intra_pred_luma_ver_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                   UWORD8 *pu1_dst, WORD32 dst_strd,
                                   WORD32 nt, WORD32 mode)
{
    UWORD8 *src_top, *src_left, *dst;
    UNUSED(mode);
    UNUSED(src_strd);

    if(32 == nt)
    {
        intra_predict_vert_32x32_msa(pu1_ref + 2 * nt + 1, pu1_dst, dst_strd);
    }
    else
    {
        src_top = pu1_ref + 2 * nt + 1;
        src_left = pu1_ref + 2 * nt - 1 - (nt - 1);
        dst = pu1_dst + (nt - 1) * dst_strd;

        switch(nt)
        {
            case 4:
                intra_predict_vert_4x4_msa(src_top, pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_vert_4_msa(src_top, src_left,
                                                     dst, -dst_strd);
                break;
            case 8:
                intra_predict_vert_8x8_msa(src_top, pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_vert_8_msa(src_top, src_left,
                                                     dst, -dst_strd);
                break;
            case 16:
                intra_predict_vert_16x16_msa(src_top, pu1_dst, dst_strd);
                hevc_post_intra_pred_filt_vert_16_msa(src_top, src_left,
                                                      dst, -dst_strd);
        }
    }
}

void ihevc_intra_pred_luma_mode2_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                     UWORD8 *pu1_dst, WORD32 dst_strd,
                                     WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UWORD8 left[32 * 2];
    UNUSED(mode);
    UNUSED(src_strd);

    for(row = 0; row < 2 * nt; row++)
    {
        left[row] = pu1_ref[2 * nt - 1 - row];
    }

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode2_4width_msa(left, pu1_dst, dst_strd);
            break;
        case 8:
            hevc_intra_pred_mode2_8width_msa(left, pu1_dst, dst_strd);
            break;
        case 16:
            hevc_intra_pred_mode2_16width_msa(left, pu1_dst, dst_strd);
            break;
        case 32:
            hevc_intra_pred_mode2_32width_msa(left, pu1_dst, dst_strd);
    }
}

void ihevc_intra_pred_luma_mode_18_34_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                          UWORD8 *pu1_dst, WORD32 dst_strd,
                                          WORD32 nt, WORD32 mode)
{
    UWORD8 *src_top;
    WORD32 row;
    UWORD8 left[32 * 2];
    UNUSED(src_strd);

    for(row = 0; row < (2 * nt); row++)
    {
        left[row] = pu1_ref[2 * nt - 1 - row];
    }

    src_top = pu1_ref + 2 * nt + 1;

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode18_34_4width_msa(src_top, left, pu1_dst,
                                                 dst_strd, mode);
            break;
        case 8:
            hevc_intra_pred_mode18_34_8width_msa(src_top, left, pu1_dst,
                                                 dst_strd, mode);
            break;
        case 16:
            hevc_intra_pred_mode18_34_16width_msa(src_top, left, pu1_dst,
                                                  dst_strd, mode);
            break;
        case 32:
            hevc_intra_pred_mode18_34_32width_msa(src_top, left, pu1_dst,
                                                  dst_strd, mode);
    }
}

void ihevc_intra_pred_luma_mode_3_to_9_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                           UWORD8 *pu1_dst, WORD32 dst_strd,
                                           WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UWORD8 left[32 * 2];
    UNUSED(src_strd);

    for(row = 0; row < 2 * nt; row++)
    {
        left[row] = pu1_ref[2 * nt - 1 - row];
    }

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode_3_to_9_4width_msa(left, pu1_dst, dst_strd,
                                                   mode);
            break;
        case 8:
            hevc_intra_pred_mode_3_to_9_8width_msa(left, pu1_dst, dst_strd,
                                                   mode);
            break;
        case 16:
            hevc_intra_pred_mode_3_to_9_16width_msa(left, pu1_dst, dst_strd,
                                                    mode);
            break;
        case 32:
            hevc_intra_pred_mode_3_to_9_32width_msa(left, pu1_dst, dst_strd,
                                                    mode);
    }
}


void ihevc_intra_pred_luma_mode_11_to_17_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                             UWORD8 *pu1_dst, WORD32 dst_strd,
                                             WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UWORD8 left[32 * 2];
    UNUSED(src_strd);

    for(row = 0; row < (2 * nt); row++)
    {
        left[row] = pu1_ref[2 * nt - row - 1];
    }

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode_11_to_17_4width_msa(pu1_ref + 2 * nt,
                                                     left, pu1_dst,
                                                     dst_strd, mode);
            break;
        case 8:
            hevc_intra_pred_mode_11_to_17_8width_msa(pu1_ref + 2 * nt,
                                                     left, pu1_dst,
                                                     dst_strd, mode);
            break;
        case 16:
            hevc_intra_pred_mode_11_to_17_16width_msa(pu1_ref + 2 * nt,
                                                      left, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 32:
            hevc_intra_pred_mode_11_to_17_32width_msa(pu1_ref + 2 * nt,
                                                      left, pu1_dst,
                                                      dst_strd, mode);
    }
}

void ihevc_intra_pred_luma_mode_19_to_25_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                             UWORD8 *pu1_dst, WORD32 dst_strd,
                                             WORD32 nt, WORD32 mode)
{
    WORD32 row;
    UWORD8 left[32 * 2];
    UNUSED(src_strd);

    for(row = 0; row < 2 * nt; row++)
    {
        left[row] = pu1_ref[2 * nt - row - 1];
    }

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode_19_to_25_4width_msa(pu1_ref + 2 * nt, left,
                                                     pu1_dst, dst_strd, mode);
            break;
        case 8:
            hevc_intra_pred_mode_19_to_25_8width_msa(pu1_ref + 2 * nt, left,
                                                     pu1_dst, dst_strd, mode);
            break;
        case 16:
            hevc_intra_pred_mode_19_to_25_16width_msa(pu1_ref + 2 * nt, left,
                                                      pu1_dst, dst_strd, mode);
            break;
        case 32:
            hevc_intra_pred_mode_19_to_25_32width_msa(pu1_ref + 2 * nt, left,
                                                      pu1_dst, dst_strd, mode);
    }
}

void ihevc_intra_pred_luma_mode_27_to_33_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                             UWORD8 *pu1_dst, WORD32 dst_strd,
                                             WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_intra_pred_mode_27_to_33_4width_msa(pu1_ref + 2 * nt + 1,
                                                     pu1_dst, dst_strd, mode);
            break;
        case 8:
            hevc_intra_pred_mode_27_to_33_8width_msa(pu1_ref + 2 * nt + 1,
                                                     pu1_dst, dst_strd, mode);
            break;
        case 16:
            hevc_intra_pred_mode_27_to_33_16width_msa(pu1_ref + 2 * nt + 1,
                                                      pu1_dst, dst_strd, mode);
            break;
        case 32:
            hevc_intra_pred_mode_27_to_33_32width_msa(pu1_ref + 2 * nt + 1,
                                                      pu1_dst, dst_strd, mode);
    }
}

void ihevc_intra_pred_ref_filtering_msa(UWORD8 *pu1_src, WORD32 nt,
                                        UWORD8 *pu1_dst, WORD32 mode,
                                        WORD32 strng_intra_smoothing_enable_flag)
{
    hevc_intra_pred_ref_filtering_msa(pu1_src, pu1_dst, nt, mode,
                                      strng_intra_smoothing_enable_flag);
}

void ihevc_intra_pred_chroma_planar_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                        UWORD8 *pu1_dst, WORD32 dst_strd,
                                        WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);
    UNUSED(mode);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_plane_4x4_msa(pu1_ref, pu1_dst, dst_strd);
            break;
        case 8:
            hevc_ilv_intra_pred_plane_8x8_msa(pu1_ref, pu1_dst, dst_strd);
            break;
        case 16:
            hevc_ilv_intra_pred_plane_16x16_msa(pu1_ref, pu1_dst, dst_strd);
    }
}

void ihevc_intra_pred_chroma_dc_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                    UWORD8 *pu1_dst, WORD32 dst_strd,
                                    WORD32 nt, WORD32 mode)
{
    UWORD8 *pu1_ref_1 = pu1_ref + 4 * nt;
    UNUSED(mode);
    UNUSED(src_strd);

    asm volatile (
    #if (__mips == 64)
        "daddiu  %[pu1_ref_1],  %[pu1_ref_1], 2  \n\t"
    #else
        "addiu   %[pu1_ref_1],  %[pu1_ref_1], 2  \n\t"
    #endif

        : [pu1_ref_1] "+r" (pu1_ref_1)
        :
    );

    switch(nt)
    {
        case 4:
            intra_ilv_predict_dc_8x4_msa(pu1_ref_1, pu1_ref + 2 * nt, pu1_dst,
                                         dst_strd);
            break;
        case 8:
            intra_ilv_predict_dc_16x8_msa(pu1_ref_1, pu1_ref + 2 * nt, pu1_dst,
                                          dst_strd);
            break;
        case 16:
            intra_ilv_predict_dc_32x16_msa(pu1_ref_1, pu1_ref + 2 * nt, pu1_dst,
                                           dst_strd);
    }
}

void ihevc_intra_pred_chroma_horz_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                      UWORD8 *pu1_dst, WORD32 dst_strd,
                                      WORD32 nt, WORD32 mode)
{
    UNUSED(mode);
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            intra_ilv_predict_horiz_8x4_msa(pu1_ref + 2 * nt,
                                            pu1_dst + (nt - 1) * dst_strd,
                                            -dst_strd);
            break;
        case 8:
            intra_ilv_predict_horiz_16x8_msa(pu1_ref + 2 * nt,
                                             pu1_dst + (nt - 1) * dst_strd,
                                             -dst_strd);
            break;
        case 16:
            intra_ilv_predict_horiz_32x16_msa(pu1_ref + 2 * nt,
                                              pu1_dst + (nt - 1) * dst_strd,
                                              -dst_strd);
    }
}

void ihevc_intra_pred_chroma_ver_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                     UWORD8 *pu1_dst, WORD32 dst_strd,
                                     WORD32 nt, WORD32 mode)
{
    UWORD8 *pu1_ref_1 = pu1_ref + 4 * nt + 2;
    UNUSED(mode);
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            intra_predict_vert_8x4_msa(pu1_ref_1, pu1_dst, dst_strd);
            break;
        case 8:
            intra_predict_vert_16x8_msa(pu1_ref_1, pu1_dst, dst_strd);
            break;
        case 16:
            intra_predict_vert_32x16_msa(pu1_ref_1, pu1_dst, dst_strd);
    }
}

void ihevc_intra_pred_chroma_mode2_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                       UWORD8 *pu1_dst, WORD32 dst_strd,
                                       WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);
    UNUSED(mode);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode2_4x4_msa(pu1_ref, pu1_dst, dst_strd);
            break;
        case 8:
            hevc_ilv_intra_pred_mode2_8x8_msa(pu1_ref, pu1_dst, dst_strd);
            break;
        case 16:
            hevc_ilv_intra_pred_mode2_16x16_msa(pu1_ref, pu1_dst, dst_strd);
    }
}

void ihevc_intra_pred_chroma_mode_18_34_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                            UWORD8 *pu1_dst, WORD32 dst_strd,
                                            WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode_18_34_4x4_msa(pu1_ref, pu1_dst, dst_strd,
                                                   mode);
            break;
        case 8:
            hevc_ilv_intra_pred_mode_18_34_8x8_msa(pu1_ref, pu1_dst, dst_strd,
                                                   mode);
            break;
        case 16:
            hevc_ilv_intra_pred_mode_18_34_16x16_msa(pu1_ref, pu1_dst,
                                                     dst_strd, mode);
    }
}

void ihevc_intra_pred_chroma_mode_3_to_9_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                             UWORD8 *pu1_dst, WORD32 dst_strd,
                                             WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode_3_to_9_4x4_msa(pu1_ref, pu1_dst,
                                                    dst_strd, mode);
            break;
        case 8:
            hevc_ilv_intra_pred_mode_3_to_9_8x8_msa(pu1_ref, pu1_dst,
                                                    dst_strd, mode);
            break;
        case 16:
            hevc_ilv_intra_pred_mode_3_to_9_16x16_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
    }
}

void ihevc_intra_pred_chroma_mode_11_to_17_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                               UWORD8 *pu1_dst, WORD32 dst_strd,
                                               WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode_11_to_17_4x4_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 8:
            hevc_ilv_intra_pred_mode_11_to_17_8x8_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 16:
            hevc_ilv_intra_pred_mode_11_to_17_16x16_msa(pu1_ref, pu1_dst,
                                                        dst_strd, mode);
    }

}

void ihevc_intra_pred_chroma_mode_19_to_25_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                               UWORD8 *pu1_dst, WORD32 dst_strd,
                                               WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode_19_to_25_4x4_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 8:
            hevc_ilv_intra_pred_mode_19_to_25_8x8_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 16:
            hevc_ilv_intra_pred_mode_19_to_25_16x16_msa(pu1_ref, pu1_dst,
                                                        dst_strd, mode);
    }

}

void ihevc_intra_pred_chroma_mode_27_to_33_msa(UWORD8 *pu1_ref, WORD32 src_strd,
                                               UWORD8 *pu1_dst, WORD32 dst_strd,
                                               WORD32 nt, WORD32 mode)
{
    UNUSED(src_strd);

    switch(nt)
    {
        case 4:
            hevc_ilv_intra_pred_mode_27_to_33_4x4_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 8:
            hevc_ilv_intra_pred_mode_27_to_33_8x8_msa(pu1_ref, pu1_dst,
                                                      dst_strd, mode);
            break;
        case 16:
            hevc_ilv_intra_pred_mode_27_to_33_16x16_msa(pu1_ref, pu1_dst,
                                                        dst_strd, mode);
    }
}
