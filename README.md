# server-client
Basic server to N clients in C sockets.

## Usage

Compile both files:
```sh
gcc -o server server.c
gcc -o client client.c
```

Run the server:
```sh
./server
```

Run N clients:
```sh
./client client 8000
```
