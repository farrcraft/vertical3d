#!/bin/sh

# NOTE: in order to create code coverage reports, the project must be built with gcov enabled:
#
# ./configure --enable-gcov
#

make gcov-clean
make gcov-reset
make test
make cov

exit 0
 

