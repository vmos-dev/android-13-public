/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "CamPrvdr@2.4-external"
#define LOG_NDEBUG 0
#include <log/log.h>

#include <regex>
#include <sys/inotify.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <cutils/properties.h>
#include "ExternalCameraProviderImpl_2_4.h"
#include "ExternalCameraDevice_3_4.h"
#include "ExternalCameraDevice_3_5.h"
#include "ExternalCameraDevice_3_6.h"
#include "ExternalFakeCameraDevice_3_4.h"

#ifdef HDMI_ENABLE
#include <rockchip/hardware/hdmi/1.0/IHdmi.h>
#endif

#define FAKE_CAMERA_ENABLE 0
#define FAKE_CAMERAID 100
namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_4 {
namespace implementation {

template struct CameraProvider<ExternalCameraProviderImpl_2_4>;

namespace {
// "device@<version>/external/<id>"
const std::regex kDeviceNameRE("device@([0-9]+\\.[0-9]+)/external/(.+)");
const int kMaxDevicePathLen = 256;
const char* kDevicePath = "/dev/";
constexpr char kPrefix[] = "video";
constexpr int kPrefixLen = sizeof(kPrefix) - 1;
constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen + 1;

char kV4l2DevicePath[kMaxDevicePathLen];

bool matchDeviceName(int cameraIdOffset,
                     const hidl_string& deviceName, std::string* deviceVersion,
                     std::string* cameraDevicePath) {
    std::string deviceNameStd(deviceName.c_str());
    std::smatch sm;
    if (std::regex_match(deviceNameStd, sm, kDeviceNameRE)) {
        if (deviceVersion != nullptr) {
            *deviceVersion = sm[1];
        }
        if (cameraDevicePath != nullptr) {
            *cameraDevicePath = "/dev/video" + std::to_string(std::stoi(sm[2]) - cameraIdOffset);
        }
        return true;
    }
    return false;
}

} // anonymous namespace

sp<V4L2DeviceEvent> ExternalCameraProviderImpl_2_4::mV4l2Event;
ExternalCameraProviderImpl_2_4* ExternalCameraProviderImpl_2_4::sInstance;
ExternalCameraProviderImpl_2_4::ExternalCameraProviderImpl_2_4()
    : mCfg(ExternalCameraConfig::loadFromCfg()) {
    mHotPlugThread = sp<HotplugThread>::make(this);
    mHotPlugThread->run("ExtCamHotPlug", PRIORITY_BACKGROUND);
#ifdef HDMI_ENABLE
    ExternalCameraProviderImpl_2_4::mV4l2Event = new V4L2DeviceEvent();
    ExternalCameraProviderImpl_2_4::sInstance= this;
    mV4l2Event->RegisterEventvCallBack((V4L2EventCallBack)ExternalCameraProviderImpl_2_4::hinDevEventCallback);
#endif

    mPreferredHal3MinorVersion =
        property_get_int32("ro.vendor.camera.external.hal3TrebleMinorVersion", 4);
    ALOGV("Preferred HAL 3 minor version is %d", mPreferredHal3MinorVersion);
    switch(mPreferredHal3MinorVersion) {
        case 4:
        case 5:
        case 6:
            // OK
            break;
        default:
            ALOGW("Unknown minor camera device HAL version %d in property "
                    "'camera.external.hal3TrebleMinorVersion', defaulting to 4",
                    mPreferredHal3MinorVersion);
            mPreferredHal3MinorVersion = 4;
    }
}

ExternalCameraProviderImpl_2_4::~ExternalCameraProviderImpl_2_4() {
    mHotPlugThread->requestExit();
    if (ExternalCameraProviderImpl_2_4::mV4l2Event)
        ExternalCameraProviderImpl_2_4::mV4l2Event->closePipe();
    if (ExternalCameraProviderImpl_2_4::mV4l2Event)
        ExternalCameraProviderImpl_2_4::mV4l2Event->closeEventThread();
}
V4L2EventCallBack ExternalCameraProviderImpl_2_4::hinDevEventCallback(int status){
    ALOGD("@%s,status:%d",__FUNCTION__,status);
    if(status){
        ExternalCameraProviderImpl_2_4::sInstance->deviceAdded(kV4l2DevicePath);
    }else{
        ExternalCameraProviderImpl_2_4::sInstance->deviceRemoved(kV4l2DevicePath);
    }
    return 0;
}

Return<Status> ExternalCameraProviderImpl_2_4::setCallback(
        const sp<ICameraProviderCallback>& callback) {
    {
        Mutex::Autolock _l(mLock);
        mCallbacks = callback;
    }
    if (mCallbacks == nullptr) {
        return Status::OK;
    }
    // Send a callback for all devices to initialize
    {
        for (const auto& pair : mCameraStatusMap) {
            mCallbacks->cameraDeviceStatusChange(pair.first, pair.second);
        }
    }

    return Status::OK;
}

Return<void> ExternalCameraProviderImpl_2_4::getVendorTags(
        ICameraProvider::getVendorTags_cb _hidl_cb) {
    // No vendor tag support for USB camera
    hidl_vec<VendorTagSection> zeroSections;
    _hidl_cb(Status::OK, zeroSections);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraIdList(
        ICameraProvider::getCameraIdList_cb _hidl_cb) {
    // External camera HAL always report 0 camera, and extra cameras
    // are just reported via cameraDeviceStatusChange callbacks
    hidl_vec<hidl_string> hidlDeviceNameList;
    _hidl_cb(Status::OK, hidlDeviceNameList);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::isSetTorchModeSupported(
        ICameraProvider::isSetTorchModeSupported_cb _hidl_cb) {
    // setTorchMode API is supported, though right now no external camera device
    // has a flash unit.
    _hidl_cb (Status::OK, true);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraDeviceInterface_V1_x(
        const hidl_string&,
        ICameraProvider::getCameraDeviceInterface_V1_x_cb _hidl_cb) {
    // External Camera HAL does not support HAL1
    _hidl_cb(Status::OPERATION_NOT_SUPPORTED, nullptr);
    return Void();
}

Return<void> ExternalCameraProviderImpl_2_4::getCameraDeviceInterface_V3_x(
        const hidl_string& cameraDeviceName,
        ICameraProvider::getCameraDeviceInterface_V3_x_cb _hidl_cb) {
    ALOGD("cameraDeviceName:%s", cameraDeviceName.c_str());
    std::string cameraDevicePath, deviceVersion;
    bool match = matchDeviceName(mCfg.cameraIdOffset, cameraDeviceName,
                                 &deviceVersion, &cameraDevicePath);
    ALOGD("cameraDevicePath:%s", cameraDevicePath.c_str());
    if (!match) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    if (mCameraStatusMap.count(cameraDeviceName) == 0 ||
            mCameraStatusMap[cameraDeviceName] != CameraDeviceStatus::PRESENT) {
        _hidl_cb(Status::ILLEGAL_ARGUMENT, nullptr);
        return Void();
    }

    if (std::atoi(cameraDevicePath.c_str() + kDevicePrefixLen) >= FAKE_CAMERAID) {
#ifdef SUBDEVICE_ENABLE
        if(std::atoi(cameraDevicePath.c_str() + kDevicePrefixLen) > FAKE_CAMERAID
         && std::atoi(cameraDevicePath.c_str() + kDevicePrefixLen) < SUBDEVICE_OFFSET + (SUBDEVICE_NUM-1)*CAMERAID_MASK){
            ALOGV("Constructing v3.4 external sub camera device");
            sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl =
                new device::V3_4::implementation::ExternalCameraDevice(
                        cameraDevicePath, mCfg);
            if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
                ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
                _hidl_cb(Status::INTERNAL_ERROR, nullptr);
                return Void();
            }

            IF_ALOGV() {
                deviceImpl->getInterface()->interfaceChain([](
                    ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
                        ALOGV("Device interface chain:");
                        for (auto iface : interfaceChain) {
                            ALOGV("  %s", iface.c_str());
                        }
                    });
            }

            _hidl_cb (Status::OK, deviceImpl->getInterface());
        }
#else
        ALOGV("Constructing v3.4 external fake camera device");
        sp<device::V3_4::implementation::ExternalFakeCameraDevice> deviceImpl = 
            new device::V3_4::implementation::ExternalFakeCameraDevice(
                    cameraDevicePath, mCfg);
        if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
            ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
            _hidl_cb(Status::INTERNAL_ERROR, nullptr);
            return Void();
        }
    
        IF_ALOGV() {
            deviceImpl->getInterface()->interfaceChain([](
                ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
                    ALOGV("Device interface chain:");
                    for (auto iface : interfaceChain) {
                        ALOGV("  %s", iface.c_str());
                    }
                });
        }
    
        _hidl_cb (Status::OK, deviceImpl->getInterface());
#endif
    } else {
 
    sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl;
    switch (mPreferredHal3MinorVersion) {
        case 4: {
            ALOGV("Constructing v3.4 external camera device");
            deviceImpl = new device::V3_4::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        case 5: {
            ALOGV("Constructing v3.5 external camera device");
            deviceImpl = new device::V3_5::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        case 6: {
            ALOGV("Constructing v3.6 external camera device");
            deviceImpl = new device::V3_6::implementation::ExternalCameraDevice(
                    cameraDevicePath, mCfg);
            break;
        }
        default:
            ALOGE("%s: Unknown HAL minor version %d!", __FUNCTION__, mPreferredHal3MinorVersion);
            _hidl_cb(Status::INTERNAL_ERROR, nullptr);
            return Void();
    }

    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGE("%s: camera device %s init failed!", __FUNCTION__, cameraDevicePath.c_str());
        _hidl_cb(Status::INTERNAL_ERROR, nullptr);
        return Void();
    }

    IF_ALOGV() {
        deviceImpl->getInterface()->interfaceChain([](
            ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
                ALOGV("Device interface chain:");
                for (auto iface : interfaceChain) {
                    ALOGV("  %s", iface.c_str());
                }
            });
    }

    _hidl_cb (Status::OK, deviceImpl->getInterface());
    }
    return Void();
}

void ExternalCameraProviderImpl_2_4::addExternalCamera(const char* devName) {
    ALOGI("ExtCam: adding %s to External Camera HAL!", devName);
    {
    Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
    if (mPreferredHal3MinorVersion == 6) {
        deviceName = std::string("device@3.6/external/") + cameraId;
    } else if (mPreferredHal3MinorVersion == 5) {
        deviceName = std::string("device@3.5/external/") + cameraId;
    } else {
        deviceName = std::string("device@3.4/external/") + cameraId;
    }
    mCameraStatusMap[deviceName] = CameraDeviceStatus::PRESENT;
    if (mCallbacks != nullptr) {
        mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::PRESENT);
    }
    }
#ifdef SUBDEVICE_ENABLE
    if(std::atoi(devName + kDevicePrefixLen) + mCfg.cameraIdOffset < SUBDEVICE_OFFSET + (SUBDEVICE_NUM-1)*CAMERAID_MASK){
        std::string cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
         //addSubCamera
        ALOGD("@%s,%d, cameraId:%s ,SUBDEVICE_NUM:%d",__FUNCTION__,__LINE__,cameraId.c_str(),SUBDEVICE_NUM);
        std::string subDevName = (std::string("/dev/video")+cameraId);
        deviceAdded(subDevName.c_str());
    }
#endif
}

void ExternalCameraProviderImpl_2_4::deviceAdded(const char* devName) {
    if (std::atoi(devName + kDevicePrefixLen) >= FAKE_CAMERAID)
    {
#ifdef SUBDEVICE_ENABLE
        if(std::atoi(devName + kDevicePrefixLen) > FAKE_CAMERAID
         && std::atoi(devName + kDevicePrefixLen) < SUBDEVICE_OFFSET + (SUBDEVICE_NUM-1)*CAMERAID_MASK){
            sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl =
            new device::V3_4::implementation::ExternalCameraDevice(devName, mCfg);
            if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
                ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
                return;
            }
            deviceImpl.clear();
        }
#else
        sp<device::V3_4::implementation::ExternalFakeCameraDevice> deviceImpl =
            new device::V3_4::implementation::ExternalFakeCameraDevice(devName, mCfg);
        if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
            ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
            return;
        }
        deviceImpl.clear();
#endif
    } else {
    {
        base::unique_fd fd(::open(devName, O_RDWR));
        if (fd.get() < 0) {
            ALOGE("%s open v4l2 device %s failed:%s", __FUNCTION__, devName, strerror(errno));
            return;
        }

        struct v4l2_capability capability;
        int ret = ioctl(fd.get(), VIDIOC_QUERYCAP, &capability);
        if (ret < 0) {
            ALOGE("%s v4l2 QUERYCAP %s failed", __FUNCTION__, devName);
            return;
        }

        if (!((capability.device_caps & V4L2_CAP_VIDEO_CAPTURE) || (capability.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE))) {
            ALOGW("%s wpzz test device %s does not support VIDEO_CAPTURE", __FUNCTION__, devName);
            return;
        }
    }
    // See if we can initialize ExternalCameraDevice correctly
    sp<device::V3_4::implementation::ExternalCameraDevice> deviceImpl =
            new device::V3_4::implementation::ExternalCameraDevice(devName, mCfg);
    if (deviceImpl == nullptr || deviceImpl->isInitFailed()) {
        ALOGW("%s: Attempt to init camera device %s failed!", __FUNCTION__, devName);
        return;
    }
    deviceImpl.clear();
    }
    addExternalCamera(devName);
    return;
}

void ExternalCameraProviderImpl_2_4::deviceRemoved(const char* devName) {
    {Mutex::Autolock _l(mLock);
    std::string deviceName;
    std::string cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
    if (mPreferredHal3MinorVersion == 6) {
        deviceName = std::string("device@3.6/external/") + cameraId;
    } else if (mPreferredHal3MinorVersion == 5) {
        deviceName = std::string("device@3.5/external/") + cameraId;
    } else {
        deviceName = std::string("device@3.4/external/") + cameraId;
    }
    if (mCameraStatusMap.find(deviceName) != mCameraStatusMap.end()) {
        mCameraStatusMap.erase(deviceName);
        if (mCallbacks != nullptr) {
            mCallbacks->cameraDeviceStatusChange(deviceName, CameraDeviceStatus::NOT_PRESENT);
        }
    } else {
        ALOGE("%s: cannot find camera device %s", __FUNCTION__, devName);
    }
    }
#ifdef SUBDEVICE_ENABLE
    std::string cameraId = std::to_string(mCfg.cameraIdOffset + std::atoi(devName + kDevicePrefixLen));
    ALOGV("@%s,%d,devName:%s,cameraId:%s",__FUNCTION__,__LINE__,devName,cameraId.c_str());
    if(std::atoi(cameraId.c_str()) > FAKE_CAMERAID && std::atoi(cameraId.c_str()) < SUBDEVICE_OFFSET + (SUBDEVICE_NUM-1)*CAMERAID_MASK){
        cameraId = std::to_string(mCfg.cameraIdOffset +
                                          std::atoi(devName + kDevicePrefixLen));
                                          ALOGV("@%s(%d) cameraId:%s",__FUNCTION__,__LINE__,cameraId.c_str());
        std::string deviceName = std::string("/dev/video") + cameraId;
        deviceRemoved(deviceName.c_str());
    }
#endif
}

ExternalCameraProviderImpl_2_4::HotplugThread::HotplugThread(
        ExternalCameraProviderImpl_2_4* parent) :
        Thread(/*canCallJava*/false),
        mParent(parent),
        mInternalDevices(parent->mCfg.mInternalDevices) {}

ExternalCameraProviderImpl_2_4::HotplugThread::~HotplugThread() {
    if (ExternalCameraProviderImpl_2_4::mV4l2Event)
        ExternalCameraProviderImpl_2_4::mV4l2Event->closeEventThread();
}




int ExternalCameraProviderImpl_2_4::HotplugThread::findDevice(int id, int& initWidth, int& initHeight,int& initFormat ) {
    const int kMaxDevicePathLen = 256;
const char* kDevicePath = "/dev/";
const char kPrefix[] = "video";
const int kPrefixLen = sizeof(kPrefix) - 1;
//constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen + 1;
const char kHdmiNodeName[] = "rk_hdmirx";
int mHinDevHandle = 0;
int mFrameWidth,mFrameHeight,mBufferSize;

static v4l2_buf_type TVHAL_V4L2_BUF_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ALOGD("%s called", __func__);
    // Find existing /dev/video* devices
    DIR* devdir = opendir(kDevicePath);
    int videofd,ret;
    if(devdir == 0) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return -1;
    }
    struct dirent* de;
    while ((de = readdir(devdir)) != 0) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp(kPrefix, de->d_name, kPrefixLen)) {
		std::string deviceId(de->d_name + kPrefixLen);
		ALOGD(" v4l device %s found", de->d_name);
		char v4l2DeviceDriver[16];
		snprintf(kV4l2DevicePath, kMaxDevicePathLen,"%s%s", kDevicePath, de->d_name);
		videofd = open(kV4l2DevicePath, O_RDWR);
		if (videofd < 0){
			ALOGE("[%s %d] mHinDevHandle:%x [%s]", __FUNCTION__, __LINE__, videofd,strerror(errno));
			continue;
		} else {
			ALOGE("%s open device %s successful.", __FUNCTION__, kV4l2DevicePath);
			struct v4l2_capability cap;
			ret = ioctl(videofd, VIDIOC_QUERYCAP, &cap);
			if (ret < 0) {
				ALOGE("VIDIOC_QUERYCAP Failed, error: %s", strerror(errno));
				close(videofd);
				continue;
		}
		snprintf(v4l2DeviceDriver, 16,"%s",cap.driver);
		ALOGE("VIDIOC_QUERYCAP driver=%s,%s", cap.driver,v4l2DeviceDriver);
		ALOGE("VIDIOC_QUERYCAP card=%s", cap.card);
		ALOGE("VIDIOC_QUERYCAP version=%d", cap.version);
		ALOGE("VIDIOC_QUERYCAP capabilities=0x%08x,0x%08x", cap.capabilities,V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
		ALOGE("VIDIOC_QUERYCAP device_caps=0x%08x", cap.device_caps);
		if(!strncmp(kHdmiNodeName, v4l2DeviceDriver, sizeof(kHdmiNodeName)-1)){
			mHinDevHandle =  videofd;
#ifdef HDMI_ENABLE
            class IHdmiRxStatusCallbackImpl:public ::rockchip::hardware::hdmi::V1_0::IHdmiRxStatusCallback{
                public:
                IHdmiRxStatusCallbackImpl(int fd):mFd(fd){
                    ALOGD("@%s,fd:%d",__FUNCTION__,mFd);
                }
                Return<void> getHdmiRxStatus(getHdmiRxStatus_cb _hidl_cb){
                    rockchip::hardware::hdmi::V1_0::HdmiStatus status;
                    struct v4l2_dv_timings timings;
                    int err = ioctl(mFd, VIDIOC_SUBDEV_QUERY_DV_TIMINGS, &timings);
                    if (err < 0) {
                        ALOGD("get VIDIOC_SUBDEV_QUERY_DV_TIMINGS failed ,%d(%s)", errno, strerror(errno));
                        _hidl_cb(status);
                        return Void();
                    }

                    const struct v4l2_bt_timings *bt =&timings.bt;
                    double tot_width, tot_height;
                    tot_height = bt->height +
                        bt->vfrontporch + bt->vsync + bt->vbackporch +
                        bt->il_vfrontporch + bt->il_vsync + bt->il_vbackporch;
                    tot_width = bt->width +
                        bt->hfrontporch + bt->hsync + bt->hbackporch;
                    ALOGD("%s:%dx%d, pixelclock:%lld Hz, %.2f fps", __func__,
                    timings.bt.width, timings.bt.height,
                    timings.bt.pixelclock,static_cast<double>(bt->pixelclock) /(tot_width * tot_height));
                    status.width = timings.bt.width;
                    status.height = timings.bt.height;
                    status.fps = round(static_cast<double>(bt->pixelclock) /(tot_width * tot_height));
                    status.status = 1;
                    _hidl_cb(status);
                    return Void();
                }
                private:
                int mFd;
            };
            static IHdmiRxStatusCallbackImpl  IHdmiRxStatusCallbackImpl(videofd);
            sp<rockchip::hardware::hdmi::V1_0::IHdmi> client = rockchip::hardware::hdmi::V1_0::IHdmi::getService();
            if(client.get()!= nullptr){
                ALOGD("foundHdmiDevice:%s, cameraIdOffset:%d",deviceId.c_str(),mParent->mCfg.cameraIdOffset);
                 const ::android::sp<rockchip::hardware::hdmi::V1_0::IHdmiRxStatusCallback> cb = &IHdmiRxStatusCallbackImpl;
                client->foundHdmiDevice(::android::hardware::hidl_string(
                    std::to_string(mParent->mCfg.cameraIdOffset+std::stoi(deviceId.c_str()))),cb);
            }
#endif
            ALOGD("mHinDevHandle::%d ,kV4l2DevicePath:%s ,deviceId:%s",mHinDevHandle,kV4l2DevicePath,deviceId.c_str());
			if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
				ALOGE("V4L2_CAP_VIDEO_CAPTURE is  a video capture device, capabilities: %x\n", cap.capabilities);
					TVHAL_V4L2_BUF_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		}else if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
				ALOGE("V4L2_CAP_VIDEO_CAPTURE_MPLANE is  a video capture device, capabilities: %x\n", cap.capabilities);
				TVHAL_V4L2_BUF_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			}
			break;
		}else{
			close(videofd);
			ALOGE("isnot hdmirx,VIDIOC_QUERYCAP driver=%s", cap.driver);
		}
            }
        }
    }
    closedir(devdir);
    if (mHinDevHandle < 0){
        ALOGE("[%s %d] mHinDevHandle:%x", __FUNCTION__, __LINE__, mHinDevHandle);
        return -1;
    }
    if(ExternalCameraProviderImpl_2_4::mV4l2Event){
        ExternalCameraProviderImpl_2_4::mV4l2Event->initialize(mHinDevHandle);
    }

    id = 0;
    initFormat=0;

    mFrameWidth = initWidth;
    mFrameHeight = initHeight;
    mBufferSize = mFrameWidth * mFrameHeight * 3/2;
    return 0;
}
bool ExternalCameraProviderImpl_2_4::HotplugThread::threadLoop() {
    // Find existing /dev/video* devices

#ifdef HDMI_ENABLE
    int id = 0,  initWidth, initHeight,initFormat ;
    findDevice(id,initWidth,initHeight,initFormat);
#endif
    DIR* devdir = opendir(kDevicePath);
    if(devdir == 0) {
        ALOGE("%s: cannot open %s! Exiting threadloop", __FUNCTION__, kDevicePath);
        return false;
    }

    struct dirent* de;
    while ((de = readdir(devdir)) != 0) {
        // Find external v4l devices that's existing before we start watching and add them
        if (!strncmp(kPrefix, de->d_name, kPrefixLen)) {
            // TODO: This might reject some valid devices. Ex: internal is 33 and a device named 3
            //       is added.
            std::string deviceId(de->d_name + kPrefixLen);
            if (mInternalDevices.count(deviceId) == 0) {
                ALOGV("Non-internal v4l device %s found", de->d_name);
                char v4l2DevicePath[kMaxDevicePathLen];
                snprintf(v4l2DevicePath, kMaxDevicePathLen,
                        "%s%s", kDevicePath, de->d_name);
                mParent->deviceAdded(v4l2DevicePath);
            }
        }
    }
#if FAKE_CAMERA_ENABLE
    mParent->deviceAdded("/dev/video100");
#endif
    closedir(devdir);

    // Watch new video devices
    mINotifyFD = inotify_init();
    if (mINotifyFD < 0) {
        ALOGE("%s: inotify init failed! Exiting threadloop", __FUNCTION__);
        return true;
    }

    mWd = inotify_add_watch(mINotifyFD, kDevicePath, IN_CREATE | IN_DELETE);
    if (mWd < 0) {
        ALOGE("%s: inotify add watch failed! Exiting threadloop", __FUNCTION__);
        return true;
    }

    ALOGI("%s start monitoring new V4L2 devices", __FUNCTION__);

    bool done = false;
    char eventBuf[512];
    while (!done) {
        int offset = 0;
        int ret = read(mINotifyFD, eventBuf, sizeof(eventBuf));
        if (ret >= (int)sizeof(struct inotify_event)) {
            while (offset < ret) {
                struct inotify_event* event = (struct inotify_event*)&eventBuf[offset];
                if (event->wd == mWd) {
                    if (!strncmp(kPrefix, event->name, kPrefixLen)) {
                        std::string deviceId(event->name + kPrefixLen);
                        if (mInternalDevices.count(deviceId) == 0) {
                            char v4l2DevicePath[kMaxDevicePathLen];
                            snprintf(v4l2DevicePath, kMaxDevicePathLen,
                                    "%s%s", kDevicePath, event->name);
                            if (event->mask & IN_CREATE) {
                                mParent->deviceAdded(v4l2DevicePath);
                            }
                            if (event->mask & IN_DELETE) {
                                mParent->deviceRemoved(v4l2DevicePath);
                            }
                        }
                    }
                }
                offset += sizeof(struct inotify_event) + event->len;
            }
        }
    }

    return true;
}

}  // namespace implementation
}  // namespace V2_4
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
