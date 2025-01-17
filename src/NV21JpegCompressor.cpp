/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains implementation of a class NV21JpegCompressor that encapsulates a
 * converter between NV21, and JPEG formats.
 */

// #define LOG_NDEBUG 0
#define LOG_TAG "VirtualCamera_JPEG"
#include <log/log.h>
#include <assert.h>
#include <dlfcn.h>
#include "NV21JpegCompressor.h"

namespace android {

void *NV21JpegCompressor::mDl = NULL;

static void *getSymbol(void *dl, const char *signature) {
    void *res = dlsym(dl, signature);
    assert(res != NULL);

    return res;
}

typedef void (*InitFunc)(JpegStub *stub);
typedef void (*CleanupFunc)(JpegStub *stub);
typedef int (*CompressFunc)(JpegStub *stub, const void *image, int width, int height, int quality,
                            ExifData *exifData);
typedef void (*GetCompressedImageFunc)(JpegStub *stub, void *buff);
typedef size_t (*GetCompressedSizeFunc)(JpegStub *stub);

NV21JpegCompressor::NV21JpegCompressor() {
    const char dlName[] = "/system/vendor/lib64/hw/camera.celadon.jpeg.so";
    if (!mDl) {
        mDl = dlopen(dlName, RTLD_NOW);
    }
    if (mDl) {
        InitFunc f = (InitFunc)getSymbol(mDl, "JpegStub_init");
        if (f)
            (*f)(&mStub);
        else
            ALOGE("%s: Fatal error: getSymbol(JpegStub_init) failed", __func__);
    } else {
        ALOGE("%s: Fatal error: dlopen(%s) failed", __func__, dlName);
    }
}

NV21JpegCompressor::~NV21JpegCompressor() {
    CleanupFunc f = (CleanupFunc)getSymbol(mDl, "JpegStub_cleanup");
    if (f)
        (*f)(&mStub);
    else
        ALOGE("%s: Fatal error: getSymbol(JpegStub_cleanup) failed", __func__);
}

/****************************************************************************
 * Public API
 ***************************************************************************/

status_t NV21JpegCompressor::compressRawImage(const void *image, int width, int height, int quality,
                                              ExifData *exifData) {
    CompressFunc f = (CompressFunc)getSymbol(mDl, "JpegStub_compress");
    if (!f) {
        ALOGE("%s: Fatal error: getSymbol(JpegStub_compress) failed", __func__);
        return -EINVAL;
    }

    return (status_t)(*f)(&mStub, image, width, height, quality, exifData);
}

size_t NV21JpegCompressor::getCompressedSize() {
    GetCompressedSizeFunc f = (GetCompressedSizeFunc)getSymbol(mDl, "JpegStub_getCompressedSize");
    if (!f) {
        ALOGE("%s: Fatal error: getSymbol(JpegStub_getCompressedSize) failed", __func__);
        return 0;
    }
    return (*f)(&mStub);
}

void NV21JpegCompressor::getCompressedImage(void *buff) {
    GetCompressedImageFunc f =
        (GetCompressedImageFunc)getSymbol(mDl, "JpegStub_getCompressedImage");
    if (!f) {
        ALOGE("%s: Fatal error: getSymbol(JpegStub_getCompressedImage) failed", __func__);
        return;
    }
    (*f)(&mStub, buff);
}

}; /* namespace android */
