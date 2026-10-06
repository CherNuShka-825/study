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
	maxPeers          = 1024
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run() error {
	if len(os.Args) != 3 {
		return fmt.Errorf("usage: %s <multicast-address> <interface>", os.Args[0])
	}

	groupIP, err := netip.ParseAddr(os.Args[1])
	if err != nil {
		return fmt.Errorf("invalid IP: %w", err)
	}

	iface, err := net.InterfaceByName(os.Args[2])
	if err != nil {
		return fmt.Errorf("invalid network interface %q: %w", os.Args[2], err)
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

	conn, err := net.ListenMulticastUDP(network, iface, groupAddr)
	if err != nil {
		return fmt.Errorf("failed to listen multicast UDP: %w", err)
	}

	if err := configureMulticastDelivery(conn, iface, groupIP); err != nil {
		conn.Close()
		return err
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

	errCh := make(chan error, 1)

	go func() {
		defer wg.Done()
		receiveLoop(ctx, conn, peers, instanceID)
	}()

	go func() {
		defer wg.Done()
		if err := heartbeatLoop(ctx, conn, groupAddr, instanceID); err != nil {
			select {
			case errCh <- err:
			default:
			}
		}
	}()

	go func() {
		defer wg.Done()
		cleanupLoop(ctx, peers)
	}()

	var runErr error

	select {
	case <-ctx.Done():
	case runErr = <-errCh:
		stop()
	}

	closeErr := conn.Close()

	wg.Wait()

	if runErr != nil {
		return runErr
	}

	if closeErr != nil {
		return fmt.Errorf("failed to close multicast UDP listener: %w", closeErr)
	}

	return nil
}

type multicastPacketConn interface {
	SetMulticastInterface(*net.Interface) error
	SetMulticastLoopback(bool) error
}

func configureMulticastDelivery(
	conn *net.UDPConn,
	iface *net.Interface,
	groupIP netip.Addr,
) error {
	var packetConn multicastPacketConn

	if groupIP.Is4() {
		ipv4Conn := ipv4.NewPacketConn(conn)
		if err := ipv4Conn.SetMulticastTTL(1); err != nil {
			return fmt.Errorf("failed to set multicast TTL: %w", err)
		}
		packetConn = ipv4Conn
	} else {
		ipv6Conn := ipv6.NewPacketConn(conn)
		if err := ipv6Conn.SetMulticastHopLimit(1); err != nil {
			return fmt.Errorf("failed to set multicast hop limit: %w", err)
		}
		packetConn = ipv6Conn
	}

	if err := packetConn.SetMulticastInterface(iface); err != nil {
		return fmt.Errorf("failed to set multicast interface: %w", err)
	}

	if err := packetConn.SetMulticastLoopback(true); err != nil {
		return fmt.Errorf("failed to enable multicast loopback: %w", err)
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
