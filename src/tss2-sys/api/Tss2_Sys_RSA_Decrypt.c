/* SPDX-License-Identifier: BSD-2-Clause */
/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
 * All rights reserved.
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h" // IWYU pragma: keep
#endif

#include "sysapi_util.h"      // for _TSS2_SYS_CONTEXT_BLOB, syscontext_cast
#include "tss2_common.h"      // for TSS2_RC, TSS2_SYS_RC_BAD_REFERENCE
#include "tss2_mu.h"          // for Tss2_MU_UINT16_Marshal, Tss2_MU_TPM2B_D...
#include "tss2_sys.h"         // for TSS2_SYS_CONTEXT, TSS2L_SYS_AUTH_COMMAND
#include "tss2_tpm2_types.h"  // for TPM2B_PUBLIC_KEY_RSA, TPM2B_DATA, TPMI_...

TSS2_RC Tss2_Sys_RSA_Decrypt_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT keyHandle,
    const TPM2B_PUBLIC_KEY_RSA *cipherText,
    const TPMT_RSA_DECRYPT *inScheme,
    const TPM2B_DATA *label)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx || !inScheme)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_RSA_Decrypt);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(keyHandle, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    if (!cipherText) {
        ctx->decryptNull = 1;

        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);
    } else {

        rval = Tss2_MU_TPM2B_PUBLIC_KEY_RSA_Marshal(cipherText,
                                                    ctx->cmdBuffer,
                                                    ctx->maxCmdSize,
                                                    &ctx->nextData);
    }

    if (rval)
        return rval;

    rval = Tss2_MU_TPMT_RSA_DECRYPT_Marshal(inScheme, ctx->cmdBuffer,
                                            ctx->maxCmdSize,
                                            &ctx->nextData);
    if (rval)
        return rval;

    if (!label) {
        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);

    } else {

        rval = Tss2_MU_TPM2B_DATA_Marshal(label, ctx->cmdBuffer,
                                          ctx->maxCmdSize,
                                          &ctx->nextData);
    }

    if (rval)
        return rval;

    ctx->decryptAllowed = 1;
    ctx->encryptAllowed = 1;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_RSA_Decrypt_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPM2B_PUBLIC_KEY_RSA *message)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(ctx);
    if (rval)
        return rval;

    return Tss2_MU_TPM2B_PUBLIC_KEY_RSA_Unmarshal(ctx->cmdBuffer,
                                                  ctx->maxCmdSize,
                                                  &ctx->nextData, message);
}

TSS2_RC Tss2_Sys_RSA_Decrypt(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT keyHandle,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    const TPM2B_PUBLIC_KEY_RSA *cipherText,
    const TPMT_RSA_DECRYPT *inScheme,
    const TPM2B_DATA *label,
    TPM2B_PUBLIC_KEY_RSA *message,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!inScheme)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_RSA_Decrypt_Prepare(sysContext, keyHandle, cipherText,
                                        inScheme, label);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_RSA_Decrypt_Complete(sysContext, message);
}
