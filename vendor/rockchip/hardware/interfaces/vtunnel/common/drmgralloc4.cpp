/*
 * Copyright (C) 2019-2020 RockChip Limited. All rights reserved.
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

/*  --------------------------------------------------------------------------------------------------------
 *  File:   platform_gralloc4.c
 *
 *  Desc:   具体实现 platform_gralloc4.h 定义的接口, 基于 IMapper 4.0.
 *
 *          -----------------------------------------------------------------------------------
 *          < 习语 和 缩略语 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 内部模块 or 对象的层次结构 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 功能实现的基本流程 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 关键标识符 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 本模块实现依赖的外部模块 > :
 *              ...
 *          -----------------------------------------------------------------------------------
 *  Note:
 *
 *  Author: ChenZhen
 *
 *  Log:
 *          init
	----Fri Aug 28 10:17:46 2020
 */


/* ---------------------------------------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------------------------------------
 */

#define LOG_TAG "drmgralloc4"
#define ENABLE_DEBUG_LOG
#include <custom_log.h>

#include <inttypes.h>

#include <sync/sync.h>

#include <drm_fourcc.h>

#include "drmgralloc4.h"

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <gralloctypes/Gralloc4.h>
// #include <aidl/arm/graphics/ArmMetadataType.h>

//#include "rockchip/drmtype.h"
//#include "rockchip/utils/drmdebug.h"

using android::hardware::graphics::mapper::V4_0::Error;
using android::hardware::graphics::mapper::V4_0::IMapper;
using android::hardware::hidl_vec;

using android::gralloc4::MetadataType_PlaneLayouts;
using android::gralloc4::decodePlaneLayouts;
using android::gralloc4::MetadataType_Usage;
using android::gralloc4::decodeUsage;
using android::gralloc4::MetadataType_PlaneLayouts;
using android::gralloc4::decodePlaneLayouts;
using android::gralloc4::MetadataType_PixelFormatFourCC;
using android::gralloc4::decodePixelFormatFourCC;
using android::gralloc4::MetadataType_PixelFormatModifier;
using android::gralloc4::decodePixelFormatModifier;
using android::gralloc4::MetadataType_PixelFormatRequested;
using android::gralloc4::decodePixelFormatRequested;
using android::gralloc4::MetadataType_AllocationSize;
using android::gralloc4::decodeAllocationSize;
using android::gralloc4::MetadataType_LayerCount;
using android::gralloc4::decodeLayerCount;
using android::gralloc4::MetadataType_Dataspace;
using android::gralloc4::decodeDataspace;
using android::gralloc4::MetadataType_Crop;
using android::gralloc4::decodeCrop;
using android::gralloc4::MetadataType_Width;
using android::gralloc4::decodeWidth;
using android::gralloc4::MetadataType_Height;
using android::gralloc4::decodeHeight;
using android::gralloc4::MetadataType_Name;
using android::gralloc4::decodeName;
using android::gralloc4::MetadataType_BufferId;
using android::gralloc4::decodeBufferId;

using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::ExtendableType;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::Rect;
using android::hardware::graphics::common::V1_2::PixelFormat;


#define GRALLOC_ARM_METADATA_TYPE_NAME "arm.graphics.ArmMetadataType"
const static IMapper::MetadataType ArmMetadataType_PLANE_FDS
{
	GRALLOC_ARM_METADATA_TYPE_NAME,
	// static_cast<int64_t>(aidl::arm::graphics::ArmMetadataType::PLANE_FDS)
    1   // 就是上面的 'PLANE_FDS'
};

#ifndef DRM_FORMAT_NV15
#define DRM_FORMAT_NV15 fourcc_code('N', 'V', '1', '5') /* 2x2 subsampled Cr:Cb plane */
#endif

/* ---------------------------------------------------------------------------------------------------------
 * External Function Prototypes (referenced in this file)
 * ---------------------------------------------------------------------------------------------------------
 */

/* ---------------------------------------------------------------------------------------------------------
 * Local Macros
 * ---------------------------------------------------------------------------------------------------------
 */

