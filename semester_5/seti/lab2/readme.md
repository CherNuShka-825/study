# file transfer on TCP with speed calculating

## protocol

### client

  Multi-byte integer fields use big-endian byte order.
  | length name: 2 bytes
  | name: N bytes (0 < N <= 4096 B)
  | file size: 8 bytes
  V file data: M bytes (0 <= M <= 1 TB)

### server

  status: 1 byte
    0x00 - success
    0x01 - failure

## How to run
  
### run client

  ```
  usage: go run ./cmd/client [-host host] [-port port] <file>
  default host: 127.0.0.1
  default port: 6767
  ```

  example:

  ```
  go run ./cmd/client -host 127.0.0.1 -port 6767 ./test.txt
  ```

### run server

  ```
  usage: go run ./cmd/server \[-port PORT]
  default port: 6767

  ```

  example:

  ```
  go run ./cmd/server -port 6767
  ```
