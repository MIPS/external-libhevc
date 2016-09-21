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

#ifndef __IHEVC_MACROS_MSA_H__
#define __IHEVC_MACROS_MSA_H__

#include <stdint.h>

#if defined(__clang__)
    #define CLANG_BUILD
#endif

#ifdef CLANG_BUILD
    typedef signed char v16i8 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned char v16u8 __attribute__((vector_size(16), aligned(16)));
    typedef short v8i16 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned short v8u16 __attribute__((vector_size(16), aligned(16)));
    typedef int v4i32 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned int v4u32 __attribute__((vector_size(16), aligned(16)));
    typedef long long v2i64 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned long long v2u64 __attribute__((vector_size(16), aligned(16)));

    #define __msa_slli_h    __builtin_msa_slli_h
    #define __msa_slli_w    __builtin_msa_slli_w
    #define __msa_srai_h    __builtin_msa_srai_h
    #define __msa_srai_w    __builtin_msa_srai_w
    #define __msa_srar_w    __builtin_msa_srar_w
    #define __msa_srari_h   __builtin_msa_srari_h
    #define __msa_srari_w   __builtin_msa_srari_w
    #define __msa_srari_d   __builtin_msa_srari_d
    #define __msa_srli_b    __builtin_msa_srli_b
    #define __msa_addvi_b   __builtin_msa_addvi_b
    #define __msa_addvi_h   __builtin_msa_addvi_h
    #define __msa_max_s_h   __builtin_msa_max_s_h
    #define __msa_maxi_s_h  __builtin_msa_maxi_s_h
    #define __msa_maxi_s_w  __builtin_msa_maxi_s_w
    #define __msa_min_s_h   __builtin_msa_min_s_h
    #define __msa_min_s_w   __builtin_msa_min_s_w
    #define __msa_ceqi_b    __builtin_msa_ceqi_b
    #define __msa_ceqi_d    __builtin_msa_ceqi_d
    #define __msa_clti_s_b  __builtin_msa_clti_s_b
    #define __msa_clti_s_h  __builtin_msa_clti_s_h
    #define __msa_sat_s_h   __builtin_msa_sat_s_h
    #define __msa_sat_s_w   __builtin_msa_sat_s_w
    #define __msa_add_a_h   __builtin_msa_add_a_h
    #define __msa_adds_s_h  __builtin_msa_adds_s_h
    #define __msa_aver_u_h  __builtin_msa_aver_u_h
    #define __msa_subs_u_h  __builtin_msa_subs_u_h
    #define __msa_hadd_u_h  __builtin_msa_hadd_u_h
    #define __msa_hadd_u_w  __builtin_msa_hadd_u_w
    #define __msa_hadd_u_d  __builtin_msa_hadd_u_d
    #define __msa_dotp_s_h  __builtin_msa_dotp_s_h
    #define __msa_dotp_s_w  __builtin_msa_dotp_s_w
    #define __msa_dotp_u_h  __builtin_msa_dotp_u_h
    #define __msa_dpadd_s_h __builtin_msa_dpadd_s_h
    #define __msa_dpadd_s_w __builtin_msa_dpadd_s_w
    #define __msa_dpadd_u_h __builtin_msa_dpadd_u_h
    #define __msa_sld_b     __builtin_msa_sld_b
    #define __msa_sldi_b    __builtin_msa_sldi_b
    #define __msa_splati_b  __builtin_msa_splati_b
    #define __msa_splati_h  __builtin_msa_splati_h
    #define __msa_splati_w  __builtin_msa_splati_w
    #define __msa_pckev_b   __builtin_msa_pckev_b
    #define __msa_pckev_h   __builtin_msa_pckev_h
    #define __msa_pckev_w   __builtin_msa_pckev_w
    #define __msa_pckev_d   __builtin_msa_pckev_d
    #define __msa_pckod_b   __builtin_msa_pckod_b
    #define __msa_pckod_d   __builtin_msa_pckod_d
    #define __msa_ilvl_b    __builtin_msa_ilvl_b
    #define __msa_ilvl_h    __builtin_msa_ilvl_h
    #define __msa_ilvl_w    __builtin_msa_ilvl_w
    #define __msa_ilvl_d    __builtin_msa_ilvl_d
    #define __msa_ilvr_b    __builtin_msa_ilvr_b
    #define __msa_ilvr_h    __builtin_msa_ilvr_h
    #define __msa_ilvr_w    __builtin_msa_ilvr_w
    #define __msa_ilvr_d    __builtin_msa_ilvr_d
    #define __msa_ilvev_b   __builtin_msa_ilvev_b
    #define __msa_ilvev_w   __builtin_msa_ilvev_w
    #define __msa_ilvev_d   __builtin_msa_ilvev_d
    #define __msa_ilvod_w   __builtin_msa_ilvod_w
    #define __msa_vshf_b    __builtin_msa_vshf_b
    #define __msa_vshf_h    __builtin_msa_vshf_h
    #define __msa_vshf_w    __builtin_msa_vshf_w
    #define __msa_andi_b    __builtin_msa_andi_b
    #define __msa_nor_v     __builtin_msa_nor_v
    #define __msa_xori_b    __builtin_msa_xori_b
    #define __msa_bmnz_v    __builtin_msa_bmnz_v
    #define __msa_bmz_v     __builtin_msa_bmz_v
    #define __msa_fill_b    __builtin_msa_fill_b
    #define __msa_fill_h    __builtin_msa_fill_h
    #define __msa_fill_w    __builtin_msa_fill_w
    #define __msa_fill_d    __builtin_msa_fill_d
    #define __msa_copy_s_w  __builtin_msa_copy_s_w
    #define __msa_copy_s_d  __builtin_msa_copy_s_d
    #define __msa_copy_u_b  __builtin_msa_copy_u_b
    #define __msa_copy_u_h  __builtin_msa_copy_u_h
    #define __msa_copy_u_w  __builtin_msa_copy_u_w
    #define __msa_copy_u_d  __builtin_msa_copy_u_d
    #define __msa_insert_b  __builtin_msa_insert_b
    #define __msa_insert_h  __builtin_msa_insert_h
    #define __msa_insert_w  __builtin_msa_insert_w
    #define __msa_insert_d  __builtin_msa_insert_d
    #define __msa_ldi_b     __builtin_msa_ldi_b
    #define __msa_ldi_h     __builtin_msa_ldi_h
    #define __msa_ldi_w     __builtin_msa_ldi_w

    #define ADDVI_B(a, b)  __msa_addvi_b((v16i8) a, b)
    #define ADDVI_H(a, b)  __msa_addvi_h((v8i16) a, b)
    #define SLLI_H(a, b)   __msa_slli_h((v8i16) a, b)
    #define SLLI_W(a, b)   __msa_slli_w((v4i32) a, b)
    #define SRLI_B(a, b)   __msa_srli_b((v16i8) a, b)
    #define SRAI_H(a, b)   __msa_srai_h((v8i16) a, b)
    #define SRAI_W(a, b)   __msa_srai_w((v4i32) a, b)
    #define CEQI_B(a, b)   __msa_ceqi_b((v16i8) a, b)
    #define CEQI_D(a, b)   __msa_ceqi_d((v2i64) a, b)
    #define CLTI_S_B(a, b) __msa_clti_s_b((v16i8) a, b)
    #define CLTI_S_H(a, b) __msa_clti_s_h((v8i16) a, b)
    #define XORI_B(a, b)   __msa_xori_b((v16u8) a, b)
    #define ANDI_B(a, b)   __msa_andi_b((v16u8) a, b)
#else
    #include <msa.h>

    #define ADDVI_B(a, b)  ((v16i8) a + b)
    #define ADDVI_H(a, b)  ((v8i16) a + b)
    #define SLLI_H(a, b)   ((v8i16) a << b)
    #define SLLI_W(a, b)   ((v4i32) a << b)
    #define SRLI_B(a, b)   ((v16u8) a >> b)
    #define SRAI_H(a, b)   ((v8i16) a >> b)
    #define SRAI_W(a, b)   ((v4i32) a >> b)
    #define CEQI_B(a, b)   ((v16i8) a == b)
    #define CEQI_D(a, b)   ((v2i64) a == b)
    #define CLTI_S_B(a, b) ((v16i8) a < b)
    #define CLTI_S_H(a, b) ((v8i16) a < b)
    #define XORI_B(a, b)   ((v16u8) a ^ b)
    #define ANDI_B(a, b)   ((v16u8) a & b)
#endif

#define LD_V(RTYPE, psrc) *((RTYPE *)(psrc))
#define LD_UB(...) LD_V(v16u8, __VA_ARGS__)
#define LD_SB(...) LD_V(v16i8, __VA_ARGS__)
#define LD_UH(...) LD_V(v8u16, __VA_ARGS__)
#define LD_SH(...) LD_V(v8i16, __VA_ARGS__)

#define ST_V(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)
#define ST_UB(...) ST_V(v16u8, __VA_ARGS__)
#define ST_SB(...) ST_V(v16i8, __VA_ARGS__)
#define ST_SH(...) ST_V(v8i16, __VA_ARGS__)