namespace gralloc4 {

/*
 * 闫孝军反馈“4.19内核里面没这个format，要从linux 主线 5.2 以后里面反向porting 回来。4.19和5.2差别很大。
 * 反向porting有很多冲突要解决”，所以从上层HWC模块去规避这个问题，HWC实现如下：
 * 1.格式转换：
 *   DRM_FORMAT_YUV420_10BIT => DRM_FORMAT_NV12_10
 *   DRM_FORMAT_YUV420_8BIT  => DRM_FORMAT_NV12
 *   DRM_FORMAT_YUYV         => DRM_FORMAT_NV16
 *
 * 2.Byte Stride 转换：
 *   DRM_FORMAT_NV12_10 / DRM_FORMAT_NV12:
 *       Byte stride = Byte stride / 1.5
 *
 *   DRM_FORMAT_NV16:
 *       Byte stride = Byte stride / 2
 *
 * 按上述实现，可以在当前版本保证视频送显正常，提供开关 WORKROUND_FOR_VOP2_DRIVER
 */
static int DrmVersion=0;
void set_drm_version(int version){
  DrmVersion = version;
}

// vendor.hwc.disable_gralloc4_use_vir_height = true
bool use_vir_height = true;
void init_env_property(){
  char value[1024];
  //property_get("vendor.hwc.disable_gralloc4_use_vir_height", value, "0");
  //use_vir_height = (atoi(value) == 0);
}

/* ---------------------------------------------------------------------------------------------------------
 * Local Typedefs
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 * Local Function Prototypes
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 * Local Variables
 * ---------------------------------------------------------------------------------------------------------
 */
static constexpr Error kTransactionError = Error::NO_RESOURCES;

/* ---------------------------------------------------------------------------------------------------------
 * Global Variables
 * ---------------------------------------------------------------------------------------------------------
 */

/* ---------------------------------------------------------------------------------------------------------
 * Local Functions Implementation
 * ---------------------------------------------------------------------------------------------------------
 */

static IMapper &get_service()
{
    static android::sp<IMapper> cached_service = IMapper::getService();
    return *cached_service;
}

template <typename T>
static int get_metadata(IMapper &mapper, buffer_handle_t handle, IMapper::MetadataType type,
                        android::status_t (*decode)(const hidl_vec<uint8_t> &, T *), T *value)
{
	void *handle_arg = const_cast<native_handle_t *>(handle);
	assert(handle_arg);
	assert(value);
	assert(decode);

	int err = 0;
	mapper.get(handle_arg, type, [&err, value, decode](Error error, const hidl_vec<uint8_t> &metadata)
	            {
		            if (error != Error::NONE)
		            {
			            err = android::BAD_VALUE;
			            return;
		            }
		            err = decode(metadata, value);
		        });
	return err;
}

android::status_t static decodeArmPlaneFds(const hidl_vec<uint8_t>& input, std::vector<int64_t>* fds)
{
    assert (fds != nullptr);
    int64_t size = 0;

    memcpy(&size, input.data(), sizeof(int64_t));
    if (size < 0)
    {
        return android::BAD_VALUE;
    }

    fds->resize(size);

    const uint8_t *tmp = input.data() + sizeof(int64_t);
    memcpy(fds->data(), tmp, sizeof(int64_t) * size);

    return android::NO_ERROR;
}

/* ---------------------------------------------------------------------------------------------------------
 * Global Functions Implementation
 * ---------------------------------------------------------------------------------------------------------
 */

uint64_t get_format_modifier(buffer_handle_t handle)
{

  auto &mapper = get_service();
  uint64_t modifier;

  /* 获取 format_modifier. */
  int err = get_metadata(mapper, handle, MetadataType_PixelFormatModifier, decodePixelFormatModifier, &modifier);
  assert(err == android::NO_ERROR);

  return modifier;
}

uint32_t convertToNV12(uint32_t origin_fourcc){
    switch(origin_fourcc){
      case DRM_FORMAT_YUV420_10BIT:
        ALOGD("%s,line=%d ,vop driver workround: fourcc %c%c%c%c => %c%c%c%c",__FUNCTION__,__LINE__,
                 origin_fourcc, origin_fourcc >> 8, origin_fourcc >> 16, origin_fourcc >> 24,
                 DRM_FORMAT_NV12_10, DRM_FORMAT_NV12_10 >> 8, DRM_FORMAT_NV12_10 >> 16, DRM_FORMAT_NV12_10 >> 24);
        return DRM_FORMAT_NV12_10;
      case DRM_FORMAT_YUV420_8BIT:
        ALOGD("%s,line=%d ,vop driver workround: fourcc %c%c%c%c => %c%c%c%c",__FUNCTION__,__LINE__,
                 origin_fourcc, origin_fourcc >> 8, origin_fourcc >> 16, origin_fourcc >> 24,
                 DRM_FORMAT_NV12, DRM_FORMAT_NV12 >> 8, DRM_FORMAT_NV12 >> 16, DRM_FORMAT_NV12 >> 24);
        return DRM_FORMAT_NV12;
      case DRM_FORMAT_YUYV:
        ALOGD("%s,line=%d ,vop driver workround: fourcc %c%c%c%c => %c%c%c%c",__FUNCTION__,__LINE__,
                 origin_fourcc, origin_fourcc >> 8, origin_fourcc >> 16, origin_fourcc >> 24,
                 DRM_FORMAT_NV16, DRM_FORMAT_NV16 >> 8, DRM_FORMAT_NV16 >> 16, DRM_FORMAT_NV16 >> 24);
        return DRM_FORMAT_NV16;
      default:
        return origin_fourcc;
    }
    return origin_fourcc;
}

uint32_t get_fourcc_format(buffer_handle_t handle)
{
    auto &mapper = get_service();
    uint32_t fourcc;

    /* 获取 format_fourcc. */
    int err = get_metadata(mapper, handle, MetadataType_PixelFormatFourCC, decodePixelFormatFourCC, &fourcc);
    assert(err == android::NO_ERROR);

    // DrmVersion:
    // 3.0.0 = Kernel 5.10
    // 2.0.0 = Kernel 4.19
    if(DrmVersion == 3)
      return fourcc;
    else
      return convertToNV12(fourcc);
}

int get_width(buffer_handle_t handle, uint64_t* width)
{
    auto &mapper = get_service();

    int err = get_metadata(mapper, handle, MetadataType_Width, decodeWidth, width);
    if (err != android::OK)
    {
        E("err : %d", err);
    }

    return err;
}

int get_height(buffer_handle_t handle, uint64_t* height)
{
    auto &mapper = get_service();
    int err = -1;
    err = get_metadata(mapper, handle, MetadataType_Height, decodeHeight, height);
    if (err != android::OK)
    {
        E("err : %d", err);
    }

    return err;
}


int get_height_stride(buffer_handle_t handle, uint64_t* height_stride)
{
    auto &mapper = get_service();
    int err = -1;
    std::vector<PlaneLayout> layouts;
    err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
    if (err != android::OK || layouts.size() < 1)
    {
        E("Failed to get plane layouts. err : %d", err);
        return err;
    }

    if ( layouts.size() > 1 )
    {
        // W("it's not reasonable to get global pixel_stride of buffer with planes more than 1.");
    }

    *height_stride = layouts[0].heightInSamples;
    return err;
}

int get_bit_per_pixel(buffer_handle_t handle, int* bit_per_pixel)
{
    auto &mapper = get_service();
    std::vector<PlaneLayout> layouts;
    int format_requested;

    int err = get_format_requested(handle, &format_requested);
    if (err != android::OK )
    {
        E("err : %d", err);
        return err;
    }

    err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
    if (err != android::OK || layouts.size() < 1)
    {
        E("Failed to get plane layouts. err : %d", err);
        return err;
    }

    if ( layouts.size() > 1 )
    {
        // W("it's not reasonable to get global pixel_stride of buffer with planes more than 1.");
    }

    *bit_per_pixel = (layouts[0].sampleIncrementInBits);

    return err;
}


int get_pixel_stride(buffer_handle_t handle, int* pixel_stride)
{
    int byte_stride = 0;

    int err = get_byte_stride(handle, &byte_stride);
    if (err != android::OK )
    {
        E("err : %d", err);
        return err;
    }

    int bit_per_pixel = 0;
    err = get_bit_per_pixel(handle, &bit_per_pixel);
    if (err != android::OK )
    {
        E("err : %d", err);
        return err;
    }

    *pixel_stride = byte_stride * 8 / bit_per_pixel;
    return err;
}

int get_byte_stride(buffer_handle_t handle, int* byte_stride)
{
    auto &mapper = get_service();
    std::vector<PlaneLayout> layouts;
    int format_requested;

    int err = get_format_requested(handle, &format_requested);
    if (err != android::OK )
    {
        E("err : %d", err);
        return err;
    }

    /* 若 'format_requested' "不是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    if ( format_requested != HAL_PIXEL_FORMAT_YCrCb_NV12_10 )
    {
        err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
        if (err != android::OK || layouts.size() < 1)
        {
            E("Failed to get plane layouts. err : %d", err);
            return err;
        }

        if ( layouts.size() > 1 )
        {
            // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
        }
        *byte_stride = (layouts[0].strideInBytes);
    }
    /* 否则, 即 'format_requested' "是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    else
    {
        uint32_t fourcc_format = get_fourcc_format(handle);
        // RK3588 mali 支持NV15格式，故 byte_stride采用正确的值
        if(fourcc_format == DRM_FORMAT_NV15){
            err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
            if (err != android::OK || layouts.size() < 1)
            {
                E("Failed to get plane layouts. err : %d", err);
                return err;
            }

            if ( layouts.size() > 1 )
            {
                // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
            }
            *byte_stride = (layouts[0].strideInBytes);
        }
        // 对于 fourcc不为 DRM_FORMAT_NV15 的情况，认为 Mali不支持 NV15格式,采用 width 作为 byte_stride.
        else
        {

            uint64_t width;

            err = get_width(handle, &width);
            if (err != android::OK )
            {
                E("err : %d", err);
                return err;
            }

            // .KP : from CSY : 分配 rk_video_decoder 输出 buffers 时, 要求的 byte_stride of buffer in NV12_10, 已经通过 width 传入.
            *byte_stride = (int)width;
        }
    }

    return err;
}


int get_byte_stride_workround(buffer_handle_t handle, int* byte_stride)
{
    auto &mapper = get_service();
    std::vector<PlaneLayout> layouts;
    int format_requested;

    int err = get_format_requested(handle, &format_requested);
    if (err != android::OK )
    {
        E("err : %d", err);
        return err;
    }

    /* 若 'format_requested' "不是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    if ( format_requested != HAL_PIXEL_FORMAT_YCrCb_NV12_10 )
    {
        err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
        if (err != android::OK || layouts.size() < 1)
        {
            E("Failed to get plane layouts. err : %d", err);
            return err;
        }

        if ( layouts.size() > 1 )
        {
            // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
        }

        // DrmVersion:
        // 3.0.0 = Kernel 5.10
        // 2.0.0 = Kernel 4.19
        if(DrmVersion == 3){
            *byte_stride = (layouts[0].strideInBytes);
        }else{
            if(format_requested == HAL_PIXEL_FORMAT_YUV420_8BIT_I
                || format_requested == HAL_PIXEL_FORMAT_YUV420_10BIT_I
                || format_requested == HAL_PIXEL_FORMAT_Y210){
                ALOGD("%s,line=%d ,vop driver workround: byte stride %" PRIi64 " => %" PRIi64,
                          __FUNCTION__,__LINE__,(layouts[0].strideInBytes),(layouts[0].strideInBytes) * 2 / 3);
                *byte_stride = (layouts[0].strideInBytes) * 2 / 3;
            }else if(format_requested == HAL_PIXEL_FORMAT_YCBCR_422_I){
                ALOGD("%s,line=%d ,vop driver workround: byte stride %" PRIi64 " => %" PRIi64,
                          __FUNCTION__,__LINE__,(layouts[0].strideInBytes),(layouts[0].strideInBytes) / 2);
                *byte_stride = (layouts[0].strideInBytes) / 2;
            }else{
                *byte_stride = (layouts[0].strideInBytes);
            }
        }
    }
    /* 否则, 即 'format_requested' "是" HAL_PIXEL_FORMAT_YCrCb_NV12_10, 则 ... */
    else
    {
        uint32_t fourcc_format = get_fourcc_format(handle);
        // RK3588 mali 支持NV15格式，故 byte_stride采用正确的值
        if(fourcc_format == DRM_FORMAT_NV15){
            err = get_metadata(mapper, handle, MetadataType_PlaneLayouts, decodePlaneLayouts, &layouts);
            if (err != android::OK || layouts.size() < 1)
            {
                E("Failed to get plane layouts. err : %d", err);
                return err;
            }

            if ( layouts.size() > 1 )
            {
                // W("it's not reasonable to get global byte_stride of buffer with planes more than 1.");
            }
            *byte_stride = (layouts[0].strideInBytes);
        }
        // 对于 fourcc不为 DRM_FORMAT_NV15 的情况，认为 Mali不支持 NV15格式,采用 width 作为 byte_stride.
        else
        {

            uint64_t width;

            err = get_width(handle, &width);
            if (err != android::OK )
            {
                E("err : %d", err);
                return err;
            }

            // .KP : from CSY : 分配 rk_video_decoder 输出 buffers 时, 要求的 byte_stride of buffer in NV12_10, 已经通过 width 传入.
            *byte_stride = (int)width;
        }
    }

    return err;
}

int get_format_requested(buffer_handle_t handle, int* format_requested)
{
    auto &mapper = get_service();
    // android::PixelFormat format;    // *format_requested
    PixelFormat format;    // *format_requested

    int err = get_metadata(mapper, handle, MetadataType_PixelFormatRequested, decodePixelFormatRequested, &format);
    if (err != android::OK)
    {
        E("Failed to get pixel_format_requested. err : %d", err);
        return err;
    }

    *format_requested = (int)format;

    return err;
}

int get_usage(buffer_handle_t handle, uint64_t* usage)
{
    auto &mapper = get_service();

    int err = get_metadata(mapper, handle, MetadataType_Usage, decodeUsage, usage);
    if (err != android::OK)
    {
        E("Failed to get pixel_format_requested. err : %d", err);
        return err;
    }

    return err;
}

int get_allocation_size(buffer_handle_t handle, uint64_t* allocation_size)
{
    auto &mapper = get_service();

    int err = get_metadata(mapper, handle, MetadataType_AllocationSize, decodeAllocationSize, allocation_size);
    if (err != android::OK)
    {
        E("Failed to get allocation_size. err : %d", err);
        return err;
    }

    return err;
}

int get_share_fd(buffer_handle_t handle, int* share_fd)
{
    auto &mapper = get_service();
    std::vector<int64_t> fds;

    int err = get_metadata(mapper, handle, ArmMetadataType_PLANE_FDS, decodeArmPlaneFds, &fds);
    if (err != android::OK)
    {
        E("Failed to get plane_fds. err : %d", err);
        return err;
    }
    assert (fds.size() > 0);

    *share_fd = (int)(fds[0]);

    return err;
}

int get_name(buffer_handle_t handle, std::string &name)
{
    auto &mapper = get_service();

    int err = get_metadata(mapper, handle, MetadataType_Name, decodeName, &name);
    if (err != android::OK)
    {
        ALOGE("err : %d", err);
    }

    return err;
}

int get_buffer_id(buffer_handle_t handle, uint64_t* buffer_id)
{
    auto &mapper = get_service();

    int err = get_metadata(mapper, handle, MetadataType_BufferId, decodeBufferId, buffer_id);
    if (err != android::OK)
    {
        ALOGE("err : %d", err);
    }

    return err;
}

status_t importBuffer(buffer_handle_t rawHandle, buffer_handle_t* outHandle)
{
    auto &mapper = get_service();
    Error error;
    auto ret = mapper.importBuffer(android::hardware::hidl_handle(rawHandle),
                                   [&](const auto& tmpError, const auto& tmpBuffer)
                                   {
        error = tmpError;
        if (error != Error::NONE)
        {
            return;
        }
        *outHandle = static_cast<buffer_handle_t>(tmpBuffer);
                                    });

    return static_cast<status_t>((ret.isOk()) ? error : kTransactionError);
}

status_t freeBuffer(buffer_handle_t handle){
    auto &mapper = get_service();
    auto buffer = const_cast<native_handle_t*>(handle);
    auto ret = mapper.freeBuffer(buffer);

    auto error = (ret.isOk()) ? static_cast<Error>(ret) : kTransactionError;
    return static_cast<status_t>(error);
}

status_t lock(buffer_handle_t bufferHandle,
              uint64_t usage,
              int x,
              int y,
              int w,
              int h,
              void** outData)
{
    auto &mapper = get_service();
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    IMapper::Rect accessRegion = {x, y, w, h};

    android::hardware::hidl_handle acquireFenceHandle; // dummy

    Error error;
    auto ret = mapper.lock(buffer,
                           usage,
                           accessRegion,
                           acquireFenceHandle,
                           [&](const auto& tmpError, const auto& tmpData) {
                                error = tmpError;
                                if (error != Error::NONE) {
                                    return;
                                }
                                *outData = tmpData;
                           });

    error = (ret.isOk()) ? error : kTransactionError;

    ALOGW_IF(error != Error::NONE, "lock(%p, ...) failed: %d", bufferHandle, error);

    return static_cast<status_t>(error);
}

void unlock(buffer_handle_t bufferHandle)
{
    auto &mapper = get_service();
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    int releaseFence = -1;
    Error error;
    auto ret = mapper.unlock(buffer,
                             [&](const auto& tmpError, const auto& tmpReleaseFence)
                             {
        error = tmpError;
        if (error != Error::NONE) {
            return;
        }

        auto fenceHandle = tmpReleaseFence.getNativeHandle(); // 预期 unlock() 不会返回有效的 release_fence.
        if (fenceHandle && fenceHandle->numFds == 1)
        {
            E("got unexpected valid fd of release_fence : %d", fenceHandle->data[0]);

            int fd = dup(fenceHandle->data[0]);
            if (fd >= 0) {
                releaseFence = fd;
            } else {
                ALOGD("failed to dup unlock release fence");
                sync_wait(fenceHandle->data[0], -1);
            }
        }
                             });

    if (!ret.isOk()) {
        error = kTransactionError;
    }

    if (error != Error::NONE) {
        ALOGE("unlock(%p) failed with %d", buffer, error);
    }

    return;
}

} // namespace gralloc4

