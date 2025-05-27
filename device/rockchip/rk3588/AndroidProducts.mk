#
# Copyright 2014 The Android Open-Source Project
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
    $(LOCAL_DIR)/rk3588_s/rk3588_s.mk \
    $(LOCAL_DIR)/rk3588_t/rk3588_t.mk \
    $(LOCAL_DIR)/rk3588s_s/rk3588s_s.mk \
    $(LOCAL_DIR)/rk3588s_t/rk3588s_t.mk \
    $(LOCAL_DIR)/rk3588_box/rk3588_box.mk \
    $(LOCAL_DIR)/rk3588_xr/rk3588_xr.mk \
    $(LOCAL_DIR)/rk3588m_s/rk3588m_s.mk \
    $(LOCAL_DIR)/rk3588m_car/rk3588m_car.mk \
    $(LOCAL_DIR)/rk3588_docker/rk3588_docker.mk \
    $(LOCAL_DIR)/vcloud/vcloud.mk \

COMMON_LUNCH_CHOICES := \
    rk3588_s-userdebug \
    rk3588_s-user \
    rk3588_t-userdebug \
    rk3588_t-user \
    rk3588s_s-userdebug \
    rk3588s_s-user \
    rk3588s_t-userdebug \
    rk3588s_t-user \
    rk3588_box-userdebug \
    rk3588_box-user \
    rk3588_xr-userdebug \
    rk3588_xr-user \
    rk3588m_s-userdebug \
    rk3588m_s-user \
    rk3588m_car-userdebug \
    rk3588m_car-user \
    rk3588_docker-userdebug \
    rk3588_docker-user \
    vcloud-userdebug \
    vcloud-user
