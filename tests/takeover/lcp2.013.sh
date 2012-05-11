#!/bin/sh

. "${TEST_SCRIPTS_DIR}/unit.sh"

define_test "Node with NODE_FLAGS_NOIPTAKEOVER doesn't lose IPs"

export CTDB_TEST_LOGLEVEL=0

required_result <<EOF
192.168.21.254 2
192.168.21.253 1
192.168.21.252 0
192.168.20.254 2
192.168.20.253 1
192.168.20.252 0
192.168.20.251 2
192.168.20.250 1
192.168.20.249 0
EOF

simple_test 0x01000000,0,0 <<EOF
192.168.20.249 0
192.168.20.250 1
192.168.20.251 2
192.168.20.252 0
192.168.20.253 1
192.168.20.254 2
192.168.21.252 0
192.168.21.253 1
192.168.21.254 2
EOF
