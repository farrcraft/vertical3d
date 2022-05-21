#!/bin/sh

cd tests

./unit_tests --build_info --log_level=all --log_format=HRF --report_level=detailed --report_format=HRF --show_progress=yes