#ifdef CLANG_BUILD
    #define LH(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lh_m = (UWORD8 *) (psrc);   \
        UWORD16 val_m;                           \
                                                 \
        asm volatile (                           \
            "lh  %[val_m],  %[psrc_lh_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lh_m] "m" (*psrc_lh_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #define LW(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lw_m = (UWORD8 *) (psrc);   \
        UWORD32 val_m;                           \
                                                 \
        asm volatile (                           \
            "lw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lw_m] "m" (*psrc_lw_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #if (__mips == 64)
        #define LD(psrc)                             \
        ( {                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);   \
            ULWORD64 val_m = 0;                      \
                                                     \
            asm volatile (                           \
                "ld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                     \
                : [val_m] "=r" (val_m)               \
                : [psrc_ld_m] "m" (*psrc_ld_m)       \
            );                                       \
                                                     \
            val_m;                                   \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                              \
        ( {                                                           \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);                    \
            UWORD32 val0_m, val1_m;                                   \
            ULWORD64 val_m = 0;                                       \
                                                                      \
            val0_m = LW(psrc_ld_m);                                   \
            val1_m = LW(psrc_ld_m + 4);                               \
                                                                      \
            val_m = (ULWORD64) (val1_m);                              \
            val_m = (ULWORD64) ((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (ULWORD64) (val_m | (ULWORD64) val0_m);           \
                                                                      \
            val_m;                                                    \
        } )
    #endif  // (__mips == 64)

    #define SH(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sh_m = (UWORD8 *) (pdst);   \
        UWORD16 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sh  %[val_m],  %[pdst_sh_m]  \n\t"  \
                                                 \
            : [pdst_sh_m] "=m" (*pdst_sh_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #define SW(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sw_m = (UWORD8 *) (pdst);   \
        UWORD32 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                 \
            : [pdst_sw_m] "=m" (*pdst_sw_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #if (__mips == 64)
        #define SD(val, pdst)                        \
        {                                            \
            UWORD8 *pdst_sd_m = (UWORD8 *) (pdst);   \
            ULWORD64 val_m = (val);                  \
                                                     \
            asm volatile (                           \
                "sd  %[val_m],  %[pdst_sd_m]  \n\t"  \
                                                     \
                : [pdst_sd_m] "=m" (*pdst_sd_m)      \
                : [val_m] "r" (val_m)                \
            );                                       \
        }
    #else
        #define SD(val, pdst)                                         \
        {                                                             \
            UWORD8 *pdst_sd_m = (UWORD8 *) (pdst);                    \
            UWORD32 val0_m, val1_m;                                   \
                                                                      \
            val0_m = (UWORD32) ((val) & 0x00000000FFFFFFFF);          \
            val1_m = (UWORD32) (((val) >> 32) & 0x00000000FFFFFFFF);  \
                                                                      \
            SW(val0_m, pdst_sd_m);                                    \
            SW(val1_m, pdst_sd_m + 4);                                \
        }
    #endif
#else
#if (__mips_isa_rev >= 6)
    #define LH(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lh_m = (UWORD8 *) (psrc);   \
        UWORD16 val_m;                           \
                                                 \
        asm volatile (                           \
            "lh  %[val_m],  %[psrc_lh_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lh_m] "m" (*psrc_lh_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #define LW(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lw_m = (UWORD8 *) (psrc);   \
        UWORD32 val_m;                           \
                                                 \
        asm volatile (                           \
            "lw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lw_m] "m" (*psrc_lw_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #if (__mips == 64)
        #define LD(psrc)                             \
        ( {                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);   \
            ULWORD64 val_m = 0;                      \
                                                     \
            asm volatile (                           \
                "ld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                     \
                : [val_m] "=r" (val_m)               \
                : [psrc_ld_m] "m" (*psrc_ld_m)       \
            );                                       \
                                                     \
            val_m;                                   \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                              \
        ( {                                                           \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);                    \
            UWORD32 val0_m, val1_m;                                   \
            ULWORD64 val_m = 0;                                       \
                                                                      \
            val0_m = LW(psrc_ld_m);                                   \
            val1_m = LW(psrc_ld_m + 4);                               \
                                                                      \
            val_m = (ULWORD64) (val1_m);                              \
            val_m = (ULWORD64) ((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (ULWORD64) (val_m | (ULWORD64) val0_m);           \
                                                                      \
            val_m;                                                    \
        } )
    #endif  // (__mips == 64)

    #define SH(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sh_m = (UWORD8 *) (pdst);   \
        UWORD16 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sh  %[val_m],  %[pdst_sh_m]  \n\t"  \
                                                 \
            : [pdst_sh_m] "=m" (*pdst_sh_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #define SW(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sw_m = (UWORD8 *) (pdst);   \
        UWORD32 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                 \
            : [pdst_sw_m] "=m" (*pdst_sw_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #define SD(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sd_m = (UWORD8 *) (pdst);   \
        ULWORD64 val_m = (val);                  \
                                                 \
        asm volatile (                           \
            "sd  %[val_m],  %[pdst_sd_m]  \n\t"  \
                                                 \
            : [pdst_sd_m] "=m" (*pdst_sd_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }
#else  // !(__mips_isa_rev >= 6)
    #define LH(psrc)                              \
    ( {                                           \
        UWORD8 *psrc_lh_m = (UWORD8 *) (psrc);    \
        UWORD16 val_m;                            \
                                                  \
        asm volatile (                            \
            "ulh  %[val_m],  %[psrc_lh_m]  \n\t"  \
                                                  \
            : [val_m] "=r" (val_m)                \
            : [psrc_lh_m] "m" (*psrc_lh_m)        \
        );                                        \
                                                  \
        val_m;                                    \
    } )

    #define LW(psrc)                              \
    ( {                                           \
        UWORD8 *psrc_lw_m = (UWORD8 *) (psrc);    \
        UWORD32 val_m;                            \
                                                  \
        asm volatile (                            \
            "ulw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                  \
            : [val_m] "=r" (val_m)                \
            : [psrc_lw_m] "m" (*psrc_lw_m)        \
        );                                        \
                                                  \
        val_m;                                    \
    } )

    #if (__mips == 64)
        #define LD(psrc)                              \
        ( {                                           \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);    \
            ULWORD64 val_m = 0;                       \
                                                      \
            asm volatile (                            \
                "uld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                      \
                : [val_m] "=r" (val_m)                \
                : [psrc_ld_m] "m" (*psrc_ld_m)        \
            );                                        \
                                                      \
            val_m;                                    \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                              \
        ( {                                                           \
            UWORD8 *psrc_ld_m = (UWORD8 *) (psrc);                    \
            UWORD32 val0_m, val1_m;                                   \
            ULWORD64 val_m = 0;                                       \
                                                                      \
            val0_m = LW(psrc_ld_m);                                   \
            val1_m = LW(psrc_ld_m + 4);                               \
                                                                      \
            val_m = (ULWORD64) (val1_m);                              \
            val_m = (ULWORD64) ((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (ULWORD64) (val_m | (ULWORD64) val0_m);           \
                                                                      \
            val_m;                                                    \
        } )
    #endif  // (__mips == 64)

    #define SH(val, pdst)                         \
    {                                             \
        UWORD8 *pdst_sh_m = (UWORD8 *) (pdst);    \
        UWORD16 val_m = (val);                    \
                                                  \
        asm volatile (                            \
            "ush  %[val_m],  %[pdst_sh_m]  \n\t"  \
                                                  \
            : [pdst_sh_m] "=m" (*pdst_sh_m)       \
            : [val_m] "r" (val_m)                 \
        );                                        \
    }

    #define SW(val, pdst)                         \
    {                                             \
        UWORD8 *pdst_sw_m = (UWORD8 *) (pdst);    \
        UWORD32 val_m = (val);                    \
                                                  \
        asm volatile (                            \
            "usw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                  \
            : [pdst_sw_m] "=m" (*pdst_sw_m)       \
            : [val_m] "r" (val_m)                 \
        );                                        \
    }

    #define SD(val, pdst)                                         \
    {                                                             \
        UWORD8 *pdst_sd_m = (UWORD8 *) (pdst);                    \
        UWORD32 val0_m, val1_m;                                   \
                                                                  \
        val0_m = (UWORD32) ((val) & 0x00000000FFFFFFFF);          \
        val1_m = (UWORD32) (((val) >> 32) & 0x00000000FFFFFFFF);  \
                                                                  \
        SW(val0_m, pdst_sd_m);                                    \
        SW(val1_m, pdst_sd_m + 4);                                \
    }
#endif  // (__mips_isa_rev >= 6)
#endif  // #ifdef CLANG_BUILD

/* Description : Load 4 words with stride
   Arguments   : Inputs  - psrc, stride
                 Outputs - out0, out1, out2, out3
   Details     : Load word in 'out0' from (psrc)
                 Load word in 'out1' from (psrc + stride)
                 Load word in 'out2' from (psrc + 2 * stride)
                 Load word in 'out3' from (psrc + 3 * stride)
*/
#define LW4(psrc, stride, out0, out1, out2, out3)  \
{                                                  \
    out0 = LW((psrc));                             \
    out1 = LW((psrc) + stride);                    \
    out2 = LW((psrc) + 2 * stride);                \
    out3 = LW((psrc) + 3 * stride);                \
}

/* Description : Load double words with stride
   Arguments   : Inputs  - psrc, stride
                 Outputs - out0, out1
   Details     : Load double word in 'out0' from (psrc)
                 Load double word in 'out1' from (psrc + stride)
*/
#define LD2(psrc, stride, out0, out1)  \
{                                      \
    out0 = LD((psrc));                 \
    out1 = LD((psrc) + stride);        \
}
#define LD4(psrc, stride, out0, out1, out2, out3)  \
{                                                  \
    LD2((psrc), stride, out0, out1);               \
    LD2((psrc) + 2 * stride, stride, out2, out3);  \
}

/* Description : Store 4 words with stride
   Arguments   : Inputs - in0, in1, in2, in3, pdst, stride
   Details     : Store word from 'in0' to (pdst)
                 Store word from 'in1' to (pdst + stride)
                 Store word from 'in2' to (pdst + 2 * stride)
                 Store word from 'in3' to (pdst + 3 * stride)
*/
#define SW4(in0, in1, in2, in3, pdst, stride)  \
{                                              \
    SW(in0, (pdst));                           \
    SW(in1, (pdst) + stride);                  \
    SW(in2, (pdst) + 2 * stride);              \
    SW(in3, (pdst) + 3 * stride);              \
}

/* Description : Store 4 double words with stride
   Arguments   : Inputs - in0, in1, in2, in3, pdst, stride
   Details     : Store double word from 'in0' to (pdst)
                 Store double word from 'in1' to (pdst + stride)
                 Store double word from 'in2' to (pdst + 2 * stride)
                 Store double word from 'in3' to (pdst + 3 * stride)
*/
#define SD4(in0, in1, in2, in3, pdst, stride)  \
{                                              \
    SD(in0, (pdst));                           \
    SD(in1, (pdst) + stride);                  \
    SD(in2, (pdst) + 2 * stride);              \
    SD(in3, (pdst) + 3 * stride);              \
}

/* Description : Load vectors with 16 byte elements with stride
   Arguments   : Inputs  - psrc, stride
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Load 16 byte elements in 'out0' from (psrc)
                 Load 16 byte elements in 'out1' from (psrc + stride)
*/
#define LD_V2(RTYPE, psrc, stride, out0, out1)  \
{                                               \
    out0 = LD_V(RTYPE, (psrc));                 \
    out1 = LD_V(RTYPE, (psrc) + stride);        \
}
#define LD_UB2(...) LD_V2(v16u8, __VA_ARGS__)
#define LD_SB2(...) LD_V2(v16i8, __VA_ARGS__)
#define LD_SH2(...) LD_V2(v8i16, __VA_ARGS__)
#define LD_SW2(...) LD_V2(v4i32, __VA_ARGS__)

#define LD_V3(RTYPE, psrc, stride, out0, out1, out2)  \
{                                                     \
    LD_V2(RTYPE, (psrc), stride, out0, out1);         \
    out2 = LD_V(RTYPE, (psrc) + 2 * stride);          \
}
#define LD_UB3(...) LD_V3(v16u8, __VA_ARGS__)
#define LD_SB3(...) LD_V3(v16i8, __VA_ARGS__)

#define LD_V4(RTYPE, psrc, stride, out0, out1, out2, out3)   \
{                                                            \
    LD_V2(RTYPE, (psrc), stride, out0, out1);                \
    LD_V2(RTYPE, (psrc) + 2 * stride , stride, out2, out3);  \
}
#define LD_UB4(...) LD_V4(v16u8, __VA_ARGS__)
#define LD_SB4(...) LD_V4(v16i8, __VA_ARGS__)
#define LD_UH4(...) LD_V4(v8u16, __VA_ARGS__)
#define LD_SH4(...) LD_V4(v8i16, __VA_ARGS__)

#define LD_V5(RTYPE, psrc, stride, out0, out1, out2, out3, out4)  \
{                                                                 \
    LD_V4(RTYPE, (psrc), stride, out0, out1, out2, out3);         \
    out4 = LD_V(RTYPE, (psrc) + 4 * stride);                      \
}
#define LD_SB5(...) LD_V5(v16i8, __VA_ARGS__)

#define LD_V6(RTYPE, psrc, stride, out0, out1, out2, out3, out4, out5)  \
{                                                                       \
    LD_V4(RTYPE, (psrc), stride, out0, out1, out2, out3);               \
    LD_V2(RTYPE, (psrc) + 4 * stride, stride, out4, out5);              \
}
#define LD_UB6(...) LD_V6(v16u8, __VA_ARGS__)
#define LD_SB6(...) LD_V6(v16i8, __VA_ARGS__)
#define LD_SH6(...) LD_V6(v8i16, __VA_ARGS__)

#define LD_V7(RTYPE, psrc, stride,                               \
              out0, out1, out2, out3, out4, out5, out6)          \
{                                                                \
    LD_V5(RTYPE, (psrc), stride, out0, out1, out2, out3, out4);  \
    LD_V2(RTYPE, (psrc) + 5 * stride, stride, out5, out6);       \
}
#define LD_SB7(...) LD_V7(v16i8, __VA_ARGS__)

#define LD_V8(RTYPE, psrc, stride,                                      \
              out0, out1, out2, out3, out4, out5, out6, out7)           \
{                                                                       \
    LD_V4(RTYPE, (psrc), stride, out0, out1, out2, out3);               \
    LD_V4(RTYPE, (psrc) + 4 * stride, stride, out4, out5, out6, out7);  \
}
#define LD_UB8(...) LD_V8(v16u8, __VA_ARGS__)
#define LD_SB8(...) LD_V8(v16i8, __VA_ARGS__)
#define LD_SH8(...) LD_V8(v8i16, __VA_ARGS__)

#define LD_V16(RTYPE, psrc, stride,                                   \
               out0, out1, out2, out3, out4, out5, out6, out7,        \
               out8, out9, out10, out11, out12, out13, out14, out15)  \
{                                                                     \
    LD_V8(RTYPE, (psrc), stride,                                      \
          out0, out1, out2, out3, out4, out5, out6, out7);            \
    LD_V8(RTYPE, (psrc) + 8 * stride, stride,                         \
          out8, out9, out10, out11, out12, out13, out14, out15);      \
}
#define LD_SH16(...) LD_V16(v8i16, __VA_ARGS__)

/* Description : Store vectors of 16 byte elements with stride
   Arguments   : Inputs - in0, in1, pdst, stride
   Details     : Store 16 byte elements from 'in0' to (pdst)
                 Store 16 byte elements from 'in1' to (pdst + stride)
*/
#define ST_V2(RTYPE, in0, in1, pdst, stride)  \
{                                             \
    ST_V(RTYPE, in0, (pdst));                 \
    ST_V(RTYPE, in1, (pdst) + stride);        \
}
#define ST_UB2(...) ST_V2(v16u8, __VA_ARGS__)
#define ST_SB2(...) ST_V2(v16i8, __VA_ARGS__)
#define ST_UH2(...) ST_V2(v8u16, __VA_ARGS__)
#define ST_SH2(...) ST_V2(v8i16, __VA_ARGS__)
#define ST_SW2(...) ST_V2(v4i32, __VA_ARGS__)

#define ST_V3(RTYPE, in0, in1, in2, pdst, stride)  \
{                                                  \
    ST_V2(RTYPE, in0, in1, (pdst), stride);        \
    ST_V(RTYPE, in2, (pdst) + 2 * stride);         \
}
#define ST_SH3(...) ST_V3(v8i16, __VA_ARGS__)

#define ST_V4(RTYPE, in0, in1, in2, in3, pdst, stride)    \
{                                                         \
    ST_V2(RTYPE, in0, in1, (pdst), stride);               \
    ST_V2(RTYPE, in2, in3, (pdst) + 2 * stride, stride);  \
}
#define ST_UB4(...) ST_V4(v16u8, __VA_ARGS__)
#define ST_SB4(...) ST_V4(v16i8, __VA_ARGS__)
#define ST_SH4(...) ST_V4(v8i16, __VA_ARGS__)

#define ST_V6(RTYPE, in0, in1, in2, in3, in4, in5, pdst, stride)  \
{                                                                 \
    ST_V4(RTYPE, in0, in1, in2, in3, (pdst), stride);             \
    ST_V2(RTYPE, in4, in5, (pdst) + 4 * stride, stride);          \
}
#define ST_SH6(...) ST_V6(v8i16, __VA_ARGS__)

#define ST_V8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
              pdst, stride)                                         \
{                                                                   \
    ST_V4(RTYPE, in0, in1, in2, in3, pdst, stride);                 \
    ST_V4(RTYPE, in4, in5, in6, in7, (pdst) + 4 * stride, stride);  \
}
#define ST_UB8(...) ST_V8(v16u8, __VA_ARGS__)
#define ST_SH8(...) ST_V8(v8i16, __VA_ARGS__)

/* Description : Store 4x2 byte block to destination memory from input vector
   Arguments   : Inputs - in, pdst, stride
   Details     : Index 0 word element from 'in' vector is copied to the GP
                 register and stored to (pdst)
                 Index 1 word element from 'in' vector is copied to the GP
                 register and stored to (pdst + stride)
*/
#define ST4x2_UB(in, pdst, stride)           \
{                                            \
    UWORD32 out0_m, out1_m;                  \
    UWORD8 *pblk_4x2_m = (UWORD8 *) (pdst);  \
                                             \
    out0_m = __msa_copy_u_w((v4i32) in, 0);  \
    out1_m = __msa_copy_u_w((v4i32) in, 1);  \
                                             \
    SW(out0_m, pblk_4x2_m);                  \
    SW(out1_m, pblk_4x2_m + stride);         \
}

/* Description : Store 4x4 byte block to destination memory from input vector
   Arguments   : Inputs - in0, in1, pdst, stride
   Details     : 'Idx0' word element from input vector 'in0' is copied to the
                 GP register and stored to (pdst)
                 'Idx1' word element from input vector 'in0' is copied to the
                 GP register and stored to (pdst + stride)
                 'Idx2' word element from input vector 'in0' is copied to the
                 GP register and stored to (pdst + 2 * stride)
                 'Idx3' word element from input vector 'in0' is copied to the
                 GP register and stored to (pdst + 3 * stride)
*/
#define ST4x4_UB(in0, in1, idx0, idx1, idx2, idx3, pdst, stride)  \
{                                                                 \
    UWORD32 out0_m, out1_m, out2_m, out3_m;                       \
    UWORD8 *pblk_4x4_m = (UWORD8 *) (pdst);                       \
                                                                  \
    out0_m = __msa_copy_u_w((v4i32) in0, idx0);                   \
    out1_m = __msa_copy_u_w((v4i32) in0, idx1);                   \
    out2_m = __msa_copy_u_w((v4i32) in1, idx2);                   \
    out3_m = __msa_copy_u_w((v4i32) in1, idx3);                   \
                                                                  \
    SW4(out0_m, out1_m, out2_m, out3_m, pblk_4x4_m, stride);      \
}
#define ST4x8_UB(in0, in1, pdst, stride)                            \
{                                                                   \
    UWORD8 *pblk_4x8 = (UWORD8 *) (pdst);                           \
                                                                    \
    ST4x4_UB(in0, in0, 0, 1, 2, 3, pblk_4x8, stride);               \
    ST4x4_UB(in1, in1, 0, 1, 2, 3, pblk_4x8 + 4 * stride, stride);  \
}

/* Description : Store 8x1 byte block to destination memory from input vector
   Arguments   : Inputs - in, pdst
   Details     : Index 0 double word element from 'in' vector is copied to the
                 GP register and stored to (pdst)
*/
#define ST8x1_UB(in, pdst)                   \
{                                            \
    int64_t out0_m;                          \
    out0_m = __msa_copy_s_d((v2i64) in, 0);  \
    SD(out0_m, pdst);                        \
}

/* Description : Store 8x2 byte block to destination memory from input vector
   Arguments   : Inputs - in, pdst, stride
   Details     : Index 0 double word element from 'in' vector is copied to the
                 GP register and stored to (pdst)
                 Index 1 double word element from 'in' vector is copied to the
                 GP register and stored to (pdst + stride)
*/
#define ST8x2_UB(in, pdst, stride)           \
{                                            \
    ULWORD64 out0_m, out1_m;                 \
    UWORD8 *pblk_8x2_m = (UWORD8 *) (pdst);  \
                                             \
    out0_m = __msa_copy_s_d((v2i64) in, 0);  \
    out1_m = __msa_copy_s_d((v2i64) in, 1);  \
                                             \
    SD(out0_m, pblk_8x2_m);                  \
    SD(out1_m, pblk_8x2_m + stride);         \
}

/* Description : Store 8x4 byte block to destination memory from input
                 vectors
   Arguments   : Inputs - in0, in1, pdst, stride
   Details     : Index 0 double word element from 'in0' vector is copied to the
                 GP register and stored to (pdst)
                 Index 1 double word element from 'in0' vector is copied to the
                 GP register and stored to (pdst + stride)
                 Index 0 double word element from 'in1' vector is copied to the
                 GP register and stored to (pdst + 2 * stride)
                 Index 1 double word element from 'in1' vector is copied to the
                 GP register and stored to (pdst + 3 * stride)
*/
#define ST8x4_UB(in0, in1, pdst, stride)                      \
{                                                             \
    ULWORD64 out0_m, out1_m, out2_m, out3_m;                  \
    UWORD8 *pblk_8x4_m = (UWORD8 *) (pdst);                   \
                                                              \
    out0_m = __msa_copy_u_d((v2i64) in0, 0);                  \
    out1_m = __msa_copy_u_d((v2i64) in0, 1);                  \
    out2_m = __msa_copy_u_d((v2i64) in1, 0);                  \
    out3_m = __msa_copy_u_d((v2i64) in1, 1);                  \
                                                              \
    SD4(out0_m, out1_m, out2_m, out3_m, pblk_8x4_m, stride);  \
}
#define ST8x8_UB(in0, in1, in2, in3, pdst, stride)        \
{                                                         \
    UWORD8 *pblk_8x8_m = (UWORD8 *) (pdst);               \
                                                          \
    ST8x4_UB(in0, in1, pblk_8x8_m, stride);               \
    ST8x4_UB(in2, in3, pblk_8x8_m + 4 * stride, stride);  \
}
#define ST12x4_UB(in0, in1, in2, pdst, stride)                \
{                                                             \
    UWORD8 *pblk_12x4_m = (UWORD8 *) (pdst);                  \
                                                              \
    /* left 8x4 */                                            \
    ST8x4_UB(in0, in1, pblk_12x4_m, stride);                  \
    /* right 4x4 */                                           \
    ST4x4_UB(in2, in2, 0, 1, 2, 3, pblk_12x4_m + 8, stride);  \
}

/* Description : Store 12x8 byte block to destination memory from input
                 vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride
   Details     : Index 0 double word element from input vector 'in0' is copied
                 and stored to destination memory at (pdst) followed by
                 index 2 word element from same input vector 'in0' at
                 (pdst + 8)
*/
#define ST12x8_UB(in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride)  \
{                                                                        \
    ULWORD64 out0_m, out1_m, out2_m, out3_m;                             \
    ULWORD64 out4_m, out5_m, out6_m, out7_m;                             \
    UWORD32 out8_m, out9_m, out10_m, out11_m;                            \
    UWORD32 out12_m, out13_m, out14_m, out15_m;                          \
    UWORD8 *pblk_12x8_m = (UWORD8 *) (pdst);                             \
                                                                         \
    out0_m = __msa_copy_u_d((v2i64) in0, 0);                             \
    out1_m = __msa_copy_u_d((v2i64) in1, 0);                             \
    out2_m = __msa_copy_u_d((v2i64) in2, 0);                             \
    out3_m = __msa_copy_u_d((v2i64) in3, 0);                             \
    out4_m = __msa_copy_u_d((v2i64) in4, 0);                             \
    out5_m = __msa_copy_u_d((v2i64) in5, 0);                             \
    out6_m = __msa_copy_u_d((v2i64) in6, 0);                             \
    out7_m = __msa_copy_u_d((v2i64) in7, 0);                             \
                                                                         \
    out8_m =  __msa_copy_u_w((v4i32) in0, 2);                            \
    out9_m =  __msa_copy_u_w((v4i32) in1, 2);                            \
    out10_m = __msa_copy_u_w((v4i32) in2, 2);                            \
    out11_m = __msa_copy_u_w((v4i32) in3, 2);                            \
    out12_m = __msa_copy_u_w((v4i32) in4, 2);                            \
    out13_m = __msa_copy_u_w((v4i32) in5, 2);                            \
    out14_m = __msa_copy_u_w((v4i32) in6, 2);                            \
    out15_m = __msa_copy_u_w((v4i32) in7, 2);                            \
                                                                         \
    SD(out0_m, pblk_12x8_m);                                             \
    SW(out8_m, pblk_12x8_m + 8);                                         \
    pblk_12x8_m += stride;                                               \
    SD(out1_m, pblk_12x8_m);                                             \
    SW(out9_m, pblk_12x8_m + 8);                                         \
    pblk_12x8_m += stride;                                               \
    SD(out2_m, pblk_12x8_m);                                             \
    SW(out10_m, pblk_12x8_m + 8);                                        \
    pblk_12x8_m += stride;                                               \
    SD(out3_m, pblk_12x8_m);                                             \
    SW(out11_m, pblk_12x8_m + 8);                                        \
    pblk_12x8_m += stride;                                               \
    SD(out4_m, pblk_12x8_m);                                             \
    SW(out12_m, pblk_12x8_m + 8);                                        \
    pblk_12x8_m += stride;                                               \
    SD(out5_m, pblk_12x8_m);                                             \
    SW(out13_m, pblk_12x8_m + 8);                                        \
    pblk_12x8_m += stride;                                               \
    SD(out6_m, pblk_12x8_m);                                             \
    SW(out14_m, pblk_12x8_m + 8);                                        \
    pblk_12x8_m += stride;                                               \
    SD(out7_m, pblk_12x8_m);                                             \
    SW(out15_m, pblk_12x8_m + 8);                                        \
}

/* Description : Immediate number of elements to slide with zero
   Arguments   : Inputs  - in0, in1, slide_val
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'zero_m' vector are slid into 'in0' by
                 value specified in the 'slide_val'
*/
#define SLDI_B2_0(RTYPE, in0, in1, out0, out1, slide_val)                 \
{                                                                         \
    v16i8 zero_m = { 0 };                                                 \
    out0 = (RTYPE) __msa_sldi_b((v16i8) zero_m, (v16i8) in0, slide_val);  \
    out1 = (RTYPE) __msa_sldi_b((v16i8) zero_m, (v16i8) in1, slide_val);  \
}

/* Description : Immediate number of elements to slide
   Arguments   : Inputs  - in0_0, in0_1, in1_0, in1_1, slide_val
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'in0_0' vector are slid into 'in1_0' by
                 value specified in the 'slide_val'
*/
#define SLDI_B2(RTYPE, in0_0, in0_1, in1_0, in1_1, out0, out1, slide_val)  \
{                                                                          \
    out0 = (RTYPE) __msa_sldi_b((v16i8) in0_0, (v16i8) in1_0, slide_val);  \
    out1 = (RTYPE) __msa_sldi_b((v16i8) in0_1, (v16i8) in1_1, slide_val);  \
}
#define SLDI_B2_UB(...) SLDI_B2(v16u8, __VA_ARGS__)
#define SLDI_B2_SB(...) SLDI_B2(v16i8, __VA_ARGS__)

#define SLDI_B3(RTYPE, in0_0, in0_1, in0_2, in1_0, in1_1, in1_2,           \
                out0, out1, out2, slide_val)                               \
{                                                                          \
    SLDI_B2(RTYPE, in0_0, in0_1, in1_0, in1_1, out0, out1, slide_val)      \
    out2 = (RTYPE) __msa_sldi_b((v16i8) in0_2, (v16i8) in1_2, slide_val);  \
}
#define SLDI_B3_SB(...) SLDI_B3(v16i8, __VA_ARGS__)

/* Description : Shuffle byte vector elements as per mask vector
   Arguments   : Inputs  - in0, in1, in2, in3, mask0, mask1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'in0' & 'in1' are copied selectively to
                 'out0' as per control vector 'mask0'
*/
/* Description : Shuffle byte vector elements as per mask vector
 * Arguments   : Inputs  - in0, in1, in2, in3, mask0, mask1
 *               Outputs - out0, out1
 *               Return Type - as per RTYPE
 * Details     : Byte elements from 'in0' & 'in1' are copied selectively to
 *               'out0' as per control vector 'mask0'
 */
#define VSHF_B2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1)       \
{                                                                          \
    out0 = (RTYPE) __msa_vshf_b((v16i8) mask0, (v16i8) in1, (v16i8) in0);  \
    out1 = (RTYPE) __msa_vshf_b((v16i8) mask1, (v16i8) in3, (v16i8) in2);  \
}
#define VSHF_B2_SB(...) VSHF_B2(v16i8, __VA_ARGS__)
#define VSHF_B2_SH(...) VSHF_B2(v8i16, __VA_ARGS__)

#define VSHF_B3(RTYPE, in0, in1, in2, in3, in4, in5, mask0, mask1, mask2,  \
                out0, out1, out2)                                          \
{                                                                          \
    VSHF_B2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1);          \
    out2 = (RTYPE) __msa_vshf_b((v16i8) mask2, (v16i8) in5, (v16i8) in4);  \
}
#define VSHF_B3_SB(...) VSHF_B3(v16i8, __VA_ARGS__)

#define VSHF_B4(RTYPE, in0, in1, mask0, mask1, mask2, mask3,       \
                out0, out1, out2, out3)                            \
{                                                                  \
    VSHF_B2(RTYPE, in0, in1, in0, in1, mask0, mask1, out0, out1);  \
    VSHF_B2(RTYPE, in0, in1, in0, in1, mask2, mask3, out2, out3);  \
}
#define VSHF_B4_SB(...) VSHF_B4(v16i8, __VA_ARGS__)

/* Description : Shuffle word vector elements as per mask vector
   Arguments   : Inputs  - in0, in1, in2, in3, mask0, mask1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Word elements from 'in0' & 'in1' are copied selectively to
                 'out0' as per control vector 'mask0'
*/
#define VSHF_W2(RTYPE, in0, in1, in2, in3, mask0, mask1, out0, out1)      \
{                                                                         \
    out0 = (RTYPE) __msa_vshf_w((v4i32) mask0, (v4i32) in1, (v4i32) in0); \
    out1 = (RTYPE) __msa_vshf_w((v4i32) mask1, (v4i32) in3, (v4i32) in2); \
}
#define VSHF_W2_SB(...) VSHF_W2(v16i8, __VA_ARGS__)

/* Description : Dot product of byte vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Unsigned byte elements from 'mult0' are multiplied with
                 unsigned byte elements from 'cnst0' producing a result
                 twice the size of input i.e. unsigned halfword.
                 The multiplication result of adjacent odd-even elements
                 are added together and written to the 'out0' vector
*/
#define DOTP_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                 \
    out0 = (RTYPE) __msa_dotp_u_h((v16u8) mult0, (v16u8) cnst0);  \
    out1 = (RTYPE) __msa_dotp_u_h((v16u8) mult1, (v16u8) cnst1);  \
}
#define DOTP_UB2_UH(...) DOTP_UB2(v8u16, __VA_ARGS__)

#define DOTP_UB4(RTYPE, mult0, mult1, mult2, mult3,           \
                 cnst0, cnst1, cnst2, cnst3,                  \
                 out0, out1, out2, out3)                      \
{                                                             \
    DOTP_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);  \
    DOTP_UB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);  \
}
#define DOTP_UB4_UH(...) DOTP_UB4(v8u16, __VA_ARGS__)

/* Description : Dot product of byte vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed byte elements from 'mult0' are multiplied with
                 signed byte elements from 'cnst0' producing a result
                 twice the size of input i.e. signed halfword.
                 The multiplication result of adjacent odd-even elements
                 are added together and written to the 'out0' vector
*/
#define DOTP_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                 \
    out0 = (RTYPE) __msa_dotp_s_h((v16i8) mult0, (v16i8) cnst0);  \
    out1 = (RTYPE) __msa_dotp_s_h((v16i8) mult1, (v16i8) cnst1);  \
}
#define DOTP_SB2_SH(...) DOTP_SB2(v8i16, __VA_ARGS__)

