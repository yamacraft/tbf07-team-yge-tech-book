#!/bin/bash -eux

gs \
  -sOutputFile=articles/tbf07-yge-firebase-authentication-guide-gray.pdf \
  -sDEVICE=pdfwrite \
  -dPDFX \
  -dCompatibilityLevel=1.3 \
  -sColorConversionStrategy=Gray \
  -dProcessColorModel=/DeviceGray \
  -dNOPAUSE \
  -dBATCH \
  articles/tbf07-yge-firebase-authentication-guide.pdf
