#!/bin/bash
qcc -Wall -O2 -disable-dimensions BpostBview.c -o BpostBview -L$BASILISK/gl -lglutils -lfb_tiny -lm
./BpostBview tb57.0 ts1.00 te199