#define DOTP_SB3(RTYPE, mult0, mult1, mult2, cnst0, cnst1, cnst2,  \
                 out0, out1, out2)                                 \
{                                                                  \
    DOTP_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);       \
    out2 = (RTYPE) __msa_dotp_s_h((v16i8) mult2, (v16i8) cnst2);   \
}
#define DOTP_SB3_SH(...) DOTP_SB3(v8i16, __VA_ARGS__)

#define DOTP_SB4(RTYPE, mult0, mult1, mult2, mult3,                   \
                 cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                     \
    DOTP_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DOTP_SB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DOTP_SB4_SH(...) DOTP_SB4(v8i16, __VA_ARGS__)

/* Description : Dot product of halfword vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed halfword elements from 'mult0' are multiplied with
                 signed halfword elements from 'cnst0' producing a result
                 twice the size of input i.e. signed word.
                 The multiplication result of adjacent odd-even elements
                 are added together and written to the 'out0' vector
*/
#define DOTP_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                 \
    out0 = (RTYPE) __msa_dotp_s_w((v8i16) mult0, (v8i16) cnst0);  \
    out1 = (RTYPE) __msa_dotp_s_w((v8i16) mult1, (v8i16) cnst1);  \
}
#define DOTP_SH2_SW(...) DOTP_SH2(v4i32, __VA_ARGS__)

