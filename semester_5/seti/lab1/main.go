package main

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"net"
	"net/netip"
	"os"
	"time"
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
	defer func() {
		if err := conn.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "failed to close UDP connection: %v\n", err)
		}
	}()

	peers := NewPeerStore()

	go receiveLoop(conn, peers, instanceID)
	go heartbeatLoop(conn, groupAddr, instanceID)
	go cleanupLoop(peers)

	select {}
}

func generateInstanceID() (string, error) {
	bytes := make([]byte, 16)

	if _, err := rand.Read(bytes); err != nil {
		return "", err
	}

	return hex.EncodeToString(bytes), nil
}
