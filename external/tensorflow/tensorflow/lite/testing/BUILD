# copybara:uncomment(oss-unused) load("//tools/build_defs/build_test:build_test.bzl", "build_test")
load(
    "//tensorflow/lite:build_def.bzl",
    "gen_zip_test",
    "gen_zipped_test_file",
    "generated_test_models_all",
    "merged_test_models",
    "tflite_custom_android_library",
    "tflite_custom_cc_library",
)
load("//tensorflow/lite:special_rules.bzl", "tflite_portable_test_suite")
load("//tensorflow/lite/testing:tflite_model_test.bzl", "tflite_model_test")
load("//tensorflow:tensorflow.bzl", "pybind_extension")
load(
    "//tensorflow:tensorflow.bzl",
    "tf_cc_binary",
    "tf_cc_test",
    "tf_features_nolayering_check_if_android_or_ios",
)

package(
    default_visibility = [
        "//visibility:public",
    ],
    licenses = ["notice"],
)

exports_files([
    "generated_examples_zip_test.cc",
    "tflite_diff_example_test.cc",
    "init_tensorflow.h",
    "init_tensorflow.cc",
])

[gen_zip_test(
    name = "zip_test_%s" % test_name,
    size = "medium",
    srcs = ["generated_examples_zip_test.cc"],
    additional_test_tags_args = {
        "xnnpack": (
            # TODO(b/162696268): remove 'notap' once the bug is fixed.
            ["notap"],
            ["--use_xnnpack=true"],
        ),
    },
    conversion_mode = conversion_mode,
    # copybara:uncomment_begin(no special handling for Android in OSS)
    # data = select({
    # "//tensorflow:android": [],
    # "//conditions:default": [
    # ":zip_%s" % test_name,
    # "//third_party/unzip",
    # ],
    # }),
    # copybara:uncomment_end_and_comment_begin
    data = [":zip_%s" % test_name],
    # copybara:comment_end
    shard_count = 20,
    test_args = args + select({
        "//tensorflow:android": [],
        "//conditions:default": [
            "--zip_file_path=$(location :zip_%s)" % test_name,
            # TODO(angerson) We may be able to add an external unzip binary instead
            # of relying on an existing one for OSS builds.
            "--unzip_binary_path=/usr/bin/unzip",
        ],
    }),
    test_name = test_name,
    test_tags = tags + [
        "no_gpu",  # Executing with TF GPU configurations is redundant.
        "no_oss",
        "tflite_not_portable_intentional",
    ],
    deps = [
        ":parse_testdata_lib",
        ":tflite_driver",
        ":tflite_driver_delegate_providers",
        ":util",
        "@com_google_googletest//:gtest",
        "@com_googlesource_code_re2//:re2",
        "//tensorflow/lite:builtin_op_data",
        "//tensorflow/lite:framework",
        "//tensorflow/lite/kernels:builtin_ops",
        "//tensorflow/lite/kernels:test_delegate_providers_lib",
    ] + select({
        "//conditions:default": [
            "//tensorflow/core:framework_internal",
            "//tensorflow/core:lib",
            "//tensorflow/core:test",
        ],
        "//tensorflow:android": [
            "//tensorflow/core:portable_tensorflow_lib",
            "//tensorflow/core:portable_tensorflow_test_lib",
        ],
    }),
) for conversion_mode, test_name, tags, args in generated_test_models_all() + merged_test_models()]

test_suite(
    name = "generated_zip_tests",
    tags = [
        "gen_zip_test",
    ],
)

py_library(
    name = "toco_convert",
    srcs = ["toco_convert.py"],
    data = [
        "//tensorflow/lite/toco",
    ],
    srcs_version = "PY3",
    deps = [
        ":zip_test_utils",
        "//tensorflow:tensorflow_py",
        "//third_party/py/numpy",
    ],
)

py_library(
    name = "op_tests",
    srcs = glob(["op_tests/*.py"]),
    srcs_version = "PY3",
    deps = [
        ":zip_test_utils",
        "//tensorflow:tensorflow_py",
        "//third_party/py/numpy",
    ],
)

py_library(
    name = "generate_examples_lib",
    srcs = ["generate_examples_lib.py"],
    srcs_version = "PY3",
    deps = [
        ":op_tests",
        ":zip_test_utils",
        "//tensorflow:tensorflow_py",
    ],
)

py_library(
    name = "zip_test_utils",
    srcs = ["zip_test_utils.py"],
    srcs_version = "PY3",
    deps = [
        ":_pywrap_string_util",
        ":generate_examples_report",
        "//tensorflow:tensorflow_py",
        "//third_party/py/numpy",
    ],
)

