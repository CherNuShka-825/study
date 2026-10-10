package protocol

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"unicode/utf8"
)

const (
	MaxFilenameLength        = 4096
	MaxFileSize       uint64 = 1 << 40 // 1 TiB

	StatusFailure byte = 0
	StatusSuccess byte = 1
)

var (
	ErrEmptyFilename   = errors.New("filename is empty")
	ErrFilenameTooLong = errors.New("filename is too long")
	ErrInvalidUTF8     = errors.New("filename is not valid UTF-8")
	ErrFileTooLarge    = errors.New("file is too large")
	ErrInvalidStatus   = errors.New("invalid status")
)

func WriteHeader(w io.Writer, filename string, fileSize uint64) error {
	if filename == "" {
		return ErrEmptyFilename
	}

	if !utf8.ValidString(filename) {
		return ErrInvalidUTF8
	}

	filenameBytes := []byte(filename)

	if len(filenameBytes) > MaxFilenameLength {
		return ErrFilenameTooLong
	}

	if fileSize > MaxFileSize {
		return ErrFileTooLarge
	}

	var filenameLength [2]byte
	binary.BigEndian.PutUint16(filenameLength[:], uint16(len(filenameBytes))) // 0x012C -> [0]:0x01, [1]:0x2C

	if err := writeFull(w, filenameLength[:]); err != nil {
		return fmt.Errorf("write filename length: %w", err)
	}

	if err := writeFull(w, filenameBytes); err != nil {
		return fmt.Errorf("write filename: %w", err)
	}

	var size [8]byte
	binary.BigEndian.PutUint64(size[:], fileSize)

	if err := writeFull(w, size[:]); err != nil {
		return fmt.Errorf("write file size: %w", err)
	}

	return nil
}

func ReadHeader(r io.Reader) (string, uint64, error) {
	var filenameLengthBytes [2]byte
	if _, err := io.ReadFull(r, filenameLengthBytes[:]); err != nil {
		return "", 0, fmt.Errorf("read filename length: %w", err)
	}

	filenameLength := binary.BigEndian.Uint16(filenameLengthBytes[:])

	if filenameLength == 0 {
		return "", 0, ErrEmptyFilename
	}

	if filenameLength > MaxFilenameLength {
		return "", 0, ErrFilenameTooLong
	}

	filenameBytes := make([]byte, filenameLength)
	if _, err := io.ReadFull(r, filenameBytes[:]); err != nil {
		return "", 0, fmt.Errorf("read filename: %w", err)
	}

	if !utf8.Valid(filenameBytes) {
		return "", 0, ErrInvalidUTF8
	}

	var fileSizeBytes [8]byte

	if _, err := io.ReadFull(r, fileSizeBytes[:]); err != nil {
		return "", 0, fmt.Errorf("read file size: %w", err)
	}

	fileSize := binary.BigEndian.Uint64(fileSizeBytes[:])

	if fileSize > MaxFileSize {
		return "", 0, ErrFileTooLarge
	}

	return string(filenameBytes), fileSize, nil
}

func WriteStatus(w io.Writer, status byte) error {
	if status != StatusFailure && status != StatusSuccess {
		return ErrInvalidStatus
	}

	if err := writeFull(w, []byte{status}); err != nil {
		return fmt.Errorf("write status: %w", err)
	}

	return nil
}

func ReadStatus(r io.Reader) (byte, error) {
	var status [1]byte

	if _, err := io.ReadFull(r, status[:]); err != nil {
		return 0, fmt.Errorf("read status: %w", err)
	}

	switch status[0] {
	case StatusSuccess, StatusFailure:
		return status[0], nil
	default:
		return 0, ErrInvalidStatus
	}
}

func writeFull(w io.Writer, data []byte) error {
	n, err := w.Write(data)
	if err != nil {
		return err
	}

	if n != len(data) {
		return io.ErrShortWrite
	}

	return nil
}
