#!/bin/bash
set -e

# --- Configuration ---

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_SUBDIR="kernel"
TOOLS_SUBDIR="tools"
KERNEL_IMAGE="../../kernel-dev/kernel/linux-5.4.289"
VM_USER="root"
VM_HOST="198.19.249.3"
VM_PORT="10021"
VM_DEST_DIR="/root"

# --- Build Kernel Module ---

echo "*** Building kernel module... ***"
cd "$SCRIPT_DIR/$KERNEL_SUBDIR"
make KDIR="$KERNEL_IMAGE"
BUILD_STATUS_KO=$?
cd "$SCRIPT_DIR"

# --- Build Userspace Tool ---

echo "*** Building userspace tool... ***"
cd "$SCRIPT_DIR/$TOOLS_SUBDIR"
make
BUILD_STATUS_USR=$?
cd "$SCRIPT_DIR"

# --- Check Build Status ---

if [[ $BUILD_STATUS_KO -ne 0 ]]; then
    echo "ERROR: Kernel module build failed."
    exit 1
fi
if [[ $BUILD_STATUS_USR -ne 0 ]]; then
    echo "ERROR: Userspace tool build failed."
    exit 1
fi

echo "*** Both builds succeeded. ***"

# --- Copy Files to VM ---

echo "*** Copying files to VM... ***"
scp -P "$VM_PORT" "$SCRIPT_DIR/$KERNEL_SUBDIR/easymoney.ko" "$VM_USER@$VM_HOST:$VM_DEST_DIR/"
scp -P "$VM_PORT" "$SCRIPT_DIR/$TOOLS_SUBDIR/ctrl" "$VM_USER@$VM_HOST:$VM_DEST_DIR/"

echo "*** Files copied to $VM_HOST:$VM_DEST_DIR via port $VM_PORT ***"

# ...[Previous build and scp steps]...

# --- Run on VM: Insert Module, Run User Tool ---

SSH_CMD="
set -e
cd $VM_DEST_DIR
if lsmod | grep -q '^easymoney'; then
  echo 'Module is already loaded—removing it first...'
  rmmod easymoney
else
  echo 'Module not loaded—proceeding to insert...'
fi
echo 'Inserting kernel module...'
insmod ./easymoney.ko
echo 'Running user tool...'
chmod +x ./ctrl
./ctrl
"

echo "*** Running module and tool on VM... ***"
ssh -p "$VM_PORT" "$VM_USER@$VM_HOST" "$SSH_CMD"

