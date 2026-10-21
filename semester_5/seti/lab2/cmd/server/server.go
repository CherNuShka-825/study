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
	"sync"
	"syscall"
	"time"

	"github.com/CherNuShka-825/study/semester_5/seti/internal/protocol"
)

const (
	defaultServerPort   = 6767
	uploadsDir          = "uploads"
	speedInterval       = 3 * time.Second
	readTimeout         = 30 * time.Second
	writeTimeout        = 5 * time.Second
	bytesPerMB          = 1_000_000
	maxFilenameAttempts = 10000
	defaultMaxClients   = 100
)

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

func serve(ctx context.Context, listener net.Listener, uploads *os.Root, maxClients int) error {
	var wg sync.WaitGroup
	var mu sync.Mutex
	active := make(map[net.Conn]struct{})

	go func() {
		<-ctx.Done()
		fmt.Println("[server] shutdown requested (SIGINT/SIGTERM)")
		if err := listener.Close(); err != nil && !errors.Is(err, net.ErrClosed) {
			fmt.Fprintln(os.Stderr, "close listener:", err)
		}
	}()

	var acceptErr error
	for {
		conn, err := listener.Accept()
		if err != nil {
			if ctx.Err() == nil {
				acceptErr = fmt.Errorf("accept connection: %w", err)
			}
			break
		}
		fmt.Printf("[server] client connected: %s\n", conn.RemoteAddr())

		mu.Lock()

		if len(active) >= maxClients {
			mu.Unlock()

			if err := conn.Close(); err != nil && !errors.Is(err, net.ErrClosed) {
				fmt.Fprintln(os.Stderr, "reject client:", err)
			}

			continue
		}

		active[conn] = struct{}{}
		mu.Unlock()

		wg.Go(func() {
			defer func() {
				mu.Lock()
				delete(active, conn)
				mu.Unlock()
			}()
			handleClient(conn, uploads)
		})
	}

	mu.Lock()
	connections := make([]net.Conn, 0, len(active))
	for conn := range active {
		connections = append(connections, conn)
	}
	mu.Unlock()

	for _, conn := range connections {
		if err := conn.Close(); err != nil && !errors.Is(err, net.ErrClosed) {
			fmt.Fprintln(os.Stderr, "close client connection:", err)
		}
	}
	wg.Wait()

	return acceptErr
}

func handleClient(conn net.Conn, uploads *os.Root) {
	defer func() {
		if err := conn.Close(); err != nil && !errors.Is(err, net.ErrClosed) {
			fmt.Fprintln(os.Stderr, "close connection:", err)
		}
	}()

	status := protocol.StatusSuccess
	if err := receiveFile(conn, uploads); err != nil {
		fmt.Fprintln(os.Stderr, "receive file:", err)
		status = protocol.StatusFailure
	}

	if err := conn.SetWriteDeadline(time.Now().Add(writeTimeout)); err != nil {
		fmt.Fprintln(os.Stderr, "set write deadline:", err)
		return
	}
	if err := protocol.WriteStatus(conn, status); err != nil {
		fmt.Fprintln(os.Stderr, "send status:", err)
	}
}
