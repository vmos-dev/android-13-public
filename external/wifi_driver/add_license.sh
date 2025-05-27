#!/bin/bash

read_dir(){
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            read_dir $1"/"$file
        else
			file_name=$1"/"$file
			if [ "${file##*.}"x = "c"x ] || [ "${file##*.}"x = "h"x ] || [ "$file" = "Makefile" ] || [ "$file" = "Kconfig" ]; then
				cat $file_name | grep "$old_license" > /dev/null 2>&1
				if [ $? -ne 0 ]; then
					cat $file_name | grep "$old_license1" > /dev/null 2>&1
					if [ $? -ne 0 ]; then
						echo "modify $file_name"
						if [ "$file" = "Makefile" ] || [ "$file" = "Kconfig" ]; then
							sed -i "1i $license1" $file_name
						else
							sed -i "1i $license" $file_name
						fi
					fi
				fi
			fi
        fi
    done
}

old_license="* Copyright"
old_license1="SPDX-License-Identifier"
license="/* SPDX-License-Identifier: GPL-2.0 */"
license1="# SPDX-License-Identifier: GPL-2.0"
read_dir $1
