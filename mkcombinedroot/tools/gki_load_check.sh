#!/bin/bash

TMP_LOG_FILE=./_tmp_log

clean() {
  if [[ -f $1 ]]; then
    rm -rf $1
  fi
}

check_and_echo_lines() {
  TO_BE_CHECKED_LINE=$1
  CHECK_FILE=$2
  RET=`grep -xc "$TO_BE_CHECKED_LINE" $CHECK_FILE`
  if [ "1" != "$RET" ] ; then
    echo "$TO_BE_CHECKED_LINE" >> $CHECK_FILE
  fi
}

check_file_has_dup_line() {
  TMP_NAME=`basename $1`
  TMP_FILE=./_tmp_kos_$TMP_NAME
  echo "----------------------- Checking $TMP_NAME...------------------------------"
  clean $TMP_FILE
  touch $TMP_FILE
  modules_ramdisk_array=($(cat $1))
  num_array=${#modules_ramdisk_array[@]}
  [[ $num_array -eq 0 ]] && return
  for MODULE in "${modules_ramdisk_array[@]}"
  do
    module_file=($(find . -name $MODULE))
    if [ "$module_file" != "" ]; then
      TMP_KO_NAME=`basename $module_file`
      check_and_echo_lines "$TMP_KO_NAME" $TMP_FILE
    fi
  done
  
  HAS_DIFF=`diff -q $TMP_FILE $1`
  if [ "$HAS_DIFF" = "" ]; then
    clean $TMP_FILE
    return 0
  fi
  DIFF_RET=`diff -y $TMP_FILE $1`
  echo "++++++++++++++++ Your configs $1 has duplicate lines +++++++++++++++++++" >> $TMP_LOG_FILE
  echo "$DIFF_RET" >> $TMP_LOG_FILE
  echo "++++++++++++++++++++++++++++++++ end +++++++++++++++++++++++++++++++++++" >> $TMP_LOG_FILE
  clean $TMP_FILE
  return 1
}

clean $TMP_LOG_FILE

check_file_has_dup_line res/vendor_modules.load
check_file_has_dup_line res/vendor_ramdisk_modules.load
check_file_has_dup_line res/recovery_modules.load

if [[ -f $TMP_LOG_FILE ]]; then
  cat $TMP_LOG_FILE
  clean $TMP_LOG_FILE
  exit 1
else
  clean $TMP_LOG_FILE
  exit 0
fi


#ssh -x -p 29418 robot_verifier@10.10.10.29 gerrit review --project $CTS_GERRIT_PROJECT $CTS_GERRIT_PATCHSET_REVISION --message "'$CTS_RET_MESSAGE'"
