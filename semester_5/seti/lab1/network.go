package main

import (
	"context"
	"encoding/hex"
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
	ctx context.Context,
	conn *net.UDPConn,
	peers *PeerStore,
	instanceID string,
) {
	buf := make([]byte, bufSize)

	for {
		n, sender, err := conn.ReadFromUDP(buf)
		if err != nil {
			if ctx.Err() != nil {
				return
			}
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
	ctx context.Context,
	conn *net.UDPConn,
	groupAddr *net.UDPAddr,
	instanceID string,
) error {
	ticker := time.NewTicker(heartbeatInterval)
	defer ticker.Stop()

	message := []byte(cutMessege + " " + instanceID)

	for {
		select {
		case <-ctx.Done():
			return nil

		case <-ticker.C:
			_, err := conn.WriteToUDP(message, groupAddr)
			if err != nil {
				if ctx.Err() != nil {
					return nil
				}

				return fmt.Errorf("failed to send heartbeat: %w", err)
			}
		}
	}
}

func parseAliveMessage(data []byte) (string, bool) {
	message := string(data)
	messageType, instanceID, found := strings.Cut(message, " ")

	if !found ||
		messageType != cutMessege || instanceID == "" || !validInstanceID(instanceID) {
		return "", false
	}

	return instanceID, true
}

func validInstanceID(id string) bool {
	if len(id) != 32 {
		return false
	}

	_, err := hex.DecodeString(id)
	return err == nil
}