#define DOTP_SH4(RTYPE, mult0, mult1, mult2, mult3,           \
                 cnst0, cnst1, cnst2, cnst3,                  \
                 out0, out1, out2, out3)                      \
{                                                             \
    DOTP_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);  \
    DOTP_SH2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);  \
}
#define DOTP_SH4_SW(...) DOTP_SH4(v4i32, __VA_ARGS__)

/* Description : Dot product & addition of byte vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed byte elements from 'mult0' are multiplied with
                 signed byte elements from 'cnst0' producing a result
                 twice the size of input i.e. signed halfword.
                 The multiplication result of adjacent odd-even elements
                 are added to the 'out0' vector
*/
#define DPADD_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                  \
    out0 = (RTYPE) __msa_dpadd_s_h((v8i16) out0,                   \
                                   (v16i8) mult0, (v16i8) cnst0);  \
    out1 = (RTYPE) __msa_dpadd_s_h((v8i16) out1,                   \
                                   (v16i8) mult1, (v16i8) cnst1);  \
}
#define DPADD_SB2_SH(...) DPADD_SB2(v8i16, __VA_ARGS__)

#define DPADD_SB4(RTYPE, mult0, mult1, mult2, mult3,                   \
                  cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                      \
    DPADD_SB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DPADD_SB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DPADD_SB4_SH(...) DPADD_SB4(v8i16, __VA_ARGS__)

