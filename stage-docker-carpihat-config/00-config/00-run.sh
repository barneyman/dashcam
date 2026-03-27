#!/bin/bash -e

# first thing, I copy a 'new' config over, but I've  based it on the official one,
# so it's important we detect if that's changed!!

CONFIG_TXT_EXPECTED_SHA="ea82a637facdcde1dc82ad343de14da4684673cb07deca9f5e0c15abfb6b2eb7  ${ROOTFS_DIR}/boot/firmware/config.txt"

echo $CONFIG_TXT_EXPECTED_SHA | sha256sum --check 

if [ "$(echo $CONFIG_TXT_EXPECTED_SHA | sha256sum --check | awk '{print $2}')" != "OK" ]
    echo "Checksums of config.txt does not match expected"
    exit 1
fi

# ok - we share the same seed, go ahead

install -m 644 files/config.txt "${ROOTFS_DIR}/boot/firmware/"
