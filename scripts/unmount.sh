#!/bin/bash

# Don't bother quitting if these fail, so we can use this to take down something that has only been
# half set up. These commands won't mess up anything important if they fail and execution continues.

echo 'Unmounting filesystem...'
sudo umount /mnt/secrets

echo 'Closing LUKS device...'
sudo cryptsetup close loop-steg

echo 'Detaching loop device...'
LOOP_DEV=$(losetup -j /mnt/loop-steg/data | head -n 1 | cut -d ':' -f 1)
sudo losetup -d "$LOOP_DEV"

echo 'Waiting for loop-steg to finish...'
sudo umount /mnt/loop-steg

while [ ! -z "$(pgrep loop-steg)" ]
do
    sleep 1
done

echo 'Cleaning up...'
sudo rmdir /mnt/loop-steg
sudo rmdir /mnt/secrets