/* Description : Dot product & addition of byte vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - unsigned halfword
   Details     : Unsigned byte elements from 'mult0' are multiplied with
                 unsigned byte elements from 'cnst0' producing a result
                 twice the size of input i.e. unsigned halfword.
                 The multiplication result of adjacent odd-even elements
                 are added to the 'out0' vector
*/
#define DPADD_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                  \
    out0 = (RTYPE) __msa_dpadd_u_h((v8u16) out0,                   \
                                   (v16u8) mult0, (v16u8) cnst0);  \
    out1 = (RTYPE) __msa_dpadd_u_h((v8u16) out1,                   \
                                   (v16u8) mult1, (v16u8) cnst1);  \
}
#define DPADD_UB2_UH(...) DPADD_UB2(v8u16, __VA_ARGS__)

#define DPADD_UB4(RTYPE, mult0, mult1, mult2, mult3,                   \
                  cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                      \
    DPADD_UB2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DPADD_UB2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DPADD_UB4_UH(...) DPADD_UB4(v8u16, __VA_ARGS__)

/* Description : Dot product & addition of halfword vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed halfword elements from 'mult0' are multiplied with
                 signed halfword elements from 'cnst0' producing a result
                 twice the size of input i.e. signed word.
                 The multiplication result of adjacent odd-even elements
                 are added to the 'out0' vector
*/
#define DPADD_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)   \
{                                                                  \
    out0 = (RTYPE) __msa_dpadd_s_w((v4i32) out0,                   \
                                   (v8i16) mult0, (v8i16) cnst0);  \
    out1 = (RTYPE) __msa_dpadd_s_w((v4i32) out1,                   \
                                   (v8i16) mult1, (v8i16) cnst1);  \
}
#define DPADD_SH2_SW(...) DPADD_SH2(v4i32, __VA_ARGS__)

#define DPADD_SH4(RTYPE, mult0, mult1, mult2, mult3,                   \
                  cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                      \
    DPADD_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DPADD_SH2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DPADD_SH4_SW(...) DPADD_SH4(v4i32, __VA_ARGS__)

/* Description : Clips all halfword elements of input vector between min & max
                 out = (in < min) ? min : ((in > max) ? max : in)
   Arguments   : Inputs - in, min, max
                 Output - out_m
                 Return Type - signed halfword
*/
#define CLIP_SH(in, min, max)                           \
( {                                                     \
    v8i16 out_m;                                        \
                                                        \
    out_m = __msa_max_s_h((v8i16) min, (v8i16) in);     \
    out_m = __msa_min_s_h((v8i16) max, (v8i16) out_m);  \
    out_m;                                              \
} )

/* Description : Clips all signed halfword elements of input vector
                 between 0 & 255
   Arguments   : Input  - in
                 Output - out_m
                 Return Type - signed halfword
*/
#define CLIP_SH_0_255(in)                                 \
( {                                                       \
    v8i16 max_m = __msa_ldi_h(255);                       \
    v8i16 out_m;                                          \
                                                          \
    out_m = __msa_maxi_s_h((v8i16) in, 0);                \
    out_m = __msa_min_s_h((v8i16) max_m, (v8i16) out_m);  \
    out_m;                                                \
} )
#define CLIP_SH2_0_255(in0, in1)  \
{                                 \
    in0 = CLIP_SH_0_255(in0);     \
    in1 = CLIP_SH_0_255(in1);     \
}
#define CLIP_SH4_0_255(in0, in1, in2, in3)  \
{                                           \
    CLIP_SH2_0_255(in0, in1);               \
    CLIP_SH2_0_255(in2, in3);               \
}

/* Description : Clips all signed word elements of input vector
                 between 0 & 255
   Arguments   : Input  - in
                 Output - out_m
                 Return Type - signed word
*/
#define CLIP_SW_0_255(in)                                 \
( {                                                       \
    v4i32 max_m = __msa_ldi_w(255);                       \
    v4i32 out_m;                                          \
                                                          \
    out_m = __msa_maxi_s_w((v4i32) in, 0);                \
    out_m = __msa_min_s_w((v4i32) max_m, (v4i32) out_m);  \
    out_m;                                                \
} )
#define CLIP_SW2_0_255(in0, in1)  \
{                                 \
    in0 = CLIP_SW_0_255(in0);     \
    in1 = CLIP_SW_0_255(in1);     \
}
#define CLIP_SW4_0_255(in0, in1, in2, in3)  \
{                                           \
    CLIP_SW2_0_255(in0, in1);               \
    CLIP_SW2_0_255(in2, in3);               \
}

/* Description : Horizontal addition of unsigned byte vector elements
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each unsigned odd byte element from 'in0' is added to
                 even unsigned byte element from 'in0' (pairwise) and the
                 halfword result is written to 'out0'
*/
#define HADD_UB2(RTYPE, in0, in1, out0, out1)                 \
{                                                             \
    out0 = (RTYPE) __msa_hadd_u_h((v16u8) in0, (v16u8) in0);  \
    out1 = (RTYPE) __msa_hadd_u_h((v16u8) in1, (v16u8) in1);  \
}
#define HADD_UB2_UH(...) HADD_UB2(v8u16, __VA_ARGS__)

/* Description : Set element n input vector to GPR value
   Arguments   : Inputs - in0, in1, in2, in3
                 Output - out
                 Return Type - as per RTYPE
   Details     : Set element 0 in vector 'out' to value specified in 'in0'
*/
#define INSERT_W2(RTYPE, in0, in1, out)                 \
{                                                       \
    out = (RTYPE) __msa_insert_w((v4i32) out, 0, in0);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 1, in1);  \
}
#define INSERT_W2_SB(...) INSERT_W2(v16i8, __VA_ARGS__)

#define INSERT_W4(RTYPE, in0, in1, in2, in3, out)       \
{                                                       \
    out = (RTYPE) __msa_insert_w((v4i32) out, 0, in0);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 1, in1);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 2, in2);  \
    out = (RTYPE) __msa_insert_w((v4i32) out, 3, in3);  \
}
#define INSERT_W4_UB(...) INSERT_W4(v16u8, __VA_ARGS__)
#define INSERT_W4_SW(...) INSERT_W4(v4i32, __VA_ARGS__)

#define INSERT_D2(RTYPE, in0, in1, out)                 \
{                                                       \
    out = (RTYPE) __msa_insert_d((v2i64) out, 0, in0);  \
    out = (RTYPE) __msa_insert_d((v2i64) out, 1, in1);  \
}
#define INSERT_D2_UB(...) INSERT_D2(v16u8, __VA_ARGS__)
#define INSERT_D2_SB(...) INSERT_D2(v16i8, __VA_ARGS__)
#define INSERT_D2_SD(...) INSERT_D2(v2i64, __VA_ARGS__)

/* Description : Interleave even byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even byte elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'
*/
#define ILVEV_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_b((v16i8) in1, (v16i8) in0);  \
    out1 = (RTYPE) __msa_ilvev_b((v16i8) in3, (v16i8) in2);  \
}
#define ILVEV_B2_SH(...) ILVEV_B2(v8i16, __VA_ARGS__)

/* Description : Interleave even word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even word elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'
*/
#define ILVEV_W2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_w((v4i32) in1, (v4i32) in0);  \
    out1 = (RTYPE) __msa_ilvev_w((v4i32) in3, (v4i32) in2);  \
}
#define ILVEV_W2_SB(...) ILVEV_W2(v16i8, __VA_ARGS__)

/* Description : Interleave even double word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even double word elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'
*/
#define ILVEV_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_ilvev_d((v2i64) in1, (v2i64) in0);  \
    out1 = (RTYPE) __msa_ilvev_d((v2i64) in3, (v2i64) in2);  \
}
#define ILVEV_D2_SB(...) ILVEV_D2(v16i8, __VA_ARGS__)

/* Description : Interleave left half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of byte elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'.
*/
#define ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvl_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvl_b((v16i8) in2, (v16i8) in3);  \
}
#define ILVL_B2_SB(...) ILVL_B2(v16i8, __VA_ARGS__)
#define ILVL_B2_SH(...) ILVL_B2(v8i16, __VA_ARGS__)

#define ILVL_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVL_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVL_B4_SB(...) ILVL_B4(v16i8, __VA_ARGS__)
#define ILVL_B4_SH(...) ILVL_B4(v8i16, __VA_ARGS__)

/* Description : Interleave left half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of halfword elements of 'in0' and 'in1' are
                 interleaved and written to 'out0'.
*/
#define ILVL_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvl_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvl_h((v8i16) in2, (v8i16) in3);  \
}
#define ILVL_H2_SH(...) ILVL_H2(v8i16, __VA_ARGS__)
#define ILVL_H2_SW(...) ILVL_H2(v4i32, __VA_ARGS__)

#define ILVL_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVL_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVL_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVL_H4_SH(...) ILVL_H4(v8i16, __VA_ARGS__)

/* Description : Interleave left half of word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of word elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'.
*/
#define ILVL_W2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvl_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvl_w((v4i32) in2, (v4i32) in3);  \
}
#define ILVL_W2_SB(...) ILVL_W2(v16i8, __VA_ARGS__)
#define ILVL_W2_SH(...) ILVL_W2(v8i16, __VA_ARGS__)

