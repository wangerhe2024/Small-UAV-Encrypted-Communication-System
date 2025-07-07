#!/bin/bash
go build -mod vendor ccs.go config.go syscall_c.go utils.go tap_conf.go string.go
