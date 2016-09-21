/******************************************************************************
*
* Copyright (C) 2012 Ittiam Systems Pvt Ltd, Bangalore
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
******************************************************************************/
/**
*******************************************************************************
* @file
*  ihevcd_function_selector.c
*
* @brief
*  Contains functions to initialize function pointers used in hevc
*
* @author
*  Naveen
*
* @par List of Functions:
* @remarks
*  None
*
*******************************************************************************
*/
/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
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

void ihevcd_init_function_ptr_mips_generic(codec_t *ps_codec);
void ihevcd_init_function_ptr_mips_32(codec_t *ps_codec);
void ihevcd_init_function_ptr_msa(codec_t *ps_codec);

void ihevcd_init_function_ptr(void *pv_codec)
{
    codec_t *ps_codec = (codec_t *)pv_codec;

    ihevcd_init_function_ptr_mips_generic(ps_codec);

    switch(ps_codec->e_processor_arch)
    {
#if ENABLE_MIPS32_SIMD
        case ARCH_MIPS_32:
            ihevcd_init_function_ptr_mips_32(ps_codec);
            break;
#endif
#ifndef DISABLE_MSA
        case ARCH_MIPS_MSA:
            ihevcd_init_function_ptr_msa(ps_codec);
        break;
#endif
        case ARCH_MIPS_GENERIC:
        default:
            ihevcd_init_function_ptr_mips_generic(ps_codec);
            break;
    }
}

void ihevcd_init_arch(void *pv_codec)
{
    codec_t *ps_codec = (codec_t *)pv_codec;

#ifdef DEFAULT_ARCH
#if DEFAULT_ARCH == D_ARCH_MIPS_NOMSA
    ps_codec->e_processor_arch = ARCH_MIPS_GENERIC;
#elif DEFAULT_ARCH == D_ARCH_MIPS_MSA
    ps_codec->e_processor_arch = ARCH_MIPS_MSA;
#elif DEFAULT_ARCH == ENABLE_MIPS32_SIMD
    ps_codec->e_processor_arch = ARCH_MIPS_32;
#endif
#else
    ps_codec->e_processor_arch = ARCH_MIPS_MSA;
#endif
}
