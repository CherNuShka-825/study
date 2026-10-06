package main

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"flag"
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
	defaultPort       = 6767
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
	interfaceName := flag.String(
		"iface",
		"",
		"network interface for multicast",
	)

	port := flag.Int(
		"port",
		defaultPort,
		"UDP multicast port",
	)

	flag.Parse()

	if flag.NArg() != 1 {
		return fmt.Errorf(
			"usage: %s [-iface interface] [-port port] <multicast-address>",
			os.Args[0],
		)
	}

	if *port < 1 || *port > 65535 {
		return fmt.Errorf("invalid port: %d", *port)
	}

	groupIP, err := netip.ParseAddr(flag.Arg(0))
	if err != nil {
		return fmt.Errorf("invalid IP: %w", err)
	}

	if !groupIP.IsMulticast() {
		return fmt.Errorf("ip is not multicast")
	}

	var iface *net.Interface

	if *interfaceName != "" {
		iface, err = net.InterfaceByName(*interfaceName)
		if err != nil {
			return fmt.Errorf(
				"invalid network interface %q: %w",
				*interfaceName,
				err,
			)
		}
	} else {
		iface, err = defaultMulticastInterface()
		if err != nil {
			return err
		}
	}

	var network string

	if groupIP.Is4() {
		network = "udp4"
	} else {
		network = "udp6"
	}

	groupAddr := net.UDPAddrFromAddrPort(
		netip.AddrPortFrom(groupIP, uint16(*port)),
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
	heartbeatDone := make(chan struct{})

	go func() {
		defer wg.Done()

		if err := receiveLoop(ctx, conn, peers, instanceID); err != nil {
			select {
			case errCh <- err:
			default:
			}
		}
	}()

	go func() {
		defer wg.Done()
		defer close(heartbeatDone)

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

	<-heartbeatDone
	if runErr == nil {
		select {
		case runErr = <-errCh:
		default:
		}
	}

	closeErr := conn.Close()

	wg.Wait()

	if runErr != nil {
		return runErr
	}

	if closeErr != nil {
		return fmt.Errorf(
			"failed to close multicast UDP listener: %w",
			closeErr,
		)
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
			return fmt.Errorf(
				"failed to set multicast TTL: %w",
				err,
			)
		}

		packetConn = ipv4Conn
	} else {
		ipv6Conn := ipv6.NewPacketConn(conn)

		if err := ipv6Conn.SetMulticastHopLimit(1); err != nil {
			return fmt.Errorf(
				"failed to set multicast hop limit: %w",
				err,
			)
		}

		packetConn = ipv6Conn
	}

	if err := packetConn.SetMulticastInterface(iface); err != nil {
		return fmt.Errorf(
			"failed to set multicast interface: %w",
			err,
		)
	}

	if err := packetConn.SetMulticastLoopback(true); err != nil {
		return fmt.Errorf(
			"failed to enable multicast loopback: %w",
			err,
		)
	}

	return nil
}

func defaultMulticastInterface() (*net.Interface, error) {
	ifaces, err := net.Interfaces()
	if err != nil {
		return nil, fmt.Errorf(
			"failed to get network interfaces: %w",
			err,
		)
	}

	var suitable []*net.Interface

	for i := range ifaces {
		iface := &ifaces[i]

		if iface.Flags&net.FlagUp == 0 { // чекает включен ли
			continue
		}

		if iface.Flags&net.FlagMulticast == 0 { // чек поддержки мультикаста
			continue
		}

		if iface.Flags&net.FlagLoopback != 0 { // чек, что интерфейс не лупбэк
			continue
		}

		suitable = append(suitable, iface)
	}

	if len(suitable) == 0 {
		return nil, fmt.Errorf(
			"no multicast-capable network interface found",
		)
	}

	if len(suitable) > 1 {
		names := make([]string, 0, len(suitable))

		for _, iface := range suitable {
			names = append(names, iface.Name)
		}

		return nil, fmt.Errorf(
			"multiple multicast interfaces found: %v; specify one with -iface",
			names,
		)
	}

	return suitable[0], nil
}

func generateInstanceID() (string, error) {
	data := make([]byte, 16)

	_, err := rand.Read(data)
	if err != nil {
		return "", err
	}

	return hex.EncodeToString(data), nil
}
