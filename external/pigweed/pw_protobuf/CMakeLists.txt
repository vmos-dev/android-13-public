# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

include($ENV{PW_ROOT}/pw_build/pigweed.cmake)
include($ENV{PW_ROOT}/pw_protobuf_compiler/proto.cmake)

pw_add_module_config(pw_protobuf_CONFIG)

pw_add_module_library(pw_protobuf.config
  HEADERS
    public/pw_protobuf/config.h
  PUBLIC_INCLUDES
    public
  PUBLIC_DEPS
    ${pw_protobuf_CONFIG}
)

pw_add_module_library(pw_protobuf
  HEADERS
    public/pw_protobuf/decoder.h
    public/pw_protobuf/encoder.h
    public/pw_protobuf/find.h
    public/pw_protobuf/internal/proto_integer_base.h
    public/pw_protobuf/map_utils.h
    public/pw_protobuf/message.h
    public/pw_protobuf/serialized_size.h
    public/pw_protobuf/stream_decoder.h
    public/pw_protobuf/wire_format.h
  PUBLIC_INCLUDES
    public
  PUBLIC_DEPS
    pw_assert
    pw_bytes
    pw_containers.vector
    pw_protobuf.config
    pw_polyfill.span
    pw_polyfill.cstddef
    pw_result
    pw_status
    pw_stream
    pw_stream.interval_reader
    pw_varint
    pw_varint.stream
  SOURCES
    decoder.cc
    encoder.cc
    find.cc
    map_utils.cc
    message.cc
    stream_decoder.cc
)

pw_add_module_library(pw_protobuf.bytes_utils
  HEADERS
    public/pw_protobuf/bytes_utils.h
  PUBLIC_INCLUDES
    public
  PUBLIC_DEPS
    pw_protobuf
    pw_bytes
    pw_result
    pw_status
)

pw_add_test(pw_protobuf.decoder_test
  SOURCES
    decoder_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.encoder_test
  SOURCES
    encoder_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.find_test
  SOURCES
    find_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.codegen_decoder_test
  SOURCES
    codegen_decoder_test.cc
  DEPS
    pw_protobuf
    pw_protobuf.codegen_test_protos.pwpb
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.codegen_encoder_test
  SOURCES
    codegen_encoder_test.cc
  DEPS
    pw_protobuf
    pw_protobuf.codegen_test_protos.pwpb
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.serialized_size_test
  SOURCES
    serialized_size_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.stream_decoder_test
  SOURCES
    stream_decoder_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.map_utils_test
  SOURCES
    map_utils_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_add_test(pw_protobuf.message_test
  SOURCES
    message_test.cc
  DEPS
    pw_protobuf
  GROUPS
    modules
    pw_protobuf
)

pw_proto_library(pw_protobuf.common_protos
  SOURCES
    pw_protobuf_protos/common.proto
)

pw_proto_library(pw_protobuf.codegen_test_protos
  SOURCES
    pw_protobuf_test_protos/full_test.proto
    pw_protobuf_test_protos/imported.proto
    pw_protobuf_test_protos/importer.proto
    pw_protobuf_test_protos/non_pw_package.proto
    pw_protobuf_test_protos/proto2.proto
    pw_protobuf_test_protos/repeated.proto
  DEPS
    pw_protobuf.common_protos
)
