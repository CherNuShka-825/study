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
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "expected ipv4 or ipv6")
		os.Exit(1)
	}

	groupIP, err := netip.ParseAddr(os.Args[1])
	if err != nil {
		fmt.Fprint(os.Stderr, "invalid IP")
		os.Exit(1)
	}

	if !groupIP.IsMulticast() {
		fmt.Fprint(os.Stderr, "ip is not multicast")
		os.Exit(1)
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
		fmt.Fprint(os.Stderr, "failed to generate instance ID")
		os.Exit(1)
	}

	conn, err := net.ListenMulticastUDP(network, nil, groupAddr)
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to listen multicast UDP: %v\n", err)
		os.Exit(1)
	}

	if groupIP.Is4() {
		packetConn := ipv4.NewPacketConn(conn)

		if err := packetConn.SetMulticastLoopback(true); err != nil {
			fmt.Fprintf(os.Stderr, "failed to enable multicast loopback: %v\n", err)
			os.Exit(1)
		}
	} else {
		packetConn := ipv6.NewPacketConn(conn)

		if err := packetConn.SetMulticastLoopback(true); err != nil {
			fmt.Fprintf(os.Stderr, "failed to enable multicast loopback: %v\n", err)
			os.Exit(1)
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

	err = conn.Close()
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed close multicast UDP listener: %v\n", err)
		os.Exit(1)
	}

	wg.Wait()
}

func generateInstanceID() (string, error) {
	data := make([]byte, 16)

	_, err := rand.Read(data)
	if err != nil {
		return "", err
	}

	return hex.EncodeToString(data), nil
}