/* Description : Interleave right half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of byte elements of 'in0' and 'in1' are interleaved
                 and written to out0.
*/
#define ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvr_b((v16i8) in2, (v16i8) in3);  \
}
#define ILVR_B2_SB(...) ILVR_B2(v16i8, __VA_ARGS__)
#define ILVR_B2_SH(...) ILVR_B2(v8i16, __VA_ARGS__)

#define ILVR_B3(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1, out2)  \
{                                                                       \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);                     \
    out2 = (RTYPE) __msa_ilvr_b((v16i8) in4, (v16i8) in5);              \
}
#define ILVR_B3_SH(...) ILVR_B3(v8i16, __VA_ARGS__)

#define ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_B4_SB(...) ILVR_B4(v16i8, __VA_ARGS__)
#define ILVR_B4_SH(...) ILVR_B4(v8i16, __VA_ARGS__)

#define ILVR_B8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,    \
                in8, in9, in10, in11, in12, in13, in14, in15,     \
                out0, out1, out2, out3, out4, out5, out6, out7)   \
{                                                                 \
    ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
            out0, out1, out2, out3);                              \
    ILVR_B4(RTYPE, in8, in9, in10, in11, in12, in13, in14, in15,  \
            out4, out5, out6, out7);                              \
}
#define ILVR_B8_UH(...) ILVR_B8(v8u16, __VA_ARGS__)

/* Description : Interleave right half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of halfword elements of 'in0' and 'in1' are
                 interleaved and written to 'out0'.
*/
#define ILVR_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvr_h((v8i16) in2, (v8i16) in3);  \
}
#define ILVR_H2_SH(...) ILVR_H2(v8i16, __VA_ARGS__)
#define ILVR_H2_SW(...) ILVR_H2(v4i32, __VA_ARGS__)

#define ILVR_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_H4_SH(...) ILVR_H4(v8i16, __VA_ARGS__)
#define ILVR_H4_SW(...) ILVR_H4(v4i32, __VA_ARGS__)

#define ILVR_W2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvr_w((v4i32) in2, (v4i32) in3);  \
}
#define ILVR_W2_UB(...) ILVR_W2(v16u8, __VA_ARGS__)
#define ILVR_W2_SB(...) ILVR_W2(v16i8, __VA_ARGS__)
#define ILVR_W2_SH(...) ILVR_W2(v8i16, __VA_ARGS__)

#define ILVR_W4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_W2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_W2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_W4_SB(...) ILVR_W4(v16i8, __VA_ARGS__)

/* Description : Interleave right half of double word elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of double word elements of 'in0' and 'in1' are
                 interleaved and written to 'out0'.
*/
#define ILVR_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_ilvr_d((v2i64) in2, (v2i64) in3);  \
}
#define ILVR_D2_UB(...) ILVR_D2(v16u8, __VA_ARGS__)
#define ILVR_D2_SB(...) ILVR_D2(v16i8, __VA_ARGS__)
#define ILVR_D2_SH(...) ILVR_D2(v8i16, __VA_ARGS__)

#define ILVR_D3(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1, out2)  \
{                                                                       \
    ILVR_D2(RTYPE, in0, in1, in2, in3, out0, out1);                     \
    out2 = (RTYPE) __msa_ilvr_d((v2i64) in4, (v2i64) in5);              \
}
#define ILVR_D3_SB(...) ILVR_D3(v16i8, __VA_ARGS__)

#define ILVR_D4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_D2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_D2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_D4_SB(...) ILVR_D4(v16i8, __VA_ARGS__)

/* Description : Interleave both left and right half of input vectors
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of byte elements from 'in0' and 'in1' are
                 interleaved and written to 'out0'
*/
#define ILVRL_B2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_ilvl_b((v16i8) in0, (v16i8) in1);  \
}
#define ILVRL_B2_SB(...) ILVRL_B2(v16i8, __VA_ARGS__)
#define ILVRL_B2_UH(...) ILVRL_B2(v8u16, __VA_ARGS__)
#define ILVRL_B2_SH(...) ILVRL_B2(v8i16, __VA_ARGS__)

#define ILVRL_H2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_ilvl_h((v8i16) in0, (v8i16) in1);  \
}
#define ILVRL_H2_SB(...) ILVRL_H2(v16i8, __VA_ARGS__)
#define ILVRL_H2_SH(...) ILVRL_H2(v8i16, __VA_ARGS__)
#define ILVRL_H2_SW(...) ILVRL_H2(v4i32, __VA_ARGS__)

#define ILVRL_W2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_w((v4i32) in0, (v4i32) in1);  \
    out1 = (RTYPE) __msa_ilvl_w((v4i32) in0, (v4i32) in1);  \
}
#define ILVRL_W2_SH(...) ILVRL_W2(v8i16, __VA_ARGS__)
#define ILVRL_W2_SW(...) ILVRL_W2(v4i32, __VA_ARGS__)

#define ILVRL_D2(RTYPE, in0, in1, out0, out1)               \
{                                                           \
    out0 = (RTYPE) __msa_ilvr_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_ilvl_d((v2i64) in0, (v2i64) in1);  \
}
#define ILVRL_D2_SW(...) ILVRL_D2(v4i32, __VA_ARGS__)

/* Description : Saturate the halfword element values to the max
                 unsigned value of (sat_val + 1) bits
                 The element data width remains unchanged
   Arguments   : Inputs  - in0, in1, sat_val
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each unsigned halfword element from 'in0' is saturated to the
                 value generated with (sat_val + 1) bit range
                 The results are written in place
*/
#define SAT_SH2(RTYPE, in0, in1, sat_val)               \
{                                                       \
    in0 = (RTYPE) __msa_sat_s_h((v8i16) in0, sat_val);  \
    in1 = (RTYPE) __msa_sat_s_h((v8i16) in1, sat_val);  \
}
#define SAT_SH2_SH(...) SAT_SH2(v8i16, __VA_ARGS__)

#define SAT_SH3(RTYPE, in0, in1, in2, sat_val)          \
{                                                       \
    SAT_SH2(RTYPE, in0, in1, sat_val);                  \
    in2 = (RTYPE) __msa_sat_s_h((v8i16) in2, sat_val);  \
}
#define SAT_SH3_SH(...) SAT_SH3(v8i16, __VA_ARGS__)

#define SAT_SH4(RTYPE, in0, in1, in2, in3, sat_val)  \
{                                                    \
    SAT_SH2(RTYPE, in0, in1, sat_val);               \
    SAT_SH2(RTYPE, in2, in3, sat_val);               \
}
#define SAT_SH4_SH(...) SAT_SH4(v8i16, __VA_ARGS__)

/* Description : Saturate the word element values to the max
                 unsigned value of (sat_val + 1) bits
                 The element data width remains unchanged
   Arguments   : Inputs  - in0, in1, sat_val
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each unsigned word element from 'in0' is saturated to the
                 value generated with (sat_val + 1) bit range
                 The results are written in place
*/
#define SAT_SW2(RTYPE, in0, in1, sat_val)               \
{                                                       \
    in0 = (RTYPE) __msa_sat_s_w((v4i32) in0, sat_val);  \
    in1 = (RTYPE) __msa_sat_s_w((v4i32) in1, sat_val);  \
}
#define SAT_SW2_SW(...) SAT_SW2(v4i32, __VA_ARGS__)

#define SAT_SW4(RTYPE, in0, in1, in2, in3, sat_val)  \
{                                                    \
    SAT_SW2(RTYPE, in0, in1, sat_val);               \
    SAT_SW2(RTYPE, in2, in3, sat_val);               \
}
#define SAT_SW4_SW(...) SAT_SW4(v4i32, __VA_ARGS__)

/* Description : Indexed halfword element values are replicated to all
                 elements in output vector
   Arguments   : Inputs  - in, idx0, idx1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : 'idx0' element value from 'in' vector is replicated to all
                  elements in 'out0' vector
                  Valid index range for halfword operation is 0-7
*/
#define SPLATI_H2(RTYPE, in, idx0, idx1, out0, out1)  \
{                                                     \
    out0 = (RTYPE) __msa_splati_h((v8i16) in, idx0);  \
    out1 = (RTYPE) __msa_splati_h((v8i16) in, idx1);  \
}
#define SPLATI_H2_SB(...) SPLATI_H2(v16i8, __VA_ARGS__)
#define SPLATI_H2_SH(...) SPLATI_H2(v8i16, __VA_ARGS__)

#define SPLATI_H4(RTYPE, in, idx0, idx1, idx2, idx3,  \
                  out0, out1, out2, out3)             \
{                                                     \
    SPLATI_H2(RTYPE, in, idx0, idx1, out0, out1);     \
    SPLATI_H2(RTYPE, in, idx2, idx3, out2, out3);     \
}
#define SPLATI_H4_SB(...) SPLATI_H4(v16i8, __VA_ARGS__)
#define SPLATI_H4_SH(...) SPLATI_H4(v8i16, __VA_ARGS__)

/* Description : Indexed word element values are replicated to all
                 elements in output vector
   Arguments   : Inputs  - in, stidx
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : 'stidx' element value from 'in' vector is replicated to all
                 elements in 'out0' vector
                 'stidx + 1' element value from 'in' vector is replicated to all
                 elements in 'out1' vector
                 Valid index range for word operation is 0-3
*/
#define SPLATI_W2(RTYPE, in, stidx, out0, out1)            \
{                                                          \
    out0 = (RTYPE) __msa_splati_w((v4i32) in, stidx);      \
    out1 = (RTYPE) __msa_splati_w((v4i32) in, (stidx+1));  \
}
#define SPLATI_W2_SH(...) SPLATI_W2(v8i16, __VA_ARGS__)
#define SPLATI_W2_SW(...) SPLATI_W2(v4i32, __VA_ARGS__)

#define SPLATI_W4(RTYPE, in, out0, out1, out2, out3)  \
{                                                     \
    SPLATI_W2(RTYPE, in, 0, out0, out1);              \
    SPLATI_W2(RTYPE, in, 2, out2, out3);              \
}
#define SPLATI_W4_SH(...) SPLATI_W4(v8i16, __VA_ARGS__)
#define SPLATI_W4_SW(...) SPLATI_W4(v4i32, __VA_ARGS__)

/* Description : Pack even byte elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even byte elements of 'in0' are copied to the left half of
                 'out0' & even byte elements of 'in1' are copied to the right
                 half of 'out0'.
*/
#define PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_b((v16i8) in0, (v16i8) in1);  \
    out1 = (RTYPE) __msa_pckev_b((v16i8) in2, (v16i8) in3);  \
}
#define PCKEV_B2_SB(...) PCKEV_B2(v16i8, __VA_ARGS__)
#define PCKEV_B2_UB(...) PCKEV_B2(v16u8, __VA_ARGS__)
#define PCKEV_B2_SH(...) PCKEV_B2(v8i16, __VA_ARGS__)
#define PCKEV_B2_SW(...) PCKEV_B2(v4i32, __VA_ARGS__)

