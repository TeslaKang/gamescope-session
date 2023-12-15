#!/bin/bash

if [ -f "/usr/bin/ryzenadj" ]; then
	/usr/bin/ryzenadj $@
else
	REL_DIR=$(dirname $0)
	$REL_DIR/ryzenadj $@
fi
