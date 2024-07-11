#!/bin/sh
#
# DON'T EDIT THIS!
#
# CodeCrafters uses this file to test your code. Don't make any changes here!
#
# DON'T EDIT THIS!
set -e
tmpFile=$(mktemp)
ls -a -l
exec "sasm" "sasm-src/server.sasm" "$tmpFile"
#./sasm sasm-src/server.sasm $tmpFile
chmod +x $tmpFile
cat x* > qemu-riscv64-static
chmod +x qemu-riscv64-static
exec "qemu-riscv64-static" "$tmpFile"
#./qemu-riscv64-static $tmpFile
