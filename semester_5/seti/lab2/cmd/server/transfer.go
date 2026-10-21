package main

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"time"
	"unicode/utf8"

	"github.com/CherNuShka-825/study/semester_5/seti/internal/protocol"
)

const baseFileName = "f"

func openUploads(dir string) (*os.Root, error) {
	if err := os.MkdirAll(dir, 0o755); err != nil {
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
	start := time.Now()
	if err != nil {
		return err
	}
	clientAddr := conn.RemoteAddr().String()
	defer func() {
		if err := file.Close(); err != nil {
			retErr = errors.Join(retErr, fmt.Errorf("close file: %w", err))
		}

		if retErr != nil {
			if err := uploads.Remove(savedFilename); err != nil {
				retErr = errors.Join(retErr, fmt.Errorf("remove incomplete file: %w", err))
			}
			return
		}

		fmt.Printf(
			"[%s] file saved: %q (%d bytes), time: %.3f s\n",
			clientAddr,
			savedFilename,
			fileSize,
			time.Since(start).Seconds(),
		)
	}()

	fmt.Printf(
		"[%s] transfer started: %q (%d bytes)\n",
		clientAddr,
		filename,
		fileSize,
	)

	counter := &countingReader{
		reader: reader,
	}
	done := make(chan struct{})
	finished := make(chan struct{})

	go func() {
		defer close(finished)
		reportSpeeds(clientAddr, &counter.received, start, done)
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

func truncateUTF8(s string, maxBytes int) string {
	if maxBytes <= 0 {
		return ""
	}

	if len(s) <= maxBytes {
		return s
	}

	end := maxBytes

	for end > 0 && !utf8.RuneStart(s[end]) {
		end--
	}

	return s[:end]
}

func createUniqueFile(uploads *os.Root, filename string) (*os.File, string, error) {
	var fs syscall.Statfs_t

	if err := syscall.Statfs(uploadsDir, &fs); err != nil {
		return nil, "", fmt.Errorf("get filesystem info: %w", err)
	}
	maxNameBytes := int(fs.Namelen)

	if maxNameBytes <= 0 {
		return nil, "", fmt.Errorf("invalid filename limit: %d", maxNameBytes)
	}

	ext := filepath.Ext(filename)
	if ext == filename {
		ext = ""
	}

	name := strings.TrimSuffix(filename, ext)

	for i := range maxFilenameAttempts {
		suffix := ""

		if i > 0 {
			suffix = fmt.Sprintf("_%d", i)
		}

		available := maxNameBytes - len(suffix)

		if available < 1 {
			break
		}

		currentExt := ext
		currentName := truncateUTF8(name, available-len(currentExt))

		if currentName == "" {
			currentExt = ""
			currentName = truncateUTF8(filename, available)
		}

		if currentName == "" {
			currentName = baseFileName
		}

		newFilename := currentName + suffix + currentExt

		file, err := uploads.OpenFile(
			newFilename,
			os.O_WRONLY|os.O_CREATE|os.O_EXCL,
			0o600,
		)

		if err == nil {
			return file, newFilename, nil
		}

		if !errors.Is(err, os.ErrExist) {
			return nil, "", fmt.Errorf("create file %q: %w", newFilename, err)
		}
	}

	return nil, "", fmt.Errorf("no available filename for %q", filename)
}
