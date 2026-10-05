package main

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"net"
	"net/netip"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"golang.org/x/net/ipv4"
	"golang.org/x/net/ipv6"
)

const (
	port              = 6767
	heartbeatInterval = 1 * time.Second
	peerTimeout       = 3 * time.Second
	cleanupInterval   = 500 * time.Millisecond
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run() error {
	if len(os.Args) != 2 {
		return fmt.Errorf("expected ipv4 or ipv6")
	}

	groupIP, err := netip.ParseAddr(os.Args[1])
	if err != nil {
		return fmt.Errorf("invalid IP: %w", err)
	}

	if !groupIP.IsMulticast() {
		return fmt.Errorf("ip is not multicast")
	}

	var network string
	if groupIP.Is4() {
		network = "udp4"
	} else {
		network = "udp6"
	}

	groupAddr := net.UDPAddrFromAddrPort(
		netip.AddrPortFrom(groupIP, port),
	)

	instanceID, err := generateInstanceID()
	if err != nil {
		return fmt.Errorf("failed to generate instance ID: %w", err)
	}

	conn, err := net.ListenMulticastUDP(network, nil, groupAddr)
	if err != nil {
		return fmt.Errorf("failed to listen multicast UDP: %w", err)
	}

	if groupIP.Is4() {
		packetConn := ipv4.NewPacketConn(conn)

		if err := packetConn.SetMulticastLoopback(true); err != nil {
			conn.Close()
			return fmt.Errorf("failed to enable multicast loopback: %w", err)
		}
	} else {
		packetConn := ipv6.NewPacketConn(conn)

		if err := packetConn.SetMulticastLoopback(true); err != nil {
			conn.Close()
			return fmt.Errorf("failed to enable multicast loopback: %w", err)
		}
	}

	ctx, stop := signal.NotifyContext(
		context.Background(),
		os.Interrupt,
		syscall.SIGTERM,
	)
	defer stop()

	peers := NewPeerStore()

	var wg sync.WaitGroup
	wg.Add(3)

	go func() {
		defer wg.Done()
		receiveLoop(ctx, conn, peers, instanceID)
	}()

	go func() {
		defer wg.Done()
		heartbeatLoop(ctx, conn, groupAddr, instanceID)
	}()

	go func() {
		defer wg.Done()
		cleanupLoop(ctx, peers)
	}()

	<-ctx.Done()

	closeErr := conn.Close()

	wg.Wait()

	if closeErr != nil {
		return fmt.Errorf("failed to close multicast UDP listener: %w", closeErr)
	}

	return nil
}

func generateInstanceID() (string, error) {
	data := make([]byte, 16)

	_, err := rand.Read(data)
	if err != nil {
		return "", err
	}

	return hex.EncodeToString(data), nil
}
