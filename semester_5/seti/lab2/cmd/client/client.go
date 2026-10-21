package main

import (
	"flag"
	"fmt"
	"os"
)

const (
	defaultServerHost = "127.0.0.1"
	defaultServerPort = 6767
)

func run() error {
	host := flag.String(
		"host",
		defaultServerHost,
		"server hostname or IP address",
	)

	port := flag.Int(
		"port",
		defaultServerPort,
		"server TCP port",
	)

	flag.Parse()

	if flag.NArg() != 1 {
		return fmt.Errorf(
			"usage: %s [-host host] [-port port] <file>",
			os.Args[0],
		)
	}

	if *host == "" {
		return fmt.Errorf("host is empty")
	}

	if *port < 1 || *port > 65535 {
		return fmt.Errorf("invalid port: %d", *port)
	}

	return sendFile(flag.Arg(0), *host, *port)
}
