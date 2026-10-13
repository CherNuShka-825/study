package main

import (
	"errors"
	"flag"
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
	defaultServerHost    = "127.0.0.1"
	defaultServerPort    = 6767
	readTimeout          = 30 * time.Second
	setConnectionTimeout = 5 * time.Second
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run() error {
	host := flag.String(
		"host",
		defaultServerHost,
		"server hostname or IP address",
	)

	port := flag.Int(
		"port",
		defaultServerPort,
		"server TCP port",
	)

	flag.Parse()

	if flag.NArg() != 1 {
		return fmt.Errorf(
			"usage: %s [-host host] [-port port] <file>",
			os.Args[0],
		)
	}

	filePath := flag.Arg(0)

	if *host == "" {
		return fmt.Errorf("host is empty")
	}

	if *port < 1 || *port > 65535 {
		return fmt.Errorf("invalid port: %d", *port)
	}

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
		return fmt.Errorf("not a regular file %s", filePath)
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

	address := net.JoinHostPort(*host, strconv.Itoa(*port))

	conn, err := net.DialTimeout("tcp", address, setConnectionTimeout)
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

	if err := conn.SetReadDeadline(time.Now().Add(readTimeout)); err != nil {
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
