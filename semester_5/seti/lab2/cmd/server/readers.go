package main

import (
	"fmt"
	"io"
	"net"
	"sync/atomic"
	"time"
)

type countingReader struct {
	reader   io.Reader
	received atomic.Uint64
}

func (r *countingReader) Read(p []byte) (int, error) {
	n, err := r.reader.Read(p)
	r.received.Add(uint64(n))
	return n, err
}

type timeoutReader struct {
	conn    net.Conn
	timeout time.Duration
}

func (r *timeoutReader) Read(p []byte) (int, error) {
	if err := r.conn.SetReadDeadline(time.Now().Add(r.timeout)); err != nil {
		return 0, fmt.Errorf("set read deadline: %w", err)
	}
	return r.conn.Read(p)
}
