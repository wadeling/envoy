#!/bin/bash

#fortio load -c 1 -n 1 -H "x-id:1" http://127.0.0.1:9211
fortio load -c 1 -n 1 -H "x-id:1" http://127.0.0.1:9300
