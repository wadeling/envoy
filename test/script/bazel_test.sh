#!/bin/bash

 bazel test -c opt --test_output=all  --test_filter=HeaderMapImplTest.TestCopyTime //test/common/http:header_map_impl_test
