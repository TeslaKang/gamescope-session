#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    exec pkexec --disable-internal-agent "$0" "$@"
fi

REL_DIR=$(dirname $0)
$REL_DIR/statusInit.sh on
