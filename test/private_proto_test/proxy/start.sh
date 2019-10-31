#!/bin/bash

#cp newest envoy

cp -f ~/go/src/github.com/wadeling/envoy/bazel-bin/source/exe/envoy-static ./

#start

./envoy-static -c config.yaml -l trace --base-id 1
