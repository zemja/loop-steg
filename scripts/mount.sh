#!/bin/bash

function quit_maybe {
    if [ $? -ne 0 ]
    then
        if [ ! -z "$0" ]
        then
            echo $1
        fi
        exit 1
    fi
}

if [ ! -z "$(pgrep loop-steg)" ]
then
    echo "loop-steg is aready running, please stop it first"
    exit 1
fi

if [ -z "$1" ]
then
    echo "Usage: $0 /path/to/images/ [<fuse mount options>]"
    exit 1
fi

if [ ! -z "$2" ]
then
    OPTS="-o $2"
fi

CUR_USER=$USER

sudo mkdir -p /mnt/loop-steg
quit_maybe
sudo chown "$USER":"$USER" /mnt/loop-steg
quit_maybe
sudo chmod 755 /mnt/loop-steg
quit_maybe

sudo mkdir -p /mnt/secrets
quit_maybe
sudo chown "$USER":"$USER" /mnt/secrets
quit_maybe
sudo chmod 755 /mnt/secrets
quit_maybe

echo -n 'Decryption password: '
read -s PASSWORD
echo

echo 'Starting loop-steg...'
loop-steg <(echo -n $PASSWORD) "$1" /mnt/loop-steg/ -o allow_other $OPTS
quit_maybe

echo 'Attaching loop device...'
LOOP_DEV=$(sudo losetup -f)
quit_maybe

sudo losetup "$LOOP_DEV" /mnt/loop-steg/data
quit_maybe

echo 'Opening LUKS device...'
echo -n $PASSWORD | sudo cryptsetup open "$LOOP_DEV" loop-steg --key-file -
quit_maybe 'Failed (wrong password?)'

echo 'Mounting filesystem...'
sudo mount /dev/mapper/loop-steg /mnt/secrets
quit_maybe

echo 'Mounted at /mnt/secrets'
