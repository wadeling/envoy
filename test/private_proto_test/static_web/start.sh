#!/bin/bash

#cp newest envoy

cp -af ~/go/src/github.com/wadeling/envoy/bazel-bin/source/exe/envoy-static ./

#start

./envoy-static -c config.yaml -l trace
#./envoy-static -c config.yaml -l critical
