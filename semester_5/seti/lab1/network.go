package main

import (
	"context"
	"encoding/hex"
	"fmt"
	"net"
	"strings"
	"time"
)

const (
	bufSize      = 6767
	aliveMessage = "AMOGUS"
	byeMessage   = "BYE"
)

func receiveLoop(
	ctx context.Context,
	conn *net.UDPConn,
	peers *PeerStore,
	instanceID string,
) error {
	buf := make([]byte, bufSize)

	for {
		n, sender, err := conn.ReadFromUDP(buf)
		if err != nil {
			if ctx.Err() != nil {
				return nil
			}

			return fmt.Errorf("failed to read UDP packet: %w", err)
		}

		messageType, receivedID, ok := parseMessage(buf[:n])
		if !ok {
			continue
		}

		if receivedID == instanceID {
			continue
		}

		var alivePeers []AlivePeer
		var changed bool

		switch messageType {
		case aliveMessage:
			alivePeers, changed = peers.Seen(
				receivedID,
				sender.IP.String(),
				time.Now(),
			)

		case byeMessage:
			alivePeers, changed = peers.Remove(receivedID)
		}

		if changed {
			printAlive(alivePeers)
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

	alive := []byte(aliveMessage + " " + instanceID)
	bye := []byte(byeMessage + " " + instanceID)

	for {
		if _, err := conn.WriteToUDP(alive, groupAddr); err != nil {
			if ctx.Err() != nil {
				return nil
			}

			return fmt.Errorf("failed to send heartbeat: %w", err)
		}

		select {
		case <-ctx.Done():
			if _, err := conn.WriteToUDP(bye, groupAddr); err != nil {
				return fmt.Errorf("failed to send BYE: %w", err)
			}

			return nil

		case <-ticker.C:
		}
	}
}

func parseMessage(data []byte) (string, string, bool) {
	messageType, instanceID, found := strings.Cut(string(data), " ")

	if !found ||
		(messageType != aliveMessage && messageType != byeMessage) ||
		!validInstanceID(instanceID) {
		return "", "", false
	}

	return messageType, instanceID, true
}

func validInstanceID(id string) bool {
	if len(id) != 32 {
		return false
	}

	_, err := hex.DecodeString(id)
	return err == nil
}
