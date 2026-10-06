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

type AlivePeer struct {
	InstanceID string
	IP         string
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
) ([]AlivePeer, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()

	peer, exists := s.peers[instanceID]

	if !exists && len(s.peers) >= maxPeers {
		return s.alivePeers(), false
	}

	changed := !exists || peer.IP != ip

	s.peers[instanceID] = Peer{
		IP:       ip,
		LastSeen: now,
	}

	return s.alivePeers(), changed
}

func (s *PeerStore) RemoveExpired(
	now time.Time,
	timeout time.Duration,
) ([]AlivePeer, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()

	changed := false

	for instanceID, peer := range s.peers {
		if now.Sub(peer.LastSeen) > timeout {
			s.remove(instanceID)
			changed = true
		}
	}

	return s.alivePeers(), changed
}

func (s *PeerStore) Remove(instanceID string) ([]AlivePeer, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()

	changed := s.remove(instanceID)

	return s.alivePeers(), changed
}

func (s *PeerStore) remove(instanceID string) bool {
	if _, exists := s.peers[instanceID]; !exists {
		return false
	}

	delete(s.peers, instanceID)
	return true
}

func (s *PeerStore) alivePeers() []AlivePeer {
	peers := make([]AlivePeer, 0, len(s.peers))

	for instanceID, peer := range s.peers {
		peers = append(peers, AlivePeer{
			InstanceID: instanceID,
			IP:         peer.IP,
		})
	}

	sort.Slice(peers, func(i, j int) bool {
		if peers[i].IP == peers[j].IP {
			return peers[i].InstanceID < peers[j].InstanceID
		}

		return peers[i].IP < peers[j].IP
	})

	return peers
}

func printAlive(peers []AlivePeer) {
	fmt.Println("Alive copies:")

	if len(peers) == 0 {
		fmt.Println("none")
		return
	}

	for _, peer := range peers {
		fmt.Printf("%s %s\n", peer.IP, peer.InstanceID)
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
			alivePeers, changed := peers.RemoveExpired(
				now,
				peerTimeout,
			)

			if changed {
				printAlive(alivePeers)
			}
		}
	}
}
