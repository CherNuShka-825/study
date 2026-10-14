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

	conn, err := net.DialTimeout("tcp", address, connectionTimeout)
	if err != nil {
		return fmt.Errorf("connect: %w", err)
	}
	defer func() {
		if err := conn.Close(); err != nil {
			fmt.Fprintln(os.Stderr, "close connection:", err)
		}
	}()

	if err := protocol.WriteHeader(conn, filename, uint64(fileSize)); err != nil {
		return fmt.Errorf("send file header: %w", err)
	}

	sent, err := io.CopyN(conn, file, fileSize)
	if err != nil {
		return fmt.Errorf("send file (%d/%d bytes): %w", sent, fileSize, err)
	}

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

	fmt.Println("file transferred successfully")

	return nil
}
