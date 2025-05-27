#!/system/bin/sh

chmod 755 /dev/cpuset -R
chmod 666 /dev/dri/*
chmod 666 /dev/snd/*

chmod 666 /dev/ashmem
chmod 666 /dev/ptmx
chmod 666 /dev/pts/ptmx
chmod 666 /dev/mali0

rm /dev/ion
chmod 444 /dev/dma_heap/*

chmod 666 /dev/mpp_service
chmod 666 /dev/rga

chmod 666 /dev/sw_sync

chmod 666 /dev/input/event*
