package main

import (
	"context"
	"errors"
	"fmt"
	"net"
	"os"
	"sync"
	"time"

	"github.com/CherNuShka-825/study/semester_5/seti/internal/protocol"
)

func serve(ctx context.Context, listener net.Listener, uploads *os.Root, maxClients int) error {
	var wg sync.WaitGroup
	var mu sync.Mutex
	active := make(map[net.Conn]struct{})

	go func() {
		<-ctx.Done()
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
