package main

import (
	"fmt"
	"net"
	"os"
	"strings"
	"time"
)

const (
	bufSize    = 6767
	cutMessege = "AMOGUS"
)

func receiveLoop(
	conn *net.UDPConn,
	peers *PeerStore,
	instanceID string,
) {
	buf := make([]byte, bufSize)

	for {
		n, sender, err := conn.ReadFromUDP(buf)
		if err != nil {
			fmt.Fprintf(os.Stderr, "failed to read UDP packet: %v\n", err)
			return
		}

		receivedID, ok := parseAliveMessage(buf[:n])
		if !ok {
			continue
		}

		if receivedID == instanceID {
			continue
		}

		aliveIPs, changed := peers.Seen(
			receivedID,
			sender.IP.String(),
			time.Now(),
		)

		if changed {
			printAlive(aliveIPs)
		}
	}
}

func heartbeatLoop(
	conn *net.UDPConn,
	groupAddr *net.UDPAddr,
	instanceID string,
) {
	ticker := time.NewTicker(heartbeatInterval)
	defer ticker.Stop()

	message := []byte(cutMessege + " " + instanceID)

	for {
		_, err := conn.WriteToUDP(message, groupAddr)
		if err != nil {
			fmt.Fprintf(os.Stderr, "failed to send heartbeat: %v\n", err)
			return
		}

		<-ticker.C
	}
}

func parseAliveMessage(data []byte) (string, bool) {
	message := string(data)

	messageType, instanceID, found := strings.Cut(message, " ")
	if !found {
		return "", false
	}

	if messageType != cutMessege {
		return "", false
	}

	if instanceID == "" {
		return "", false
	}

	return instanceID, true
}
