package main

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/CherNuShka-825/study/semester_5/seti/internal/protocol"
)

func openUploads(dir string) (*os.Root, error) {
	if err := os.MkdirAll(dir, 0755); err != nil {
		return nil, fmt.Errorf("create uploads directory: %w", err)
	}

	info, err := os.Lstat(dir)
	if err != nil {
		return nil, fmt.Errorf("stat uploads directory: %w", err)
	}
	if !info.IsDir() {
		return nil, errors.New("uploads is not a directory")
	}

	uploads, err := os.OpenRoot(dir)
	if err != nil {
		return nil, fmt.Errorf("open uploads directory: %w", err)
	}
	return uploads, nil
}

func receiveFile(conn net.Conn, uploads *os.Root) (retErr error) {
	reader := &timeoutReader{
		conn:    conn,
		timeout: readTimeout,
	}
	filename, fileSize, err := protocol.ReadHeader(reader)
	if err != nil {
		return fmt.Errorf("read header: %w", err)
	}

	if filename == "." || filename == ".." || strings.ContainsAny(filename, `/\`) {
		return fmt.Errorf("invalid filename: %q", filename)
	}

	file, savedFilename, err := createUniqueFile(uploads, filename)
	if err != nil {
		return err
	}
	defer func() {
		if err := file.Close(); err != nil {
			retErr = errors.Join(retErr, fmt.Errorf("close file: %w", err))
		}
		if retErr != nil {
			if err := uploads.Remove(savedFilename); err != nil {
				retErr = errors.Join(retErr, fmt.Errorf("remove incomplete file: %w", err))
			}
		}
	}()

	counter := &countingReader{
		reader: reader,
	}
	done := make(chan struct{})
	finished := make(chan struct{})
	start := time.Now()

	go func() {
		defer close(finished)
		reportSpeeds(conn.RemoteAddr().String(), &counter.received, start, done)
	}()
	defer func() {
		close(done)
		<-finished
	}()

	received, err := io.CopyN(file, counter, int64(fileSize))
	if err != nil {
		return fmt.Errorf("receive file (%d/%d bytes): %w", received, fileSize, err)
	}
	if uint64(received) != fileSize {
		return fmt.Errorf("file size mismatch: received %d, expected %d", received, fileSize)
	}
	return nil
}

func createUniqueFile(uploads *os.Root, filename string) (*os.File, string, error) {
	ext := filepath.Ext(filename)
	if ext == filename {
		ext = ""
	}
	name := strings.TrimSuffix(filename, ext)

	for i := range maxFilenameAttempts {
		newFilename := filename
		if i > 0 {
			newFilename = fmt.Sprintf("%s_%d%s", name, i, ext)
		}
		file, err := uploads.OpenFile(newFilename, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0600)
		if err == nil {
			return file, newFilename, nil
		}
		if !errors.Is(err, os.ErrExist) {
			return nil, "", fmt.Errorf("create file %s: %w", newFilename, err)
		}
	}
	return nil, "", fmt.Errorf("no available filename for %s", filename)
}
