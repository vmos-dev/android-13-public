#
# Copyright 2020 The Android Open Source Project
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

PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/bluejay_car/aosp_bluejay_car.mk \
    $(LOCAL_DIR)/bramble_car/aosp_bramble_car.mk \
    $(LOCAL_DIR)/coral_car/aosp_coral_car.mk \
    $(LOCAL_DIR)/flame_car/aosp_flame_car.mk \
    $(LOCAL_DIR)/oriole_car/aosp_oriole_car.mk \
    $(LOCAL_DIR)/raven_car/aosp_raven_car.mk \
    $(LOCAL_DIR)/redfin_car/aosp_redfin_car.mk \
    $(LOCAL_DIR)/sunfish_car/aosp_sunfish_car.mk


COMMON_LUNCH_CHOICES := \
    aosp_bluejay_car-userdebug \
    aosp_bramble_car-userdebug \
    aosp_coral_car-userdebug \
    aosp_flame_car-userdebug \
    aosp_oriole_car-userdebug \
    aosp_raven_car-userdebug \
    aosp_redfin_car-userdebug \
    aosp_sunfish_car-userdebug
