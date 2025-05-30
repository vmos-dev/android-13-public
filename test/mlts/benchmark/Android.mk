#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_STATIC_JAVA_LIBRARIES := androidx.test.rules collector-device-lib
LOCAL_JAVA_LIBRARIES := android.test.runner.stubs android.test.base.stubs

LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE += device-tests

# List of NNAPI SL libraries for different chipsets, which are determined by SL release notes.
QC_SM8350_NNAPI_SL_LIBS := libnnapi_sl_driver libQnnGpu libQnnHtp libQnnHtpPrepare libQnnHtpV68Skel libQnnHtpV68Stub libUnnhalAccGpu libUnnhalAccHtp
QC_SM8450_NNAPI_SL_LIBS := libnnapi_sl_driver libQnnGpu libQnnHtp libQnnHtpPrepare libQnnHtpV69Skel libQnnHtpV69Stub libUnnhalAccGpu libUnnhalAccHtp

# Set the set of SL libraries to use in this test. By default including all
# chipsets, you can override this variable with a chipset specific one (see
# QC_SMxxxx_NNAPI_SL_LIBS) test a more realistic NNAPI SL distribution scenario.
ifeq ($(strip $(SL_LIBS)),)
	SL_LIBS := $(patsubst $(LOCAL_PATH)/sl_prebuilt/%.so,%,$(wildcard $(LOCAL_PATH)/sl_prebuilt/*.so))
endif

LOCAL_SRC_FILES := $(call all-java-files-under, src/com/android/nn/benchmark)
LOCAL_JNI_SHARED_LIBRARIES := libnnbenchmark_jni libsupport_library_jni $(SL_LIBS)

# need fread_unlocked in version 28
LOCAL_SDK_VERSION := 28
LOCAL_ASSET_DIR := $(LOCAL_PATH)/../models/assets

GOOGLE_TEST_MODELS_DIR := vendor/google/tests/mlts/models/assets
ifneq ($(wildcard $(GOOGLE_TEST_MODELS_DIR)),)
LOCAL_ASSET_DIR += $(GOOGLE_TEST_MODELS_DIR)
endif

# This folder contains metadata describing the SL library distribution.
# Currently the only one is a file sl_prebuilt_filelist.txt with the list of libraries
# in this SL. The file is generated by the build_and_run_benchmark.sh script
# and is used to know the list of files to extract if the user
SL_PREBUILT_METADATA_DIR := $(LOCAL_PATH)/sl_prebuilt/assets
ifneq ($(wildcard $(SL_PREBUILT_METADATA_DIR)),)
LOCAL_ASSET_DIR += $(SL_PREBUILT_METADATA_DIR)
endif

LOCAL_PACKAGE_NAME := NeuralNetworksApiBenchmark
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-MIT
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/LICENSE
include $(BUILD_PACKAGE)

include $(CLEAR_VARS)

LOCAL_STATIC_JAVA_LIBRARIES := androidx.test.rules
LOCAL_JAVA_LIBRARIES := android.test.runner.stubs android.test.base.stubs

LOCAL_MODULE_TAGS := tests
LOCAL_COMPATIBILITY_SUITE += device-tests

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JNI_SHARED_LIBRARIES := libnnbenchmark_jni libsupport_library_jni librandom_graph_test_jni $(SL_LIBS)

# need fread_unlocked in version 28
LOCAL_SDK_VERSION := 28
LOCAL_ASSET_DIR := $(LOCAL_PATH)/../models/assets

GOOGLE_TEST_MODELS_DIR := vendor/google/tests/mlts/models/assets
ifneq ($(wildcard $(GOOGLE_TEST_MODELS_DIR)),)
LOCAL_ASSET_DIR += $(GOOGLE_TEST_MODELS_DIR)
endif

SL_PREBUILT_METADATA_DIR := $(LOCAL_PATH)/sl_prebuilt/assets
ifneq ($(wildcard $(SL_PREBUILT_METADATA_DIR)),)
LOCAL_ASSET_DIR += $(SL_PREBUILT_METADATA_DIR)
endif

LOCAL_PACKAGE_NAME := NeuralNetworksApiCrashTest
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 SPDX-license-identifier-MIT
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/LICENSE
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
