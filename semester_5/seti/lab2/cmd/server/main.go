package main

import (
	"context"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"syscall"
	"time"
)

const (
	defaultServerPort   = 6767
	uploadsDir          = "uploads"
	speedInterval       = 3 * time.Second
	readTimeout         = 30 * time.Second
	writeTimeout        = 5 * time.Second
	bytesPerMiB         = 1 << 20
	maxFilenameAttempts = 10000
	defaultMaxClients   = 100
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run() error {
	port := flag.Int(
		"port",
		defaultServerPort,
		"server TCP port",
	)

	maxClients := flag.Int(
		"max-clients",
		defaultMaxClients,
		"maximum number of concurrent clients",
	)

	flag.Parse()

	if flag.NArg() != 0 {
		return fmt.Errorf("usage: %s [-port PORT]", os.Args[0])
	}

	if *port < 1 || *port > 65535 {
		return fmt.Errorf("invalid port: %d", *port)
	}

	if *maxClients < 1 {
		return fmt.Errorf("invalid max clients: %d", *maxClients)
	}

	uploads, err := openUploads(uploadsDir)
	if err != nil {
		return err
	}
	defer func() {
		if err := uploads.Close(); err != nil {
			fmt.Fprintln(os.Stderr, "close uploads directory:", err)
		}
	}()

	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	defer stop()

	address := net.JoinHostPort("", strconv.Itoa(*port))
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return fmt.Errorf("listen: %w", err)
	}
	defer func() {
		if err := listener.Close(); err != nil && !errors.Is(err, net.ErrClosed) {
			fmt.Fprintln(os.Stderr, "close listener:", err)
		}
	}()

	return serve(ctx, listener, uploads, *maxClients)
}