#define PCKEV_B3(RTYPE, in0, in1, in2, in3, in4, in5, out0, out1, out2)  \
{                                                                        \
    PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1);                     \
    out2 = (RTYPE) __msa_pckev_b((v16i8) in4, (v16i8) in5);              \
}
#define PCKEV_B3_SB(...) PCKEV_B3(v16i8, __VA_ARGS__)

#define PCKEV_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_B4_SB(...) PCKEV_B4(v16i8, __VA_ARGS__)
#define PCKEV_B4_UB(...) PCKEV_B4(v16u8, __VA_ARGS__)
#define PCKEV_B4_SH(...) PCKEV_B4(v8i16, __VA_ARGS__)

/* Description : Pack even halfword elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even halfword elements of 'in0' are copied to the left half of
                 'out0' & even halfword elements of 'in1' are copied to the
                 right half of 'out0'.
*/
#define PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_pckev_h((v8i16) in2, (v8i16) in3);  \
}
#define PCKEV_H2_SH(...) PCKEV_H2(v8i16, __VA_ARGS__)
#define PCKEV_H2_SW(...) PCKEV_H2(v4i32, __VA_ARGS__)

#define PCKEV_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_H4_SH(...) PCKEV_H4(v8i16, __VA_ARGS__)
#define PCKEV_H4_SW(...) PCKEV_H4(v4i32, __VA_ARGS__)

/* Description : Pack even double word elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even double elements of 'in0' are copied to the left half of
                 'out0' & even double elements of 'in1' are copied to the right
                 half of 'out0'.
*/
#define PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckev_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_pckev_d((v2i64) in2, (v2i64) in3);  \
}
#define PCKEV_D2_SH(...) PCKEV_D2(v8i16, __VA_ARGS__)

#define PCKEV_D4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_D2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}

/* Description : Pack odd double word elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Odd double word elements of 'in0' are copied to the left half
                 of 'out0' & odd double word elements of 'in1' are copied to
                 the right half of 'out0'.
*/
#define PCKOD_D2(RTYPE, in0, in1, in2, in3, out0, out1)      \
{                                                            \
    out0 = (RTYPE) __msa_pckod_d((v2i64) in0, (v2i64) in1);  \
    out1 = (RTYPE) __msa_pckod_d((v2i64) in2, (v2i64) in3);  \
}

/* Description : Each byte element is logically xor'ed with immediate 128
   Arguments   : Inputs  - in0, in1
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each unsigned byte element from input vector 'in0' is
                 logically xor'ed with 128 and the result is stored in-place.
*/
#define XORI_B2_128(RTYPE, in0, in1)  \
{                                     \
    in0 = (RTYPE) XORI_B(in0, 128);   \
    in1 = (RTYPE) XORI_B(in1, 128);   \
}
#define XORI_B2_128_UB(...) XORI_B2_128(v16u8, __VA_ARGS__)
#define XORI_B2_128_SB(...) XORI_B2_128(v16i8, __VA_ARGS__)
#define XORI_B2_128_SH(...) XORI_B2_128(v8i16, __VA_ARGS__)

#define XORI_B3_128(RTYPE, in0, in1, in2)  \
{                                          \
    XORI_B2_128(RTYPE, in0, in1);          \
    in2 = (RTYPE) XORI_B(in2, 128);        \
}
#define XORI_B3_128_SB(...) XORI_B3_128(v16i8, __VA_ARGS__)

#define XORI_B4_128(RTYPE, in0, in1, in2, in3)  \
{                                               \
    XORI_B2_128(RTYPE, in0, in1);               \
    XORI_B2_128(RTYPE, in2, in3);               \
}
#define XORI_B4_128_UB(...) XORI_B4_128(v16u8, __VA_ARGS__)
#define XORI_B4_128_SB(...) XORI_B4_128(v16i8, __VA_ARGS__)

#define XORI_B5_128(RTYPE, in0, in1, in2, in3, in4)  \
{                                                    \
    XORI_B3_128(RTYPE, in0, in1, in2);               \
    XORI_B2_128(RTYPE, in3, in4);                    \
}
#define XORI_B5_128_SB(...) XORI_B5_128(v16i8, __VA_ARGS__)

#define XORI_B6_128(RTYPE, in0, in1, in2, in3, in4, in5)  \
{                                                         \
    XORI_B4_128(RTYPE, in0, in1, in2, in3);               \
    XORI_B2_128(RTYPE, in4, in5);                         \
}
#define XORI_B6_128_SB(...) XORI_B6_128(v16i8, __VA_ARGS__)

#define XORI_B7_128(RTYPE, in0, in1, in2, in3, in4, in5, in6)  \
{                                                              \
    XORI_B4_128(RTYPE, in0, in1, in2, in3);                    \
    XORI_B3_128(RTYPE, in4, in5, in6);                         \
}
#define XORI_B7_128_SB(...) XORI_B7_128(v16i8, __VA_ARGS__)

#define XORI_B8_128(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7)  \
{                                                                   \
    XORI_B4_128(RTYPE, in0, in1, in2, in3);                         \
    XORI_B4_128(RTYPE, in4, in5, in6, in7);                         \
}
#define XORI_B8_128_SB(...) XORI_B8_128(v16i8, __VA_ARGS__)

/* Description : Addition of signed halfword elements and signed saturation
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed halfword elements from 'in0' are added to signed
                 halfword elements of 'in1'. The result is then signed saturated
                 between halfword data type range
*/
#define ADDS_SH2(RTYPE, in0, in1, in2, in3, out0, out1)       \
{                                                             \
    out0 = (RTYPE) __msa_adds_s_h((v8i16) in0, (v8i16) in1);  \
    out1 = (RTYPE) __msa_adds_s_h((v8i16) in2, (v8i16) in3);  \
}
#define ADDS_SH2_SH(...) ADDS_SH2(v8i16, __VA_ARGS__)

#define ADDS_SH4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    ADDS_SH2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ADDS_SH2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ADDS_SH4_SH(...) ADDS_SH4(v8i16, __VA_ARGS__)

/* Description : Shift left all elements of half-word vector
   Arguments   : Inputs  - in0, in1, shift_val
                 Outputs - in place operation
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is left shifted by 'shift' and
                 the result is written in-place.
*/
#define SLLI_H2(RTYPE, in0, in1, shift_val)  \
{                                            \
    in0 = (RTYPE) SLLI_H(in0, shift_val);    \
    in1 = (RTYPE) SLLI_H(in1, shift_val);    \
}
#define SLLI_H2_SH(...) SLLI_H2(v8i16, __VA_ARGS__)

#define SLLI_H4(RTYPE, in0, in1, in2, in3, shift_val)  \
{                                                      \
    SLLI_H2(RTYPE, in0, in1, shift_val)                \
    SLLI_H2(RTYPE, in2, in3, shift_val)                \
}
#define SLLI_H4_SH(...) SLLI_H4(v8i16, __VA_ARGS__)

/* Description : Arithmetic shift right all elements of half-word vector
   Arguments   : Inputs  - in0, in1, shift_val
                 Outputs - in place operation
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is right shifted by 'shift' and
                 the result is written in-place. 'shift' is a GP variable.
*/
#define SRAI_H2(RTYPE, in0, in1, shift_val)  \
{                                            \
    in0 = (RTYPE) SRAI_H(in0, shift_val);    \
    in1 = (RTYPE) SRAI_H(in1, shift_val);    \
}
#define SRAI_H2_SH(...) SRAI_H2(v8i16, __VA_ARGS__)

/* Description : Arithmetic shift right all elements of word vector
   Arguments   : Inputs  - in0, in1, shift_val
                 Outputs - in place operation
                 Return Type - as per input vector RTYPE
   Details     : Each element of vector 'in0' is right shifted by 'shift' and
                 the result is written in-place. 'shift' is a GP variable.
*/
#define SRAI_W2(RTYPE, in0, in1, shift_val)  \
{                                            \
    in0 = (RTYPE) SRAI_W(in0, shift_val);    \
    in1 = (RTYPE) SRAI_W(in1, shift_val);    \
}
#define SRAI_W2_SW(...) SRAI_W2(v4i32, __VA_ARGS__)

#define SRAI_W4(RTYPE, in0, in1, in2, in3, shift_val)  \
{                                                      \
    SRAI_W2(RTYPE, in0, in1, shift_val);               \
    SRAI_W2(RTYPE, in2, in3, shift_val);               \
}
#define SRAI_W4_SW(...) SRAI_W4(v4i32, __VA_ARGS__)

/* Description : Shift right arithmetic rounded words
   Arguments   : Inputs  - in0, in1, shift
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each element of vector 'in0' is shifted right arithmetically by
                 the number of bits in the corresponding element in the vector
                 'shift'. The last discarded bit is added to shifted value for
                 rounding and the result is written in-place.
                 'shift' is a vector.
*/
#define SRAR_W2(RTYPE, in0, in1, shift)                      \
{                                                            \
    in0 = (RTYPE) __msa_srar_w((v4i32) in0, (v4i32) shift);  \
    in1 = (RTYPE) __msa_srar_w((v4i32) in1, (v4i32) shift);  \
}
#define SRAR_W2_SW(...) SRAR_W2(v4i32, __VA_ARGS__)

#define SRAR_W4(RTYPE, in0, in1, in2, in3, shift)  \
{                                                  \
    SRAR_W2(RTYPE, in0, in1, shift);               \
    SRAR_W2(RTYPE, in2, in3, shift);               \
}
#define SRAR_W4_SW(...) SRAR_W4(v4i32, __VA_ARGS__)

/* Description : Shift right arithmetic rounded (immediate)
   Arguments   : Inputs  - in0, in1, shift
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each element of vector 'in0' is shifted right arithmetically by
                 the value in 'shift'. The last discarded bit is added to the
                 shifted value for rounding and the result is written in-place.
                 'shift' is an immediate value.
*/
#define SRARI_H2(RTYPE, in0, in1, shift)              \
{                                                     \
    in0 = (RTYPE) __msa_srari_h((v8i16) in0, shift);  \
    in1 = (RTYPE) __msa_srari_h((v8i16) in1, shift);  \
}
#define SRARI_H2_UH(...) SRARI_H2(v8u16, __VA_ARGS__)
#define SRARI_H2_SH(...) SRARI_H2(v8i16, __VA_ARGS__)

#define SRARI_H3(RTYPE, in0, in1, in2, shift)         \
{                                                     \
    SRARI_H2(RTYPE, in0, in1, shift);                 \
    in2 = (RTYPE) __msa_srari_h((v8i16) in2, shift);  \
}
#define SRARI_H3_SH(...) SRARI_H3(v8i16, __VA_ARGS__)

#define SRARI_H4(RTYPE, in0, in1, in2, in3, shift)    \
{                                                     \
    SRARI_H2(RTYPE, in0, in1, shift);                 \
    SRARI_H2(RTYPE, in2, in3, shift);                 \
}
#define SRARI_H4_UH(...) SRARI_H4(v8u16, __VA_ARGS__)
#define SRARI_H4_SH(...) SRARI_H4(v8i16, __VA_ARGS__)

