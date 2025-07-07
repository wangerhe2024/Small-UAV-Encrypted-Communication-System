package main

import (
	"fmt"
	"log"
	"os/exec"
)

func cmd_server_side(dev string, mtu int, NetAddr string) {
	int_mtu := mtu - 20
	str_mtu := fmt.Sprintf("%v", int_mtu)
	server_cmd := "ifconfig " + dev + " mtu " + str_mtu
	log.Print(server_cmd)
	server_cmd1 := "ifconfig " + dev + " " + NetAddr //"10.64.1.1/30"
	log.Print(server_cmd1)
	run_cmd(server_cmd)
	run_cmd(server_cmd1)
}

func cmd_client_side(dev string, mtu int, NetAddr string) {
	int_mtu := mtu - 20
	str_mtu := fmt.Sprintf("%v", int_mtu)
	client_cmd := "ifconfig " + dev + " mtu " + str_mtu
	log.Print(client_cmd)
	client_cmd1 := "ifconfig " + dev + " " + NetAddr //" 10.64.1.2/30"
	log.Print(client_cmd1)
	run_cmd(client_cmd)
	run_cmd(client_cmd1)
}

func run_cmd(cmd string) {
	c := exec.Command("bash", "-c", cmd)
	output, err := c.CombinedOutput()
	if err != nil {
		log.Print(err)
	}
	if len(string(output)) > 0 {
		log.Print(string(output))
	}
}
