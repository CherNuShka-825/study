package main

import (
	"context"
	"fmt"
	"sort"
	"sync"
	"time"
)

type Peer struct {
	IP       string
	LastSeen time.Time
}

type PeerStore struct {
	mu    sync.Mutex
	peers map[string]Peer
}

func NewPeerStore() *PeerStore {
	return &PeerStore{
		peers: make(map[string]Peer),
	}
}

func (s *PeerStore) Seen(
	instanceID string,
	ip string,
	now time.Time,
) ([]string, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	before := s.aliveIPs()

	s.peers[instanceID] = Peer{
		IP:       ip,
		LastSeen: now,
	}

	after := s.aliveIPs()

	return after, !sameIPs(before, after)
}

func (s *PeerStore) RemoveExpired(
	now time.Time,
	timeout time.Duration,
) ([]string, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()

	before := s.aliveIPs()

	for instanceID, peer := range s.peers {
		if now.Sub(peer.LastSeen) > timeout {
			delete(s.peers, instanceID)
		}
	}

	after := s.aliveIPs()

	return after, !sameIPs(before, after)
}

func (s *PeerStore) aliveIPs() []string {
	ips := make([]string, 0, len(s.peers))

	for _, peer := range s.peers {
		ips = append(ips, peer.IP)
	}

	sort.Strings(ips)

	return ips
}

func sameIPs(a, b []string) bool {
	if len(a) != len(b) {
		return false
	}

	for i := range a {
		if a[i] != b[i] {
			return false
		}
	}

	return true
}

func printAlive(ips []string) {
	fmt.Println("Alive copies:")

	if len(ips) == 0 {
		fmt.Println("none")
		return
	}

	for _, ip := range ips {
		fmt.Println(ip)
	}
}

func cleanupLoop(
	ctx context.Context,
	peers *PeerStore,
) {
	ticker := time.NewTicker(cleanupInterval)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return

		case now := <-ticker.C:
			aliveIPs, changed := peers.RemoveExpired(
				now,
				peerTimeout,
			)

			if changed {
				printAlive(aliveIPs)
			}
		}
	}
}