py_binary(
    name = "generate_examples",
    srcs = ["generate_examples.py"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":generate_examples_lib",
        ":toco_convert",
        "//tensorflow:tensorflow_py",
        "//third_party/py/numpy",
        "@six_archive//:six",
    ],
)

py_library(
    name = "generate_examples_report",
    srcs = ["generate_examples_report.py"],
    srcs_version = "PY3",
)

cc_library(
    name = "parse_testdata_lib",
    srcs = ["parse_testdata.cc"],
    hdrs = ["parse_testdata.h"],
    deps = [
        ":message",
        ":split",
        ":test_runner",
        "//tensorflow/lite:framework",
    ],
)

cc_library(
    name = "message",
    srcs = ["message.cc"],
    hdrs = ["message.h"],
    deps = [":tokenize"],
)

cc_test(
    name = "message_test",
    srcs = ["message_test.cc"],
    deps = [
        ":message",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "split",
    srcs = ["split.cc"],
    hdrs = ["split.h"],
    deps = [
        "//tensorflow/lite:string",
    ],
)

cc_test(
    name = "split_test",
    size = "small",
    srcs = ["split_test.cc"],
    deps = [
        ":split",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "join",
    hdrs = ["join.h"],
    deps = ["//tensorflow/lite:string"],
)

cc_test(
    name = "join_test",
    size = "small",
    srcs = ["join_test.cc"],
    deps = [
        ":join",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "tflite_driver",
    srcs = ["tflite_driver.cc"],
    hdrs = ["tflite_driver.h"],
    deps = [
        ":join",
        ":split",
        ":test_runner",
        "@com_google_absl//absl/strings",
        "//tensorflow/lite:builtin_op_data",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:string_util",
        "//tensorflow/lite/kernels:builtin_ops",
        "//tensorflow/lite/kernels:custom_ops",
        "//tensorflow/lite/kernels/gradient:gradient_ops",
        "//tensorflow/lite/kernels:reference_ops",
        "//tensorflow/lite/kernels:test_delegate_providers_lib",
        "//tensorflow/lite/kernels/parse_example:parse_example",
        "//tensorflow/lite/kernels/perception:perception_ops",
        "//tensorflow/lite/tools/evaluation:utils",
    ] + select({
        "//tensorflow:ios": [],
        "//conditions:default": ["//tensorflow/lite/delegates/flex:delegate"],
    }),
)

# A convenient library of tflite delegate execution providers for tests based
# on the `tflite_driver` library.
cc_library(
    name = "tflite_driver_delegate_providers",
    deps = [
        "//tensorflow/lite/tools/delegates:coreml_delegate_provider",
        "//tensorflow/lite/tools/delegates:default_execution_provider",
        "//tensorflow/lite/tools/delegates:external_delegate_provider",
        "//tensorflow/lite/tools/delegates:gpu_delegate_provider",
        "//tensorflow/lite/tools/delegates:hexagon_delegate_provider",
        "//tensorflow/lite/tools/delegates:nnapi_delegate_provider",
        "//tensorflow/lite/tools/delegates:xnnpack_delegate_provider",
    ],
    alwayslink = 1,
)

tf_cc_test(
    name = "tflite_driver_test",
    size = "small",
    srcs = ["tflite_driver_test.cc"],
    data = [
        "//tensorflow/lite:testdata/add_quantized_int8.bin",
        "//tensorflow/lite:testdata/multi_add.bin",
    ],
    tags = [
        "tflite_not_portable_android",
        "tflite_not_portable_ios",
    ],
    deps = [
        ":tflite_driver",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "tokenize",
    srcs = ["tokenize.cc"],
    hdrs = ["tokenize.h"],
    deps = [
        "//tensorflow/lite:string",
    ],
)

cc_test(
    name = "tokenize_test",
    srcs = ["tokenize_test.cc"],
    deps = [
        ":tokenize",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_runner",
    hdrs = ["test_runner.h"],
    deps = [
        "//tensorflow/lite:string",
    ],
)

cc_library(
    name = "util",
    hdrs = ["util.h"],
    deps = [
        "//tensorflow/core/platform:logging",
        "//tensorflow/lite:error_reporter",
        "//tensorflow/lite:string",
        "//tensorflow/lite/core/api",
    ],
)

cc_test(
    name = "test_runner_test",
    srcs = ["test_runner_test.cc"],
    deps = [
        ":test_runner",
        "@com_google_googletest//:gtest_main",
    ],
)

tf_cc_binary(
    name = "nnapi_example",
    srcs = ["nnapi_example.cc"],
    deps = [
        ":parse_testdata_lib",
        ":tflite_driver",
        "//tensorflow/lite/nnapi:nnapi_implementation",
    ],
)

cc_library(
    name = "tf_driver",
    srcs = ["tf_driver.cc"],
    hdrs = ["tf_driver.h"],
    features = tf_features_nolayering_check_if_android_or_ios(),
    deps = [
        ":join",
        ":split",
        ":test_runner",
        "@com_google_absl//absl/strings",
        "//tensorflow/lite:string_util",
    ] + select({
        "//conditions:default": [
            "//tensorflow/core:core_cpu",
            "//tensorflow/core:framework",
            "//tensorflow/core:lib",
            "//tensorflow/core:tensorflow",
        ],
        "//tensorflow:android": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
        "//tensorflow:ios": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
    }),
)

tf_cc_test(
    name = "tf_driver_test",
    size = "small",
    srcs = ["tf_driver_test.cc"],
    data = ["//tensorflow/lite:testdata/multi_add.pb"],
    tags = [
        "tflite_not_portable",
    ],
    deps = [
        ":tf_driver",
        "//tensorflow/lite:string_util",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "generate_testspec",
    srcs = ["generate_testspec.cc"],
    hdrs = ["generate_testspec.h"],
    features = tf_features_nolayering_check_if_android_or_ios(),
    deps = [
        ":join",
        ":split",
        ":tf_driver",
        ":tflite_driver",
        "//tensorflow/lite:string",
    ] + select({
        "//conditions:default": [
            "//tensorflow/core:framework",
        ],
        "//tensorflow:android": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
        "//tensorflow:ios": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
    }),
)

tf_cc_test(
    name = "generate_testspec_test",
    size = "small",
    srcs = ["generate_testspec_test.cc"],
    tags = [
        "tflite_not_portable",
    ],
    deps = [
        ":generate_testspec",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "init_tensorflow",
    srcs = [
        "init_tensorflow.cc",
    ],
    hdrs = [
        "init_tensorflow.h",
    ],
    visibility = [
        # copybara:uncomment_begin(internal brella benchmark)
        # "//learning/brain/mobile/lite/brella_benchmark:__subpackages__",
        # copybara:uncomment_end
        "//tensorflow/lite/delegates/flex:__subpackages__",
        "//tensorflow/lite/java/src/main/native:__subpackages__",
        "//tensorflow/lite/testing:__subpackages__",
        "//tensorflow/lite/tools/benchmark:__subpackages__",
    ],
    deps = select({
        "//conditions:default": [
            "//tensorflow/core:lib",
        ],
        "//tensorflow:android": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
        "//tensorflow:ios": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
    }),
)

cc_library(
    name = "tflite_diff_util",
    srcs = ["tflite_diff_util.cc"],
    hdrs = ["tflite_diff_util.h"],
    deps = [
        ":generate_testspec",
        ":parse_testdata_lib",
        ":tflite_driver",
        "//tensorflow/lite:framework",
        "//tensorflow/lite:string",
    ],
)

cc_library(
    name = "tflite_diff_flags",
    hdrs = ["tflite_diff_flags.h"],
    features = tf_features_nolayering_check_if_android_or_ios(),
    deps = [
        ":split",
        ":tflite_diff_util",
        ":tflite_driver",
        "@com_google_absl//absl/strings",
    ] + select({
        "//conditions:default": [
            "//tensorflow/core:framework_internal",
            "//tensorflow/core:lib",
        ],
        "//tensorflow:android": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
        "//tensorflow:ios": [
            "//tensorflow/core:portable_tensorflow_lib",
        ],
    }),
)

tf_cc_test(
    name = "tflite_diff_example_test",
    size = "medium",
    srcs = ["tflite_diff_example_test.cc"],
    args = [
        "--tensorflow_model=third_party/tensorflow/lite/testdata/multi_add.pb",
        "--tflite_model=third_party/tensorflow/lite/testdata/multi_add.bin",
        "--input_layer=a,b,c,d",
        "--input_layer_type=float,float,float,float",
        "--input_layer_shape=1,3,4,3:1,3,4,3:1,3,4,3:1,3,4,3",
        "--output_layer=x,y",
    ],
    data = [
        "//tensorflow/lite:testdata/multi_add.bin",
        "//tensorflow/lite:testdata/multi_add.pb",
    ],
    tags = [
        "no_cuda_on_cpu_tap",
        "no_oss",  # needs test data
        "tflite_not_portable",
    ],
    deps = [
        ":init_tensorflow",
        ":tflite_diff_flags",
        ":tflite_diff_util",
    ],
)

tf_cc_binary(
    name = "tflite_diff",
    srcs = ["tflite_diff_example_test.cc"],
    deps = [
        ":init_tensorflow",
        ":tflite_diff_flags",
        ":tflite_diff_util",
    ],
)

tflite_model_test(
    name = "tflite_model_example_test",
    input_layer = "a,b,c,d",
    input_layer_shape = "1,8,8,3:1,8,8,3:1,8,8,3:1,8,8,3",
    input_layer_type = "float,float,float,float",
    output_layer = "x,y",
    tags = [
        "no_cuda_on_cpu_tap",
        "no_oss",  # needs test data
        "tflite_not_portable",  # TODO(b/134772701): Enable after making this a proper GTest.
    ],
    tensorflow_model_file = "//tensorflow/lite:testdata/multi_add.pb",
)

cc_library(
    name = "string_util_lib",
    srcs = ["string_util.cc"],
    hdrs = ["string_util.h"],
    deps = [
        "//tensorflow/lite:string_util",
        "//tensorflow/lite/python/interpreter_wrapper:numpy",
        "//tensorflow/lite/python/interpreter_wrapper:python_utils",
        "//third_party/py/numpy:headers",
        "//third_party/python_runtime:headers",
        "@com_google_absl//absl/strings",
    ],
)

# A selective built tflite for testing.
tflite_custom_cc_library(
    name = "test_tflite_lib",
    models = [
        "//tensorflow/lite:testdata/add.bin",
        "//tensorflow/lite:testdata/lstm.bin",
    ],
)

cc_test(
    name = "selective_build_test",
    srcs = ["selective_build_test.cc"],
    data = [
        "//tensorflow/lite:testdata/add.bin",
        "//tensorflow/lite:testdata/lstm.bin",
    ],
    tags = [
        "no_mac",  # b/161990368
        "tflite_not_portable",
    ],
    deps = [
        ":test_tflite_lib",
        "//tensorflow/core:tflite_portable_logging",
        "//tensorflow/lite:framework",
        "//tensorflow/lite/c:common",
        "@com_google_googletest//:gtest_main",
    ],
)

pybind_extension(
    name = "_pywrap_string_util",
    srcs = [
        "string_util_wrapper.cc",
    ],
    hdrs = ["string_util.h"],
    features = ["-use_header_modules"],
    module_name = "_pywrap_string_util",
    deps = [
        ":string_util_lib",
        "//tensorflow/lite/python/interpreter_wrapper:numpy",
        "//tensorflow/python:pybind11_lib",
        "//third_party/python_runtime:headers",
        "@pybind11",
    ],
)

tflite_portable_test_suite()

tflite_custom_android_library(
    name = "customized_tflite_for_add_ops",
    models = ["//tensorflow/lite:testdata/add.bin"],
    visibility = ["//visibility:public"],
)

edgetpu_ops = [
    "add",
    "avg_pool",
    "concat",
    "conv",  # high error
    "conv_relu",
    "conv_relu1",
    "conv_relu6",
    "depthwiseconv",  # high error
    "expand_dims",
    "fully_connected",
    "l2norm",  # high error
    "maximum",
    "max_pool",
    "mean",
    "minimum",
    "mul",
    "pad",  # high error
    "pack",
    "relu",
    "relu1",
    "relu6",
    "reshape",
    "resize_bilinear",
    "resize_nearest_neighbor",
    "sigmoid",
    "slice",
    "softmax",
    "space_to_depth",
    "split",
    "squeeze",
    "strided_slice",
    "sub",
    "sum",  # high error
    "tanh",
    "transpose",
    "transpose_conv",
]

# copybara:uncomment_begin(google-only)
# [gen_zipped_test_file(
#     name = "zip_%s_edgetpu" % op_name,
#     file = "%s_edgetpu.zip" % op_name,
#     flags = " --make_edgetpu_tests",
#     toco = "//tensorflow/lite/toco:toco",  # Unused
# ) for op_name in edgetpu_ops]
#
# edgetpu_targets = [":zip_%s_edgetpu" % op_name for op_name in edgetpu_ops]
#
# build_test(
#     name = "gen_edgetpu_tests",
#     targets = edgetpu_targets,
# )
# copybara:uncomment_end
