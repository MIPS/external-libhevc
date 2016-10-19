libhevcd_inc_dir_mips   +=  $(LOCAL_PATH)/decoder/mips
libhevcd_inc_dir_mips   +=  $(LOCAL_PATH)/common/mips

libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_function_selector.c
libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_function_selector_mips_generic.c

ifeq ($(ARCH_MIPS_HAS_MSA),true)
libhevcd_cflags_mips    += -DDEFAULT_ARCH=D_ARCH_MIPS_MSA

libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_function_selector_msa.c
libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_itrans_recon_dc_msa.c
libhevcd_srcs_c_mips    +=  decoder/mips/ihevcd_fmt_conv_msa.c

libhevcd_srcs_c_mips    +=  common/mips/ihevc_inter_pred_luma_filters_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_inter_pred_chroma_filters_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_intra_pred_filters_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_padding_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_mem_fns_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_itrans_recon_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_recon_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_weighted_pred_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_deblk_edge_filter_msa.c
libhevcd_srcs_c_mips    +=  common/mips/ihevc_sao_msa.c
else
libhevcd_cflags_mips    += -DDISABLE_MSA -DDEFAULT_ARCH=D_ARCH_MIPS_NOMSA
endif

LOCAL_SRC_FILES_mips += $(libhevcd_srcs_c_mips)
LOCAL_C_INCLUDES_mips += $(libhevcd_inc_dir_mips)
LOCAL_CFLAGS_mips += $(libhevcd_cflags_mips)



