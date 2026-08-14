#!/bin/bash

rm -f ../VirtUSB.zip
git ls-files -co --exclude-standard | zip -@ ../VirtUSB.zip
