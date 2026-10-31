#ifndef __VISE_BIND_INFO_H__
#define __VISE_BIND_INFO_H__
#include "sample_common_media.h"
#include "dsp/fh_system_mpi.h"

FH_SINT32 vise_bind_enc(FH_SINT32 vise_id, FH_SINT32 enc_chn);

FH_SINT32 vise_bind_jpeg(FH_UINT32 vise_id, FH_UINT32 jpeg_chn);

#endif // __VISE_BIND_INFO_H__
