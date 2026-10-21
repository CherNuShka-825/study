package main

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strconv"
	"time"
	"unicode/utf8"

	"github.com/CherNuShka-825/study/semester_5/seti/internal/protocol"
)

const (
	connectionTimeout = 5 * time.Second
	statusTimeout     = 30 * time.Second
	writeTimeout      = 30 * time.Second
)

func sendFile(filePath, host string, port int) error {
	file, err := os.Open(filePath)
	if err != nil {
		return fmt.Errorf("open file: %w", err)
	}
	defer func() {
		if err := file.Close(); err != nil {
			fmt.Fprintln(os.Stderr, "close file:", err)
		}
	}()

	info, err := file.Stat()
	if err != nil {
		return fmt.Errorf("stat file: %w", err)
	}

	if !info.Mode().IsRegular() {
		return fmt.Errorf("not a regular file: %s", filePath)
	}

	filename := filepath.Base(filePath)

	if !utf8.ValidString(filename) {
		return protocol.ErrInvalidUTF8
	}

	if len(filename) > protocol.MaxFilenameLength {
		return protocol.ErrFilenameTooLong
	}

	fileSize := info.Size()

	if fileSize < 0 || uint64(fileSize) > protocol.MaxFileSize {
		return fmt.Errorf("invalid file size: %d", fileSize)
	}

	address := net.JoinHostPort(host, strconv.Itoa(port))
	fmt.Printf("[client] connecting to %s\n", address)

	conn, err := net.DialTimeout("tcp", address, connectionTimeout)
	if err != nil {
		return fmt.Errorf("connect: %w", err)
	}
	fmt.Printf("[client] connected to %s\n", conn.RemoteAddr())
	defer func() {
		if err := conn.Close(); err != nil {
			fmt.Fprintln(os.Stderr, "close connection:", err)
		}
	}()

	writer := &timeoutWriter{
		conn:    conn,
		timeout: writeTimeout,
	}

	if err := protocol.WriteHeader(writer, filename, uint64(fileSize)); err != nil {
		return fmt.Errorf("send file header: %w", err)
	}
	fmt.Printf(
		"[client] sending file: %q (%d bytes)\n",
		filename,
		fileSize,
	)

	start := time.Now()
	sent, err := io.CopyN(writer, file, fileSize)
	if err != nil {
		return fmt.Errorf("send file (%d/%d bytes): %w", sent, fileSize, err)
	}
	elapsed := time.Since(start)
	fmt.Printf(
		"[client] sent %d bytes in %.3f seconds\n",
		sent,
		elapsed.Seconds(),
	)

	if err := conn.SetReadDeadline(time.Now().Add(statusTimeout)); err != nil {
		return fmt.Errorf("set read deadline: %w", err)
	}

	status, err := protocol.ReadStatus(conn)
	if err != nil {
		return fmt.Errorf("read server status: %w", err)
	}

	if status != protocol.StatusSuccess {
		return errors.New("server reported file transfer failure")
	}

	fmt.Printf("[client] file transferred successfully: %q\n", filename)

	return nil
}

type timeoutWriter struct {
	conn    net.Conn
	timeout time.Duration
}

func (w *timeoutWriter) Write(p []byte) (int, error) {
	if err := w.conn.SetWriteDeadline(time.Now().Add(w.timeout)); err != nil {
		return 0, fmt.Errorf("set write deadline: %w", err)
	}
	return w.conn.Write(p)
}
