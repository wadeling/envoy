#!/bin/bash

# copy bin to cur dir
rootdir='../../../../../..'
cp $rootdir/build_release_stripped/envoy ./

# run
./envoy -c echo2_server.yaml -l trace
