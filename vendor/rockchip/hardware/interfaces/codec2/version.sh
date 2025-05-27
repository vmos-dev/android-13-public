#!/bin/bash
CURRENTDIR=`dirname $0`
cd $CURRENTDIR

VERSION_TARGET="$(cat version.h.template)"

echo "${VERSION_TARGET}"
