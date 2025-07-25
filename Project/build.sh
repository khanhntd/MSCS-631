#!/bin/bash

CXX=g++
NS3_DIR="./ns-allinone-3.41/ns-3.41"  # or your NS-3 path
SRC="./iot_smartcity.cc"
OUT="iot_smartcity"

$CXX -std=c++17 -Wall -g \
-I $NS3_DIR \
-I $NS3_DIR/build \
-I $NS3_DIR/src/core \
-I $NS3_DIR/src/network \
-I $NS3_DIR/src/internet \
-I $NS3_DIR/src/point-to-point \
-I $NS3_DIR/src/applications \
$SRC \
-o $OUT \
$NS3_DIR/build/lib/libns3-core-default.a \
$NS3_DIR/build/lib/libns3-network-default.a \
$NS3_DIR/build/lib/libns3-internet-default.a \
$NS3_DIR/build/lib/libns3-point-to-point-default.a \
$NS3_DIR/build/lib/libns3-applications-default.a