#define SRARI_W2(RTYPE, in0, in1, shift)              \
{                                                     \
    in0 = (RTYPE) __msa_srari_w((v4i32) in0, shift);  \
    in1 = (RTYPE) __msa_srari_w((v4i32) in1, shift);  \
}

#define SRARI_W4(RTYPE, in0, in1, in2, in3, shift)  \
{                                                   \
    SRARI_W2(RTYPE, in0, in1, shift);               \
    SRARI_W2(RTYPE, in2, in3, shift);               \
}
#define SRARI_W4_SW(...) SRARI_W4(v4i32, __VA_ARGS__)

/* Description : Addition of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element in 'in0' is added to 'in1' and result is written
                 to 'out0'.
*/
#define ADD2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 + in1;                         \
    out1 = in2 + in3;                         \
}
#define ADD4(in0, in1, in2, in3, in4, in5, in6, in7,  \
             out0, out1, out2, out3)                  \
{                                                     \
    ADD2(in0, in1, in2, in3, out0, out1);             \
    ADD2(in4, in5, in6, in7, out2, out3);             \
}

/* Description : Subtraction of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element in 'in1' is subtracted from 'in0' and result is
                 written to 'out0'.
*/
#define SUB2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 - in1;                         \
    out1 = in2 - in3;                         \
}
#define SUB4(in0, in1, in2, in3, in4, in5, in6, in7,  \
             out0, out1, out2, out3)                  \
{                                                     \
    out0 = in0 - in1;                                 \
    out1 = in2 - in3;                                 \
    out2 = in4 - in5;                                 \
    out3 = in6 - in7;                                 \
}

/* Description : Sign extend byte elements from right half of the vector
   Arguments   : Input  - in    (byte vector)
                 Output - out   (sign extended halfword vector)
                 Return Type - signed halfword
   Details     : Sign bit of byte elements from input vector 'in' is
                 extracted and interleaved with same vector 'in0' to generate
                 8 halfword elements keeping sign intact
*/
#define UNPCK_R_SB_SH(in, out)                       \
{                                                    \
    v16i8 sign_m;                                    \
                                                     \
    sign_m = CLTI_S_B(in, 0);                        \
    out = (v8i16) __msa_ilvr_b(sign_m, (v16i8) in);  \
}

/* Description : Sign extend byte elements from input vector and return
                 halfword results in pair of vectors
   Arguments   : Input   - in           (byte vector)
                 Outputs - out0, out1   (sign extended halfword vectors)
                 Return Type - signed halfword
   Details     : Sign bit of byte elements from input vector 'in' is
                 extracted and interleaved right with same vector 'in0' to
                 generate 8 signed halfword elements in 'out0'
                 Then interleaved left with same vector 'in0' to
                 generate 8 signed halfword elements in 'out1'
*/
#define UNPCK_SB_SH(in, out0, out1)         \
{                                           \
    v16i8 tmp_m;                            \
                                            \
    tmp_m = CLTI_S_B(in, 0);                \
    ILVRL_B2_SH(tmp_m, in, out0, out1);     \
}

/* Description : Zero extend unsigned byte elements to halfword elements
   Arguments   : Input   - in          (unsigned byte vector)
                 Outputs - out0, out1  (unsigned  halfword vectors)
                 Return Type - signed halfword
   Details     : Zero extended right half of vector is returned in 'out0'
                 Zero extended left half of vector is returned in 'out1'
*/
#define UNPCK_UB_SH(in, out0, out1)       \
{                                         \
    v16i8 zero_m = { 0 };                 \
                                          \
    ILVRL_B2_SH(zero_m, in, out0, out1);  \
}

/* Description : Sign extend halfword elements from input vector and return
                 the result in pair of vectors
   Arguments   : Input   - in            (halfword vector)
                 Outputs - out0, out1   (sign extended word vectors)
                 Return Type - signed word
   Details     : Sign bit of halfword elements from input vector 'in' is
                 extracted and interleaved right with same vector 'in0' to
                 generate 4 signed word elements in 'out0'
                 Then interleaved left with same vector 'in0' to
                 generate 4 signed word elements in 'out1'
*/
#define UNPCK_SH_SW(in, out0, out1)      \
{                                        \
    v8i16 tmp_m;                         \
                                         \
    tmp_m = CLTI_S_H((v8i16) in, 0);     \
    ILVRL_H2_SW(tmp_m, in, out0, out1);  \
}

/* Description : Butterfly of 4 input vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
   Details     : Butterfly operation
*/
#define BUTTERFLY_4(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                \
    out0 = in0 + in3;                                            \
    out1 = in1 + in2;                                            \
                                                                 \
    out2 = in1 - in2;                                            \
    out3 = in0 - in3;                                            \
}

/* Description : Transpose input 4x4 byte block
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
                 Return Type - unsigned byte
*/
#define TRANSPOSE4x4_UB_UB(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                       \
    v16i8 zero_m = { 0 };                                               \
    v16i8 s0_m, s1_m, s2_m, s3_m;                                       \
                                                                        \
    ILVR_D2_SB(in1, in0, in3, in2, s0_m, s1_m);                         \
    ILVRL_B2_SB(s1_m, s0_m, s2_m, s3_m);                                \
                                                                        \
    out0 = (v16u8) __msa_ilvr_b(s3_m, s2_m);                            \
    out1 = (v16u8) __msa_sldi_b(zero_m, (v16i8) out0, 4);               \
    out2 = (v16u8) __msa_sldi_b(zero_m, (v16i8) out1, 4);               \
    out3 = (v16u8) __msa_sldi_b(zero_m, (v16i8) out2, 4);               \
}

/* Description : Transpose input 4x8 byte block
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - as per RTYPE
*/
#define TRANSPOSE4x8_UB(RTYPE, in0, in1, in2, in3, out0, out1, \
                        out2, out3, out4, out5, out6, out7)    \
{                                                              \
    v16i8 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                      \
                                                               \
    ILVR_B2_SB(in2, in0, in3, in1, tmp0_m, tmp1_m);            \
    ILVRL_B2_SB(tmp1_m, tmp0_m, tmp2_m, tmp3_m);               \
    ILVRL_W2(RTYPE, zero, tmp2_m, out0, out2);                 \
    ILVRL_W2(RTYPE, zero, tmp3_m, out4, out6);                 \
    SLDI_B2_0(RTYPE, out0, out2, out1, out3, 8);               \
    SLDI_B2_0(RTYPE, out4, out6, out5, out7, 8);               \
}
#define TRANSPOSE4x8_UB_UH(...) TRANSPOSE4x8_UB(v8u16, __VA_ARGS__)

/* Description : Transpose input 8x4 byte block into 4x8
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3
                 Return Type - as per RTYPE
*/
#define TRANSPOSE8x4_UB(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                        out0, out1, out2, out3)                         \
{                                                                       \
    v16i8 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
                                                                        \
    ILVEV_W2_SB(in0, in4, in1, in5, tmp0_m, tmp1_m);                    \
    tmp2_m = __msa_ilvr_b(tmp1_m, tmp0_m);                              \
    ILVEV_W2_SB(in2, in6, in3, in7, tmp0_m, tmp1_m);                    \
                                                                        \
    tmp3_m = __msa_ilvr_b(tmp1_m, tmp0_m);                              \
    ILVRL_H2_SB(tmp3_m, tmp2_m, tmp0_m, tmp1_m);                        \
                                                                        \
    ILVRL_W2(RTYPE, tmp1_m, tmp0_m, out0, out2);                        \
    out1 = (RTYPE) __msa_ilvl_d((v2i64) out2, (v2i64) out0);            \
    out3 = (RTYPE) __msa_ilvl_d((v2i64) out0, (v2i64) out2);            \
}
#define TRANSPOSE8x4_UB_UB(...) TRANSPOSE8x4_UB(v16u8, __VA_ARGS__)

/* Description : Transpose 8x4 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - signed halfword
*/
#define TRANSPOSE8X4_SH_SH(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                       \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
                                                                        \
    ILVR_H2_SH(in1, in0, in3, in2, tmp0_m, tmp1_m);                     \
    ILVL_H2_SH(in1, in0, in3, in2, tmp2_m, tmp3_m);                     \
    ILVR_W2_SH(tmp1_m, tmp0_m, tmp3_m, tmp2_m, out0, out2);             \
    ILVL_W2_SH(tmp1_m, tmp0_m, tmp3_m, tmp2_m, out1, out3);             \
}

/* Description : Transpose 8x8 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - as per RTYPE
*/
#define TRANSPOSE8x8_H(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,   \
                       out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                       \
    v8i16 s0_m, s1_m;                                                   \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
    v8i16 tmp4_m, tmp5_m, tmp6_m, tmp7_m;                               \
                                                                        \
    ILVR_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp0_m, tmp1_m);                            \
    ILVL_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp2_m, tmp3_m);                            \
    ILVR_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp4_m, tmp5_m);                            \
    ILVL_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp6_m, tmp7_m);                            \
    PCKEV_D4(RTYPE, tmp0_m, tmp4_m, tmp1_m, tmp5_m, tmp2_m, tmp6_m,     \
             tmp3_m, tmp7_m, out0, out2, out4, out6);                   \
    PCKOD_D2(RTYPE, tmp0_m, tmp4_m, tmp1_m, tmp5_m, out1, out3);        \
    PCKOD_D2(RTYPE, tmp2_m, tmp6_m, tmp3_m, tmp7_m, out5, out7);        \
}
#define TRANSPOSE8x8_SH_SH(...) TRANSPOSE8x8_H(v8i16, __VA_ARGS__)

/* Description : Transpose 4x4 block with word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
                 Return Type - as per RTYPE
*/
#define TRANSPOSE4x4_SW_SW(in0, in1, in2, in3,      \
                           out0, out1, out2, out3)  \
{                                                   \
    v4i32 s0_m, s1_m, s2_m, s3_m;                   \
                                                    \
    ILVRL_W2_SW(in1, in0, s0_m, s1_m);              \
    ILVRL_W2_SW(in3, in2, s2_m, s3_m);              \
    ILVRL_D2_SW(s2_m, s0_m, out0, out1);            \
    ILVRL_D2_SW(s3_m, s1_m, out2, out3);            \
}

/* Description : Pack even elements of input vectors & xor with 128
   Arguments   : Inputs - in0, in1
                 Output - out_m
                 Return Type - unsigned byte
   Details     : Signed byte even elements from 'in0' and 'in1' are packed
                 together in one vector and the resulting vector is xor'ed with
                 128 to shift the range from signed to unsigned byte
*/
#define PCKEV_XORI128_UB(in0, in1)                            \
( {                                                           \
    v16u8 out_m;                                              \
    out_m = (v16u8) __msa_pckev_b((v16i8) in1, (v16i8) in0);  \
    out_m = XORI_B(out_m, 128);                               \
    out_m;                                                    \
} )
#endif  /* __IHEVC_MACROS_MSA_H__ */
