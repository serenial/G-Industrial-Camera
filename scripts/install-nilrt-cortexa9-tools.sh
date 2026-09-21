#! /bin/sh

ZIPFILE_NAME="NILinux2026Q3DeviceDrivers"
ZIPFILE_SHA512="6e7e2590c79dd06bddabe287794e03032622493f89b55c2711f1f4d89ae529c2576b1216139f34235021d967123a58dab138865614678bca73f2f79575593bcd"

curl https://download.ni.com/support/softlib/MasterRepository/LinuxDrivers2026Q3/NILinux2026Q3DeviceDrivers.zip -Lo $ZIPFILE_NAME.zip
echo "${ZIPFILE_SHA512} ${ZIPFILE_NAME}.zip" | sha512sum --check --status

mkdir $ZIPFILE_NAME
unzip $ZIPFILE_NAME.zip -d $ZIPFILE_NAME

sudo apt install ./$ZIPFILE_NAME/ni-ubuntu2204-* && sudo apt update
sudo apt install ni-linuxrt-toolchain-*arm
