libhevcd_inc_dir_mips64 +=  $(LOCAL_PATH)/decoder/mips
libhevcd_inc_dir_mips64 +=  $(LOCAL_PATH)/common/mips

libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_function_selector.c
libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_function_selector_mips_generic.c

ifeq ($(ARCH_MIPS_HAS_MSA),true)
libhevcd_cflags_mips64  += -mfp64 -mmsa
libhevcd_cflags_mips64  += -DDEFAULT_ARCH=D_ARCH_MIPS_MSA

libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_function_selector_msa.c
libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_itrans_recon_dc_msa.c
libhevcd_srcs_c_mips64  +=  decoder/mips/ihevcd_fmt_conv_msa.c

libhevcd_srcs_c_mips64  +=  common/mips/ihevc_inter_pred_luma_filters_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_inter_pred_chroma_filters_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_intra_pred_filters_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_padding_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_mem_fns_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_itrans_recon_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_recon_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_weighted_pred_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_deblk_edge_filter_msa.c
libhevcd_srcs_c_mips64  +=  common/mips/ihevc_sao_msa.c
else
libhevcd_cflags_mips64  += -DDISABLE_MSA -DDEFAULT_ARCH=D_ARCH_MIPS_NOMSA
endif

LOCAL_SRC_FILES_mips64 += $(libhevcd_srcs_c_mips64)
LOCAL_C_INCLUDES_mips64 += $(libhevcd_inc_dir_mips64)
LOCAL_CFLAGS_mips64 += $(libhevcd_cflags_mips64)

