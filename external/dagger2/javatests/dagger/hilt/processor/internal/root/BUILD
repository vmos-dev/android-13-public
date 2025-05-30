# Copyright (C) 2020 The Dagger Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# Description:
#   Tests for internal code for implementing Hilt processors.

load("//java/dagger/testing/compile:macros.bzl", "compiler_test")

package(default_visibility = ["//:src"])

android_library(
    name = "MyAppPreviousCompilation",
    srcs = ["MyAppPreviousCompilation.java"],
    deps = [
        "//java/dagger/hilt/android:hilt_android_app",
    ],
)

compiler_test(
    name = "MyAppPreviousCompilationTest",
    srcs = ["MyAppPreviousCompilationTest.java"],
    compiler_deps = [
        ":MyAppPreviousCompilation",
        "//java/dagger/hilt/android:hilt_android_app",
        "//java/dagger/hilt/android/testing:hilt_android_test",
        "@androidsdk//:platforms/android-30/android.jar",
        "@maven//:androidx_annotation_annotation",
        "@maven//:org_robolectric_robolectric",
        "@maven//:androidx_test_ext_junit",
        "@maven//:androidx_test_core",
    ],
    deps = [
        "//java/dagger/internal/guava:collect",
        "//javatests/dagger/hilt/android/processor:android_compilers",
        "@google_bazel_common//third_party/java/compile_testing",
        "@google_bazel_common//third_party/java/junit",
        "@google_bazel_common//third_party/java/truth",
    ],
)

android_library(
    name = "MyTestPreviousCompilation",
    srcs = ["MyTestPreviousCompilation.java"],
    deps = [
        "//:android_local_test_exports",
        "//java/dagger/hilt/android/testing:hilt_android_test",
    ],
)

compiler_test(
    name = "MyTestPreviousCompilationTest",
    srcs = ["MyTestPreviousCompilationTest.java"],
    compiler_deps = [
        ":MyTestPreviousCompilation",
        "//java/dagger/hilt/android:hilt_android_app",
        "//java/dagger/hilt/android/testing:hilt_android_test",
        "@androidsdk//:platforms/android-30/android.jar",
        "@maven//:androidx_annotation_annotation",
        "@maven//:org_robolectric_robolectric",
        "@maven//:androidx_test_ext_junit",
        "@maven//:androidx_test_core",
    ],
    deps = [
        "//java/dagger/internal/guava:collect",
        "//javatests/dagger/hilt/android/processor:android_compilers",
        "@google_bazel_common//third_party/java/compile_testing",
        "@google_bazel_common//third_party/java/junit",
        "@google_bazel_common//third_party/java/truth",
    ],
)

compiler_test(
    name = "RootProcessorErrorsTest",
    srcs = ["RootProcessorErrorsTest.java"],
    compiler_deps = [
        "//java/dagger/hilt/android:hilt_android_app",
        "//java/dagger/hilt/android/testing:hilt_android_test",
        "@androidsdk//:platforms/android-30/android.jar",
        "@maven//:androidx_annotation_annotation",
        "@maven//:org_robolectric_robolectric",
        "@maven//:androidx_test_ext_junit",
        "@maven//:androidx_test_core",
    ],
    deps = [
        "//java/dagger/internal/guava:collect",
        "//javatests/dagger/hilt/android/processor:android_compilers",
        "@google_bazel_common//third_party/java/compile_testing",
        "@google_bazel_common//third_party/java/junit",
        "@google_bazel_common//third_party/java/truth",
    ],
)

compiler_test(
    name = "RootFileFormatterTest",
    srcs = ["RootFileFormatterTest.java"],
    compiler_deps = [
        "//java/dagger/hilt/android:hilt_android_app",
        "//java/dagger/hilt/android/testing:hilt_android_test",
        "@androidsdk//:platforms/android-30/android.jar",
        "@maven//:androidx_annotation_annotation",
        "@maven//:org_robolectric_robolectric",
        "@maven//:androidx_test_ext_junit",
        "@maven//:androidx_test_core",
    ],
    deps = [
        "//java/dagger/internal/guava:base",
        "//javatests/dagger/hilt/android/processor:android_compilers",
        "@google_bazel_common//third_party/java/compile_testing",
        "@google_bazel_common//third_party/java/junit",
        "@google_bazel_common//third_party/java/truth",
    ],
)

filegroup(
    name = "srcs_filegroup",
    srcs = glob(["*"]),
)
