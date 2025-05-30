# Copyright 2018, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Utils for finder classes.
"""

# pylint: disable=line-too-long
# pylint: disable=too-many-lines

from __future__ import print_function

import logging
import multiprocessing
import os
import pickle
import re
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET

import atest_decorator
import atest_error
import atest_utils
import constants

from atest_enum import AtestEnum, DetectType
from metrics import metrics, metrics_utils
from tools import atest_tools

# Helps find apk files listed in a test config (AndroidTest.xml) file.
# Matches "filename.apk" in <option name="foo", value="filename.apk" />
# We want to make sure we don't grab apks with paths in their name since we
# assume the apk name is the build target.
_APK_RE = re.compile(r'^[^/]+\.apk$', re.I)

# Macros that used in GTest. Detailed explanation can be found in
# $ANDROID_BUILD_TOP/external/googletest/googletest/samples/sample*_unittest.cc
# 1. Traditional Tests:
#   TEST(class, method)
#   TEST_F(class, method)
# 2. Type Tests:
#   TYPED_TEST_SUITE(class, types)
#     TYPED_TEST(class, method)
# 3. Value-parameterized Tests:
#   TEST_P(class, method)
#     INSTANTIATE_TEST_SUITE_P(Prefix, class, param_generator, name_generator)
# 4. Type-parameterized Tests:
#   TYPED_TEST_SUITE_P(class)
#     TYPED_TEST_P(class, method)
#       REGISTER_TYPED_TEST_SUITE_P(class, method)
#         INSTANTIATE_TYPED_TEST_SUITE_P(Prefix, class, Types)
# Macros with (class, method) pattern.
_CC_CLASS_METHOD_RE = re.compile(
    r'^\s*(TYPED_TEST(?:|_P)|TEST(?:|_F|_P))\s*\(\s*'
    r'(?P<class_name>\w+),\s*(?P<method_name>\w+)\)\s*\{', re.M)
# Macros with (prefix, class, ...) pattern.
# Note: Since v1.08, the INSTANTIATE_TEST_CASE_P was replaced with
#   INSTANTIATE_TEST_SUITE_P. However, Atest does not intend to change the
#   behavior of a test, so we still search *_CASE_* macros.
_CC_PARAM_CLASS_RE = re.compile(
    r'^\s*INSTANTIATE_(?:|TYPED_)TEST_(?:SUITE|CASE)_P\s*\(\s*'
    r'(?P<instantiate>\w+),\s*(?P<class>\w+)\s*,', re.M)
# Type/Type-parameterized Test macros:
_TYPE_CC_CLASS_RE = re.compile(
    r'^\s*TYPED_TEST_SUITE(?:|_P)\(\s*(?P<class_name>\w+)', re.M)

# Group that matches java/kt method.
_JAVA_METHODS_RE = r'.*\s+(fun|void)\s+(?P<method>\w+)\('
# Parse package name from the package declaration line of a java or
# a kotlin file.
# Group matches "foo.bar" of line "package foo.bar;" or "package foo.bar"
_PACKAGE_RE = re.compile(r'\s*package\s+(?P<package>[^(;|\s)]+)\s*', re.I)
# Matches install paths in module_info to install location(host or device).
_HOST_PATH_RE = re.compile(r'.*\/host\/.*', re.I)
_DEVICE_PATH_RE = re.compile(r'.*\/target\/.*', re.I)
# RE for checking if parameterized java class.
_PARAMET_JAVA_CLASS_RE = re.compile(
    r'^\s*@RunWith\s*\(\s*(Parameterized|TestParameterInjector|'
    r'JUnitParamsRunner|DataProviderRunner|JukitoRunner|Theories|BedsteadJUnit4'
    r').class\s*\)', re.I)
# RE for Java/Kt parent classes:
# Java:   class A extends B {...}
# Kotlin: class A : B (...)
_PARENT_CLS_RE = re.compile(r'.*class\s+\w+\s+(?:extends|:)\s+'
                            r'(?P<parent>[\w\.]+)\s*(?:\{|\()')

# Explanation of FIND_REFERENCE_TYPEs:
# ----------------------------------
# 0. CLASS: Name of a java/kotlin class, usually file is named the same
#    (HostTest lives in HostTest.java or HostTest.kt)
# 1. QUALIFIED_CLASS: Like CLASS but also contains the package in front like
#                     com.android.tradefed.testtype.HostTest.
# 2. PACKAGE: Name of a java package.
# 3. INTEGRATION: XML file name in one of the 4 integration config directories.
# 4. CC_CLASS: Name of a cc class.

FIND_REFERENCE_TYPE = AtestEnum(['CLASS',
                                 'QUALIFIED_CLASS',
                                 'PACKAGE',
                                 'INTEGRATION',
                                 'CC_CLASS'])
# Get cpu count.
_CPU_COUNT = 0 if os.uname()[0] == 'Linux' else multiprocessing.cpu_count()

# Unix find commands for searching for test files based on test type input.
# Note: Find (unlike grep) exits with status 0 if nothing found.
FIND_CMDS = {
    FIND_REFERENCE_TYPE.CLASS: r"find {0} {1} -type f"
                               r"| egrep '.*/{2}\.(kt|java)$' || true",
    FIND_REFERENCE_TYPE.QUALIFIED_CLASS: r"find {0} {1} -type f"
                                         r"| egrep '.*{2}\.(kt|java)$' || true",
    FIND_REFERENCE_TYPE.PACKAGE: r"find {0} {1} -wholename "
                                 r"'*{2}' -type d -print",
    FIND_REFERENCE_TYPE.INTEGRATION: r"find {0} {1} -wholename "
                                     r"'*{2}.xml' -print",
    # Searching a test among files where the absolute paths contain *test*.
    # If users complain atest couldn't find a CC_CLASS, ask them to follow the
    # convention that the filename or dirname must contain *test*, where *test*
    # is case-insensitive.
    FIND_REFERENCE_TYPE.CC_CLASS: r"find {0} {1} -type f -print"
                                  r"| egrep -i '/*test.*\.(cc|cpp)$'"
                                  r"| xargs -P" + str(_CPU_COUNT) + r" egrep -sH"
                                  r" '{}' ".format(constants.CC_GREP_KWRE) +
                                  " || true"
}

# Map ref_type with its index file.
FIND_INDEXES = {
    FIND_REFERENCE_TYPE.CLASS: constants.CLASS_INDEX,
    FIND_REFERENCE_TYPE.QUALIFIED_CLASS: constants.QCLASS_INDEX,
    FIND_REFERENCE_TYPE.PACKAGE: constants.PACKAGE_INDEX,
    FIND_REFERENCE_TYPE.INTEGRATION: constants.INT_INDEX,
    FIND_REFERENCE_TYPE.CC_CLASS: constants.CC_CLASS_INDEX
}

# XML parsing related constants.
_COMPATIBILITY_PACKAGE_PREFIX = "com.android.compatibility"
_XML_PUSH_DELIM = '->'
_APK_SUFFIX = '.apk'
DALVIK_TEST_RUNNER_CLASS = 'com.android.compatibility.testtype.DalvikTest'
LIBCORE_TEST_RUNNER_CLASS = 'com.android.compatibility.testtype.LibcoreTest'
DALVIK_TESTRUNNER_JAR_CLASSES = [DALVIK_TEST_RUNNER_CLASS,
                                 LIBCORE_TEST_RUNNER_CLASS]
DALVIK_DEVICE_RUNNER_JAR = 'cts-dalvik-device-test-runner'
DALVIK_HOST_RUNNER_JAR = 'cts-dalvik-host-test-runner'
DALVIK_TEST_DEPS = {DALVIK_DEVICE_RUNNER_JAR,
                    DALVIK_HOST_RUNNER_JAR,
                    constants.CTS_JAR}
# Setup script for device perf tests.
_PERF_SETUP_LABEL = 'perf-setup.sh'
_PERF_SETUP_TARGET = 'perf-setup'

# XML tags.
_XML_NAME = 'name'
_XML_VALUE = 'value'

# VTS xml parsing constants.
_VTS_TEST_MODULE = 'test-module-name'
_VTS_MODULE = 'module-name'
_VTS_BINARY_SRC = 'binary-test-source'
_VTS_PUSH_GROUP = 'push-group'
_VTS_PUSH = 'push'
_VTS_BINARY_SRC_DELIM = '::'
_VTS_PUSH_DIR = os.path.join(os.environ.get(constants.ANDROID_BUILD_TOP, ''),
                             'test', 'vts', 'tools', 'vts-tradefed', 'res',
                             'push_groups')
_VTS_PUSH_SUFFIX = '.push'
_VTS_BITNESS = 'append-bitness'
_VTS_BITNESS_TRUE = 'true'
_VTS_BITNESS_32 = '32'
_VTS_BITNESS_64 = '64'
_VTS_TEST_FILE = 'test-file-name'
_VTS_APK = 'apk'
# Matches 'DATA/target' in '_32bit::DATA/target'
_VTS_BINARY_SRC_DELIM_RE = re.compile(r'.*::(?P<target>.*)$')
_VTS_OUT_DATA_APP_PATH = 'DATA/app'

# pylint: disable=inconsistent-return-statements
def split_methods(user_input):
    """Split user input string into test reference and list of methods.

    Args:
        user_input: A string of the user's input.
                    Examples:
                        class_name
                        class_name#method1,method2
                        path
                        path#method1,method2
    Returns:
        A tuple. First element is String of test ref and second element is
        a set of method name strings or empty list if no methods included.
    Exception:
        atest_error.TooManyMethodsError raised when input string is trying to
        specify too many methods in a single positional argument.

        Examples of unsupported input strings:
            module:class#method,class#method
            class1#method,class2#method
            path1#method,path2#method
    """
    parts = user_input.split('#')
    if len(parts) == 1:
        return parts[0], frozenset()
    if len(parts) == 2:
        return parts[0], frozenset(parts[1].split(','))
    raise atest_error.TooManyMethodsError(
        'Too many methods specified with # character in user input: %s.'
        '\n\nOnly one class#method combination supported per positional'
        ' argument. Multiple classes should be separated by spaces: '
        'class#method class#method')


# pylint: disable=inconsistent-return-statements
def get_fully_qualified_class_name(test_path):
    """Parse the fully qualified name from the class java file.

    Args:
        test_path: A string of absolute path to the java class file.

    Returns:
        A string of the fully qualified class name.

    Raises:
        atest_error.MissingPackageName if no class name can be found.
    """
    with open(test_path) as class_file:
        for line in class_file:
            match = _PACKAGE_RE.match(line)
            if match:
                package = match.group('package')
                cls = os.path.splitext(os.path.split(test_path)[1])[0]
                return '%s.%s' % (package, cls)
    raise atest_error.MissingPackageNameError('%s: Test class java file'
                                              'does not contain a package'
                                              'name.'% test_path)


def has_cc_class(test_path):
    """Find out if there is any test case in the cc file.

    Args:
        test_path: A string of absolute path to the cc file.

    Returns:
        Boolean: has cc class in test_path or not.
    """
    with open(test_path) as class_file:
        content = class_file.read()
        if re.findall(_CC_CLASS_METHOD_RE, content):
            return True
    return False


def get_package_name(file_name):
    """Parse the package name from a java file.

    Args:
        file_name: A string of the absolute path to the java file.

    Returns:
        A string of the package name or None
    """
    with open(file_name) as data:
        for line in data:
            match = _PACKAGE_RE.match(line)
            if match:
                return match.group('package')


def get_parent_cls_name(file_name):
    """Parse the parent class name from a java/kt file.

    Args:
        file_name: A string of the absolute path to the javai/kt file.

    Returns:
        A string of the parent class name or None
    """
    with open(file_name) as data:
        for line in data:
            match = _PARENT_CLS_RE.match(line)
            if match:
                return match.group('parent')


def get_java_parent_paths(test_path):
    """Find out the paths of parent classes, including itself.

    Args:
        test_path: A string of absolute path to the test file.

    Returns:
        A set of test paths.
    """
    all_parent_test_paths = set([test_path])
    parent = get_parent_cls_name(test_path)
    if not parent:
        return all_parent_test_paths
    # Remove <Generics> if any.
    parent_cls = re.sub(r'\<\w+\>', '', parent)
    package = get_package_name(test_path)
    # Use Fully Qualified Class Name for searching precisely.
    # package org.gnome;
    # public class Foo extends com.android.Boo -> com.android.Boo
    # public class Foo extends Boo -> org.gnome.Boo
    if '.' in parent_cls:
        parent_fqcn = parent_cls
    else:
        parent_fqcn = package + '.' + parent_cls
    parent_test_paths = run_find_cmd(
        FIND_REFERENCE_TYPE.QUALIFIED_CLASS,
        os.environ.get(constants.ANDROID_BUILD_TOP),
        parent_fqcn)
    # Recursively search parent classes until the class is not found.
    if parent_test_paths:
        for parent_test_path in parent_test_paths:
            all_parent_test_paths |= get_java_parent_paths(parent_test_path)
    return all_parent_test_paths


def has_method_in_file(test_path, methods):
    """Find out if every method can be found in the file.

    Note: This method doesn't handle if method is in comment sections.

    Args:
        test_path: A string of absolute path to the test file.
        methods: A set of method names.

    Returns:
        Boolean: there is at least one method in test_path.
    """
    if not os.path.isfile(test_path):
        return False
    all_methods = set()
    if constants.JAVA_EXT_RE.match(test_path):
        # omit parameterized pattern: method[0]
        _methods = set(re.sub(r'\[\S+\]', '', x) for x in methods)
        # Return True when every method is in the same Java file.
        if _methods.issubset(get_java_methods(test_path)):
            return True
        # Otherwise, search itself and all the parent classes respectively
        # to get all test names.
        parent_test_paths = get_java_parent_paths(test_path)
        logging.debug('Will search methods %s in %s\n',
                      _methods, parent_test_paths)
        for path in parent_test_paths:
            all_methods |= get_java_methods(path)
        if _methods.issubset(all_methods):
            return True
        # If cannot find all methods, override the test_path for debugging.
        test_path = parent_test_paths
    elif constants.CC_EXT_RE.match(test_path):
        # omit parameterized pattern: method/argument
        _methods = set(re.sub(r'\/.*', '', x) for x in methods)
        class_info = get_cc_class_info(test_path)
        for info in class_info.values():
            all_methods |= info.get('methods')
        if _methods.issubset(all_methods):
            return True
    missing_methods = _methods - all_methods
    logging.debug('Cannot find methods %s in %s',
        atest_utils.colorize(','.join(missing_methods), constants.RED),
        test_path)
    return False


def extract_test_path(output, methods=None):
    """Extract the test path from the output of a unix 'find' command.

    Example of find output for CLASS find cmd:
    /<some_root>/cts/tests/jank/src/android/jank/cts/ui/CtsDeviceJankUi.java

    Args:
        output: A string or list output of a unix 'find' command.
        methods: A set of method names.

    Returns:
        A list of the test paths or None if output is '' or None.
    """
    if not output:
        return None
    verified_tests = set()
    if isinstance(output, str):
        output = output.splitlines()
    for test in output:
        match_obj = constants.CC_OUTPUT_RE.match(test)
        # Legacy "find" cc output (with TEST_P() syntax):
        if match_obj:
            fpath = match_obj.group('file_path')
            if not methods or match_obj.group('method_name') in methods:
                verified_tests.add(fpath)
        # "locate" output path for both java/cc.
        elif not methods or has_method_in_file(test, methods):
            verified_tests.add(test)
    return extract_test_from_tests(sorted(list(verified_tests)))


def extract_test_from_tests(tests, default_all=False):
    """Extract the test path from the tests.

    Return the test to run from tests. If more than one option, prompt the user
    to select multiple ones. Supporting formats:
    - An integer. E.g. 0
    - Comma-separated integers. E.g. 1,3,5
    - A range of integers denoted by the starting integer separated from
      the end integer by a dash, '-'. E.g. 1-3

    Args:
        tests: A string list which contains multiple test paths.

    Returns:
        A string list of paths.
    """
    count = len(tests)
    if default_all or count <= 1:
        return tests if count else None
    mtests = set()
    try:
        numbered_list = ['%s: %s' % (i, t) for i, t in enumerate(tests)]
        numbered_list.append('%s: All' % count)
        print('Multiple tests found:\n{0}'.format('\n'.join(numbered_list)))
        test_indices = input("Please enter numbers of test to use. If none of "
                             "above option matched, keep searching for other "
                             "possible tests.\n(multiple selection is supported, "
                             "e.g. '1' or '0,1' or '0-2'): ")
        for idx in re.sub(r'(\s)', '', test_indices).split(','):
            indices = idx.split('-')
            len_indices = len(indices)
            if len_indices > 0:
                start_index = min(int(indices[0]), int(indices[len_indices-1]))
                end_index = max(int(indices[0]), int(indices[len_indices-1]))
                # One of input is 'All', return all options.
                if count in (start_index, end_index):
                    return tests
                mtests.update(tests[start_index:(end_index+1)])
    except (ValueError, IndexError, AttributeError, TypeError) as err:
        logging.debug('%s', err)
        print('None of above option matched, keep searching for other'
              ' possible tests...')
    return list(mtests)


@atest_decorator.static_var("cached_ignore_dirs", [])
def _get_ignored_dirs():
    """Get ignore dirs in find command.

    Since we can't construct a single find cmd to find the target and
    filter-out the dir with .out-dir, .find-ignore and $OUT-DIR. We have
    to run the 1st find cmd to find these dirs. Then, we can use these
    results to generate the real find cmd.

    Return:
        A list of the ignore dirs.
    """
    out_dirs = _get_ignored_dirs.cached_ignore_dirs
    if not out_dirs:
        build_top = os.environ.get(constants.ANDROID_BUILD_TOP)
        find_out_dir_cmd = (r'find %s -maxdepth 2 '
                            r'-type f \( -name ".out-dir" -o -name '
                            r'".find-ignore" \)') % build_top
        out_files = subprocess.check_output(find_out_dir_cmd, shell=True)
        if isinstance(out_files, bytes):
            out_files = out_files.decode()
        # Get all dirs with .out-dir or .find-ignore
        if out_files:
            out_files = out_files.splitlines()
            for out_file in out_files:
                if out_file:
                    out_dirs.append(os.path.dirname(out_file.strip()))
        # Get the out folder if user specified $OUT_DIR
        custom_out_dir = os.environ.get(constants.ANDROID_OUT_DIR)
        if custom_out_dir:
            user_out_dir = None
            if os.path.isabs(custom_out_dir):
                user_out_dir = custom_out_dir
            else:
                user_out_dir = os.path.join(build_top, custom_out_dir)
            # only ignore the out_dir when it under $ANDROID_BUILD_TOP
            if build_top in user_out_dir:
                if user_out_dir not in out_dirs:
                    out_dirs.append(user_out_dir)
        _get_ignored_dirs.cached_ignore_dirs = out_dirs
    return out_dirs


def _get_prune_cond_of_ignored_dirs():
    """Get the prune condition of ignore dirs.

    Generation a string of the prune condition in the find command.
    It will filter-out the dir with .out-dir, .find-ignore and $OUT-DIR.
    Because they are the out dirs, we don't have to find them.

    Return:
        A string of the prune condition of the ignore dirs.
    """
    out_dirs = _get_ignored_dirs()
    prune_cond = r'-type d \( -name ".*"'
    for out_dir in out_dirs:
        prune_cond += r' -o -path %s' % out_dir
    prune_cond += r' \) -prune -o'
    return prune_cond


def run_find_cmd(ref_type, search_dir, target, methods=None):
    """Find a path to a target given a search dir and a target name.

    Args:
        ref_type: An AtestEnum of the reference type.
        search_dir: A string of the dirpath to search in.
        target: A string of what you're trying to find.
        methods: A set of method names.

    Return:
        A list of the path to the target.
        If the search_dir is inexistent, None will be returned.
    """
    # If module_info.json is outdated, finding in the search_dir can result in
    # raising exception. Return null immediately can guild users to run
    # --rebuild-module-info to resolve the problem.
    if not os.path.isdir(search_dir):
        logging.debug('\'%s\' does not exist!', search_dir)
        return None
    ref_name = FIND_REFERENCE_TYPE[ref_type]
    start = time.time()
    if os.path.isfile(FIND_INDEXES[ref_type]):
        _dict, out = {}, None
        with open(FIND_INDEXES[ref_type], 'rb') as index:
            try:
                _dict = pickle.load(index, encoding='utf-8')
            except (TypeError, IOError, EOFError, pickle.UnpicklingError) as err:
                logging.debug('Exception raised: %s', err)
                metrics_utils.handle_exc_and_send_exit_event(
                    constants.ACCESS_CACHE_FAILURE)
                os.remove(FIND_INDEXES[ref_type])
        if _dict.get(target):
            out = [path for path in _dict.get(target) if search_dir in path]
            logging.debug('Found %s in %s', target, out)
    else:
        prune_cond = _get_prune_cond_of_ignored_dirs()
        if '.' in target:
            target = target.replace('.', '/')
        find_cmd = FIND_CMDS[ref_type].format(search_dir, prune_cond, target)
        logging.debug('Executing %s find cmd: %s', ref_name, find_cmd)
        out = subprocess.check_output(find_cmd, shell=True)
        if isinstance(out, bytes):
            out = out.decode()
        logging.debug('%s find cmd out: %s', ref_name, out)
    logging.debug('%s find completed in %ss', ref_name, time.time() - start)
    return extract_test_path(out, methods)


def find_class_file(search_dir, class_name, is_native_test=False, methods=None):
    """Find a path to a class file given a search dir and a class name.

    Args:
        search_dir: A string of the dirpath to search in.
        class_name: A string of the class to search for.
        is_native_test: A boolean variable of whether to search for a native
        test or not.
        methods: A set of method names.

    Return:
        A list of the path to the java/cc file.
    """
    if is_native_test:
        ref_type = FIND_REFERENCE_TYPE.CC_CLASS
    elif '.' in class_name:
        ref_type = FIND_REFERENCE_TYPE.QUALIFIED_CLASS
    else:
        ref_type = FIND_REFERENCE_TYPE.CLASS
    return run_find_cmd(ref_type, search_dir, class_name, methods)


def is_equal_or_sub_dir(sub_dir, parent_dir):
    """Return True sub_dir is sub dir or equal to parent_dir.

    Args:
      sub_dir: A string of the sub directory path.
      parent_dir: A string of the parent directory path.

    Returns:
        A boolean of whether both are dirs and sub_dir is sub of parent_dir
        or is equal to parent_dir.
    """
    # avoid symlink issues with real path
    parent_dir = os.path.realpath(parent_dir)
    sub_dir = os.path.realpath(sub_dir)
    if not os.path.isdir(sub_dir) or not os.path.isdir(parent_dir):
        return False
    return os.path.commonprefix([sub_dir, parent_dir]) == parent_dir


def find_parent_module_dir(root_dir, start_dir, module_info):
    """From current dir search up file tree until root dir for module dir.

    Args:
        root_dir: A string  of the dir that is the parent of the start dir.
        start_dir: A string of the dir to start searching up from.
        module_info: ModuleInfo object containing module information from the
                     build system.

    Returns:
        A string of the module dir relative to root, None if no Module Dir
        found. There may be multiple testable modules at this level.

    Exceptions:
        ValueError: Raised if cur_dir not dir or not subdir of root dir.
    """
    if not is_equal_or_sub_dir(start_dir, root_dir):
        raise ValueError('%s not in repo %s' % (start_dir, root_dir))
    auto_gen_dir = None
    current_dir = start_dir
    while current_dir != root_dir:
        # TODO (b/112904944) - migrate module_finder functions to here and
        # reuse them.
        rel_dir = os.path.relpath(current_dir, root_dir)
        # Check if actual config file here but need to make sure that there
        # exist module in module-info with the parent dir.
        if (os.path.isfile(os.path.join(current_dir, constants.MODULE_CONFIG))
                and module_info.get_module_names(current_dir)):
            return rel_dir
        # Check module_info if auto_gen config or robo (non-config) here
        for mod in module_info.path_to_module_info.get(rel_dir, []):
            if module_info.is_robolectric_module(mod):
                return rel_dir
            for test_config in mod.get(constants.MODULE_TEST_CONFIG, []):
                # If the test config doesn's exist until it was auto-generated
                # in the build time(under <android_root>/out), atest still
                # recognizes it testable.
                if test_config:
                    return rel_dir
            if mod.get('auto_test_config'):
                auto_gen_dir = rel_dir
                # Don't return for auto_gen, keep checking for real config,
                # because common in cts for class in apk that's in hostside
                # test setup.
        current_dir = os.path.dirname(current_dir)
    return auto_gen_dir


def get_targets_from_xml(xml_file, module_info):
    """Retrieve build targets from the given xml.

    Just a helper func on top of get_targets_from_xml_root.

    Args:
        xml_file: abs path to xml file.
        module_info: ModuleInfo class used to verify targets are valid modules.

    Returns:
        A set of build targets based on the signals found in the xml file.
    """
    xml_root = ET.parse(xml_file).getroot()
    return get_targets_from_xml_root(xml_root, module_info)


def _get_apk_target(apk_target):
    """Return the sanitized apk_target string from the xml.

    The apk_target string can be of 2 forms:
      - apk_target.apk
      - apk_target.apk->/path/to/install/apk_target.apk

    We want to return apk_target in both cases.

    Args:
        apk_target: String of target name to clean.

    Returns:
        String of apk_target to build.
    """
    apk = apk_target.split(_XML_PUSH_DELIM, 1)[0].strip()
    return apk[:-len(_APK_SUFFIX)]


def _is_apk_target(name, value):
    """Return True if XML option is an apk target.

    We have some scenarios where an XML option can be an apk target:
      - value is an apk file.
      - name is a 'push' option where value holds the apk_file + other stuff.

    Args:
        name: String name of XML option.
        value: String value of the XML option.

    Returns:
        True if it's an apk target we should build, False otherwise.
    """
    if _APK_RE.match(value):
        return True
    if name == 'push' and value.endswith(_APK_SUFFIX):
        return True
    return False


def get_targets_from_xml_root(xml_root, module_info):
    """Retrieve build targets from the given xml root.

    We're going to pull the following bits of info:
      - Parse any .apk files listed in the config file.
      - Parse option value for "test-module-name" (for vts10 tests).
      - Look for the perf script.

    Args:
        module_info: ModuleInfo class used to verify targets are valid modules.
        xml_root: ElementTree xml_root for us to look through.

    Returns:
        A set of build targets based on the signals found in the xml file.
    """
    targets = set()
    option_tags = xml_root.findall('.//option')
    for tag in option_tags:
        target_to_add = None
        name = tag.attrib[_XML_NAME].strip()
        value = tag.attrib[_XML_VALUE].strip()
        if _is_apk_target(name, value):
            target_to_add = _get_apk_target(value)
        elif _PERF_SETUP_LABEL in value:
            target_to_add = _PERF_SETUP_TARGET

        # Let's make sure we can actually build the target.
        if target_to_add and module_info.is_module(target_to_add):
            targets.add(target_to_add)
        elif target_to_add:
            logging.debug('Build target (%s) not present in module info, '
                          'skipping build', target_to_add)

    # TODO (b/70813166): Remove this lookup once all runtime dependencies
    # can be listed as a build dependencies or are in the base test harness.
    nodes_with_class = xml_root.findall(".//*[@class]")
    for class_attr in nodes_with_class:
        fqcn = class_attr.attrib['class'].strip()
        if fqcn.startswith(_COMPATIBILITY_PACKAGE_PREFIX):
            targets.add(constants.CTS_JAR)
        if fqcn in DALVIK_TESTRUNNER_JAR_CLASSES:
            for dalvik_dep in DALVIK_TEST_DEPS:
                if module_info.is_module(dalvik_dep):
                    targets.add(dalvik_dep)
    logging.debug('Targets found in config file: %s', targets)
    return targets


def _get_vts_push_group_targets(push_file, rel_out_dir):
    """Retrieve vts10 push group build targets.

    A push group file is a file that list out test dependencies and other push
    group files. Go through the push file and gather all the test deps we need.

    Args:
        push_file: Name of the push file in the VTS
        rel_out_dir: Abs path to the out dir to help create vts10 build targets.

    Returns:
        Set of string which represent build targets.
    """
    targets = set()
    full_push_file_path = os.path.join(_VTS_PUSH_DIR, push_file)
    # pylint: disable=invalid-name
    with open(full_push_file_path) as f:
        for line in f:
            target = line.strip()
            # Skip empty lines.
            if not target:
                continue

            # This is a push file, get the targets from it.
            if target.endswith(_VTS_PUSH_SUFFIX):
                targets |= _get_vts_push_group_targets(line.strip(),
                                                       rel_out_dir)
                continue
            sanitized_target = target.split(_XML_PUSH_DELIM, 1)[0].strip()
            targets.add(os.path.join(rel_out_dir, sanitized_target))
    return targets


def _specified_bitness(xml_root):
    """Check if the xml file contains the option append-bitness.

    Args:
        xml_root: abs path to xml file.

    Returns:
        True if xml specifies to append-bitness, False otherwise.
    """
    option_tags = xml_root.findall('.//option')
    for tag in option_tags:
        value = tag.attrib[_XML_VALUE].strip()
        name = tag.attrib[_XML_NAME].strip()
        if name == _VTS_BITNESS and value == _VTS_BITNESS_TRUE:
            return True
    return False


def _get_vts_binary_src_target(value, rel_out_dir):
    """Parse out the vts10 binary src target.

    The value can be in the following pattern:
      - {_32bit,_64bit,_IPC32_32bit}::DATA/target (DATA/target)
      - DATA/target->/data/target (DATA/target)
      - out/host/linx-x86/bin/VtsSecuritySelinuxPolicyHostTest (the string as
        is)

    Args:
        value: String of the XML option value to parse.
        rel_out_dir: String path of out dir to prepend to target when required.

    Returns:
        String of the target to build.
    """
    # We'll assume right off the bat we can use the value as is and modify it if
    # necessary, e.g. out/host/linux-x86/bin...
    target = value
    # _32bit::DATA/target
    match = _VTS_BINARY_SRC_DELIM_RE.match(value)
    if match:
        target = os.path.join(rel_out_dir, match.group('target'))
    # DATA/target->/data/target
    elif _XML_PUSH_DELIM in value:
        target = value.split(_XML_PUSH_DELIM, 1)[0].strip()
        target = os.path.join(rel_out_dir, target)
    return target


def get_plans_from_vts_xml(xml_file):
    """Get configs which are included by xml_file.

    We're looking for option(include) to get all dependency plan configs.

    Args:
        xml_file: Absolute path to xml file.

    Returns:
        A set of plan config paths which are depended by xml_file.
    """
    if not os.path.exists(xml_file):
        raise atest_error.XmlNotExistError('%s: The xml file does'
                                           'not exist' % xml_file)
    plans = set()
    xml_root = ET.parse(xml_file).getroot()
    plans.add(xml_file)
    option_tags = xml_root.findall('.//include')
    if not option_tags:
        return plans
    # Currently, all vts10 xmls live in the same dir :
    # https://android.googlesource.com/platform/test/vts/+/master/tools/vts-tradefed/res/config/
    # If the vts10 plans start using folders to organize the plans, the logic here
    # should be changed.
    xml_dir = os.path.dirname(xml_file)
    for tag in option_tags:
        name = tag.attrib[_XML_NAME].strip()
        plans |= get_plans_from_vts_xml(os.path.join(xml_dir, name + ".xml"))
    return plans


def get_targets_from_vts_xml(xml_file, rel_out_dir, module_info):
    """Parse a vts10 xml for test dependencies we need to build.

    We have a separate vts10 parsing function because we make a big assumption
    on the targets (the way they're formatted and what they represent) and we
    also create these build targets in a very special manner as well.
    The 6 options we're looking for are:
      - binary-test-source
      - push-group
      - push
      - test-module-name
      - test-file-name
      - apk

    Args:
        module_info: ModuleInfo class used to verify targets are valid modules.
        rel_out_dir: Abs path to the out dir to help create vts10 build targets.
        xml_file: abs path to xml file.

    Returns:
        A set of build targets based on the signals found in the xml file.
    """
    xml_root = ET.parse(xml_file).getroot()
    targets = set()
    option_tags = xml_root.findall('.//option')
    for tag in option_tags:
        value = tag.attrib[_XML_VALUE].strip()
        name = tag.attrib[_XML_NAME].strip()
        if name in [_VTS_TEST_MODULE, _VTS_MODULE]:
            if module_info.is_module(value):
                targets.add(value)
            else:
                logging.debug('vts10 test module (%s) not present in module '
                              'info, skipping build', value)
        elif name == _VTS_BINARY_SRC:
            targets.add(_get_vts_binary_src_target(value, rel_out_dir))
        elif name == _VTS_PUSH_GROUP:
            # Look up the push file and parse out build artifacts (as well as
            # other push group files to parse).
            targets |= _get_vts_push_group_targets(value, rel_out_dir)
        elif name == _VTS_PUSH:
            # Parse out the build artifact directly.
            push_target = value.split(_XML_PUSH_DELIM, 1)[0].strip()
            # If the config specified append-bitness, append the bits suffixes
            # to the target.
            if _specified_bitness(xml_root):
                targets.add(os.path.join(
                    rel_out_dir, push_target + _VTS_BITNESS_32))
                targets.add(os.path.join(
                    rel_out_dir, push_target + _VTS_BITNESS_64))
            else:
                targets.add(os.path.join(rel_out_dir, push_target))
        elif name == _VTS_TEST_FILE:
            # The _VTS_TEST_FILE values can be set in 2 possible ways:
            #   1. test_file.apk
            #   2. DATA/app/test_file/test_file.apk
            # We'll assume that test_file.apk (#1) is in an expected path (but
            # that is not true, see b/76158619) and create the full path for it
            # and then append the _VTS_TEST_FILE value to targets to build.
            target = os.path.join(rel_out_dir, value)
            # If value is just an APK, specify the path that we expect it to be in
            # e.g. out/host/linux-x86/vts10/android-vts10/testcases/DATA/app/test_file/test_file.apk
            head, _ = os.path.split(value)
            if not head:
                target = os.path.join(rel_out_dir, _VTS_OUT_DATA_APP_PATH,
                                      _get_apk_target(value), value)
            targets.add(target)
        elif name == _VTS_APK:
            targets.add(os.path.join(rel_out_dir, value))
    logging.debug('Targets found in config file: %s', targets)
    return targets


def get_dir_path_and_filename(path):
    """Return tuple of dir and file name from given path.

    Args:
        path: String of path to break up.

    Returns:
        Tuple of (dir, file) paths.
    """
    if os.path.isfile(path):
        dir_path, file_path = os.path.split(path)
    else:
        dir_path, file_path = path, None
    return dir_path, file_path


def get_cc_filter(class_info, class_name, methods):
    """Get the cc filter.

    Args:
        class_info: a dict of class info.
        class_name: class name of the cc test.
        methods: a list of method names.

    Returns:
        A formatted string for cc filter.
        For a Type/Typed-parameterized test, it will be:
          "class1/*.method1:class1/*.method2" or "class1/*.*"
        For a parameterized test, it will be:
          "*/class1.*" or "prefix/class1.*"
        For the rest the pattern will be:
          "class1.method1:class1.method2" or "class1.*"
    """
    #Strip prefix from class_name.
    _class_name = class_name
    if '/' in class_name:
        _class_name = str(class_name).split('/')[-1]
    type_str = get_cc_class_type(class_info, _class_name)
    logging.debug('%s is a "%s".', _class_name, type_str)
    # When found parameterized tests, recompose the class name
    # in */$(ClassName) if the prefix is not given.
    if type_str in (constants.GTEST_TYPED_PARAM, constants.GTEST_PARAM):
        if not '/' in class_name:
            class_name = '*/%s' % class_name
    if type_str in (constants.GTEST_TYPED, constants.GTEST_TYPED_PARAM):
        if methods:
            sorted_methods = sorted(list(methods))
            return ":".join(["%s/*.%s" % (class_name, x) for x in sorted_methods])
        return "%s/*.*" % class_name
    if methods:
        sorted_methods = sorted(list(methods))
        return ":".join(["%s.%s" % (class_name, x) for x in sorted_methods])
    return "%s.*" % class_name


def search_integration_dirs(name, int_dirs):
    """Search integration dirs for name and return full path.

    Args:
        name: A string of plan name needed to be found.
        int_dirs: A list of path needed to be searched.

    Returns:
        A list of the test path.
        Ask user to select if multiple tests are found.
        None if no matched test found.
    """
    root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
    test_files = []
    for integration_dir in int_dirs:
        abs_path = os.path.join(root_dir, integration_dir)
        test_paths = run_find_cmd(FIND_REFERENCE_TYPE.INTEGRATION, abs_path,
                                  name)
        if test_paths:
            test_files.extend(test_paths)
    return extract_test_from_tests(test_files)


def get_int_dir_from_path(path, int_dirs):
    """Search integration dirs for the given path and return path of dir.

    Args:
        path: A string of path needed to be found.
        int_dirs: A list of path needed to be searched.

    Returns:
        A string of the test dir. None if no matched path found.
    """
    root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
    if not os.path.exists(path):
        return None
    dir_path, file_name = get_dir_path_and_filename(path)
    int_dir = None
    for possible_dir in int_dirs:
        abs_int_dir = os.path.join(root_dir, possible_dir)
        if is_equal_or_sub_dir(dir_path, abs_int_dir):
            int_dir = abs_int_dir
            break
    if not file_name:
        logging.debug('Found dir (%s) matching input (%s).'
                      ' Referencing an entire Integration/Suite dir'
                      ' is not supported. If you are trying to reference'
                      ' a test by its path, please input the path to'
                      ' the integration/suite config file itself.',
                      int_dir, path)
        return None
    return int_dir


def get_install_locations(installed_paths):
    """Get install locations from installed paths.

    Args:
        installed_paths: List of installed_paths from module_info.

    Returns:
        Set of install locations from module_info installed_paths. e.g.
        set(['host', 'device'])
    """
    install_locations = set()
    for path in installed_paths:
        if _HOST_PATH_RE.match(path):
            install_locations.add(constants.DEVICELESS_TEST)
        elif _DEVICE_PATH_RE.match(path):
            install_locations.add(constants.DEVICE_TEST)
    return install_locations


def get_levenshtein_distance(test_name, module_name,
                             dir_costs=constants.COST_TYPO):
    """Return an edit distance between test_name and module_name.

    Levenshtein Distance has 3 actions: delete, insert and replace.
    dis_costs makes each action weigh differently.

    Args:
        test_name: A keyword from the users.
        module_name: A testable module name.
        dir_costs: A tuple which contains 3 integer, where dir represents
                   Deletion, Insertion and Replacement respectively.
                   For guessing typos: (1, 1, 1) gives the best result.
                   For searching keywords, (8, 1, 5) gives the best result.

    Returns:
        An edit distance integer between test_name and module_name.
    """
    rows = len(test_name) + 1
    cols = len(module_name) + 1
    deletion, insertion, replacement = dir_costs

    # Creating a Dynamic Programming Matrix and weighting accordingly.
    dp_matrix = [[0 for _ in range(cols)] for _ in range(rows)]
    # Weigh rows/deletion
    for row in range(1, rows):
        dp_matrix[row][0] = row * deletion
    # Weigh cols/insertion
    for col in range(1, cols):
        dp_matrix[0][col] = col * insertion
    # The core logic of LD
    for col in range(1, cols):
        for row in range(1, rows):
            if test_name[row-1] == module_name[col-1]:
                cost = 0
            else:
                cost = replacement
            dp_matrix[row][col] = min(dp_matrix[row-1][col] + deletion,
                                      dp_matrix[row][col-1] + insertion,
                                      dp_matrix[row-1][col-1] + cost)

    return dp_matrix[row][col]


def is_test_from_kernel_xml(xml_file, test_name):
    """Check if test defined in xml_file.

    A kernel test can be defined like:
    <option name="test-command-line" key="test_class_1" value="command 1" />
    where key is the name of test class and method of the runner. This method
    returns True if the test_name was defined in the given xml_file.

    Args:
        xml_file: Absolute path to xml file.
        test_name: test_name want to find.

    Returns:
        True if test_name in xml_file, False otherwise.
    """
    if not os.path.exists(xml_file):
        return False
    xml_root = ET.parse(xml_file).getroot()
    option_tags = xml_root.findall('.//option')
    for option_tag in option_tags:
        if option_tag.attrib['name'] == 'test-command-line':
            if option_tag.attrib['key'] == test_name:
                return True
    return False


def is_parameterized_java_class(test_path):
    """Find out if input test path is a parameterized java class.

    Args:
        test_path: A string of absolute path to the java file.

    Returns:
        Boolean: Is parameterized class or not.
    """
    with open(test_path) as class_file:
        for line in class_file:
            match = _PARAMET_JAVA_CLASS_RE.match(line)
            if match:
                return True
    return False


def get_java_methods(test_path):
    """Find out the java test class of input test_path.

    Args:
        test_path: A string of absolute path to the java file.

    Returns:
        A set of methods.
    """
    logging.debug('Probing %s:', test_path)
    with open(test_path) as class_file:
        content = class_file.read()
    matches = re.findall(_JAVA_METHODS_RE, content)
    if matches:
        methods = {match[1] for match in matches}
        logging.debug('Available methods: %s\n', methods)
        return methods
    return set()


# pylint: disable=too-many-branches
def get_cc_class_info(test_path):
    """Get the class info of the given cc input test_path.

    The class info dict will be like:
        {'classA': {
            'methods': {'m1', 'm2'}, 'prefixes': {'pfx1'}, 'typed': True},
         'classB': {
            'methods': {'m3', 'm4'}, 'prefixes': set(), 'typed': False},
         'classC': {
            'methods': {'m5', 'm6'}, 'prefixes': set(), 'typed': True},
         'classD': {
            'methods': {'m7', 'm8'}, 'prefixes': {'pfx3'}, 'typed': False}}
    According to the class info, we can tell that:
        classA is a typed-parameterized test. (TYPED_TEST_SUITE_P)
        classB is a regular gtest.            (TEST_F|TEST)
        classC is a typed test.               (TYPED_TEST_SUITE)
        classD is a value-parameterized test. (TEST_P)

    Args:
        test_path: A string of absolute path to the cc file.

    Returns:
        A dict of class info.
    """
    # Strip comments prior to parsing class and method names if possible.
    _test_path = tempfile.NamedTemporaryFile()
    if atest_tools.has_command('gcc'):
        strip_comment_cmd = 'gcc -fpreprocessed -dD -E {} > {}'
        if subprocess.getoutput(
            strip_comment_cmd.format(test_path, _test_path.name)):
            logging.debug('Failed to strip comments in %s. Parsing class/method'
                          'names may not be accurate.', test_path)
    file_to_parse = test_path
    # If failed to strip comments, it will be empty.
    if os.stat(_test_path.name).st_size != 0:
        file_to_parse = _test_path.name

    # TODO: b/234531695 support reading header files as well.
    with open(file_to_parse) as class_file:
        logging.debug('Parsing: %s', test_path)
        content = class_file.read()
        # ('TYPED_TEST', 'PrimeTableTest', 'ReturnsTrueForPrimes')
        method_matches = re.findall(_CC_CLASS_METHOD_RE, content)
        # ('OnTheFlyAndPreCalculated', 'PrimeTableTest2')
        prefix_matches = re.findall(_CC_PARAM_CLASS_RE, content)
        # 'PrimeTableTest'
        typed_matches = re.findall(_TYPE_CC_CLASS_RE, content)

    classes = {cls[1] for cls in method_matches}
    class_info = {}
    test_not_found = False
    for cls in classes:
        class_info.setdefault(cls, {'methods': set(),
                                    'prefixes': set(),
                                    'typed': False})
    logging.debug('Probing TestCase.TestName pattern:')
    for match in method_matches:
        if class_info.get(match[1]):
            logging.debug('  Found %s.%s', match[1], match[2])
            class_info[match[1]]['methods'].add(match[2])
        else:
            test_not_found = True
    # Parameterized test.
    logging.debug('Probing InstantiationName/TestCase pattern:')
    for match in prefix_matches:
        if class_info.get(match[1]):
            logging.debug('  Found %s/%s', match[0], match[1])
            class_info[match[1]]['prefixes'].add(match[0])
        else:
            test_not_found = True
    # Typed test
    logging.debug('Probing typed test names:')
    for match in typed_matches:
        if class_info.get(match):
            logging.debug('  Found %s', match)
            class_info[match]['typed'] = True
        else:
            test_not_found = True
    if test_not_found:
        metrics.LocalDetectEvent(
            detect_type=DetectType.NATIVE_TEST_NOT_FOUND,
            result=DetectType.NATIVE_TEST_NOT_FOUND)
    return class_info

def get_cc_class_type(class_info, classname):
    """Tell the type of the given class.

    Args:
        class_info: A dict of class info.
        classname: A string of class name.

    Returns:
        String of the gtest type to prompt. The output will be one of:
        1. 'regular test'             (GTEST_REGULAR)
        2. 'typed test'               (GTEST_TYPED)
        3. 'value-parameterized test' (GTEST_PARAM)
        4. 'typed-parameterized test' (GTEST_TYPED_PARAM)
    """
    if class_info.get(classname).get('prefixes'):
        if class_info.get(classname).get('typed'):
            return constants.GTEST_TYPED_PARAM
        return constants.GTEST_PARAM
    if class_info.get(classname).get('typed'):
        return constants.GTEST_TYPED
    return constants.GTEST_REGULAR

def find_host_unit_tests(module_info, path):
    """Find host unit tests for the input path.

    Args:
        module_info: ModuleInfo obj.
        path: A string of the relative path from $ANDROID_BUILD_TOP that we want
              to search.

    Returns:
        A list that includes the module name of host unit tests, otherwise an empty
        list.
    """
    logging.debug('finding host unit tests under %s', path)
    host_unit_test_names = module_info.get_all_host_unit_tests()
    logging.debug('All the host unit tests: %s', host_unit_test_names)

    # Return all tests if the path relative to ${ANDROID_BUILD_TOP} is '.'.
    if path == '.':
        return host_unit_test_names

    tests = []
    for name in host_unit_test_names:
        for test_path in module_info.get_paths(name):
            if test_path.find(path) == 0:
                tests.append(name)
    return tests

def get_annotated_methods(annotation, file_path):
    """Find all the methods annotated by the input annotation in the file_path.

    Args:
        annotation: A string of the annotation class.
        file_path: A string of the file path.

    Returns:
        A set of all the methods annotated.
    """
    methods = set()
    annotation_name = '@' + str(annotation).split('.')[-1]
    with open(file_path) as class_file:
        enter_annotation_block = False
        for line in class_file:
            if str(line).strip().startswith(annotation_name):
                enter_annotation_block = True
                continue
            if enter_annotation_block:
                matches = re.findall(_JAVA_METHODS_RE, line)
                if matches:
                    methods.update({match[1] for match in matches})
                    enter_annotation_block = False
                    continue
    return methods

def get_test_config_and_srcs(test_info, module_info):
    """Get the test config path for the input test_info.

    The search rule will be:
    Check if test name in test_info could be found in module_info
      1. AndroidTest.xml under module path if no test config be set.
      2. The first test config defined in Android.bp if test config be set.
    If test name could not found matched module in module_info, search all the
    test config name if match.

    Args:
        test_info: TestInfo obj.
        module_info: ModuleInfo obj.

    Returns:
        A string of the config path and list of srcs, None if test config not
        exist.
    """
    android_root_dir = os.environ.get(constants.ANDROID_BUILD_TOP)
    test_name = test_info.test_name
    mod_info = module_info.get_module_info(test_name)
    if mod_info:
        test_configs = mod_info.get(constants.MODULE_TEST_CONFIG, [])
        if len(test_configs) == 0:
            # Check for AndroidTest.xml at the module path.
            for path in mod_info.get(constants.MODULE_PATH, []):
                config_path = os.path.join(
                    android_root_dir, path, constants.MODULE_CONFIG)
                if os.path.isfile(config_path):
                    return config_path, mod_info.get(constants.MODULE_SRCS, [])
        if len(test_configs) >= 1:
            test_config = test_configs[0]
            config_path = os.path.join(android_root_dir, test_config)
            if os.path.isfile(config_path):
                return config_path, mod_info.get(constants.MODULE_SRCS, [])
    else:
        for _, info in module_info.name_to_module_info.items():
            test_configs = info.get(constants.MODULE_TEST_CONFIG, [])
            for test_config in test_configs:
                config_path = os.path.join(android_root_dir, test_config)
                config_name = os.path.splitext(os.path.basename(config_path))[0]
                if config_name == test_name and os.path.isfile(config_path):
                    return config_path, info.get(constants.MODULE_SRCS, [])
    return None, None


def need_aggregate_metrics_result(test_xml):
    """Check if input test config need aggregate metrics.

    If the input test define metrics_collector, which means there's a need for
    atest to have the aggregate metrcis result.

    Args:
        test_xml: A string of the path for the test xml.

    Returns:
        True if input test need to enable aggregate metrics result.
    """
    if os.path.isfile(test_xml):
        xml_root = ET.parse(test_xml).getroot()
        if xml_root.findall('.//metrics_collector'):
            return True
        # Check if include other config
        include_configs = xml_root.findall('.//include')
        for include_config in include_configs:
            name = include_config.attrib[_XML_NAME].strip()
            # Get the absolute path for the include config.
            include_path = os.path.join(
                str(test_xml).split(str(name).split('/')[0])[0], name)
            if need_aggregate_metrics_result(include_path):
                return True
    return False
