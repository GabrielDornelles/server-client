# server-client
 
A simple group chat where a server receives messages from clients and send it to all connected clients through TCP:

![alt text](assets/image.png)

Right now it only works on a internal network (can communicate over your devices connected through wi-fi or ethernet cables).

## Usage

Install SDL:

```sh
sudo apt install libsdl2-dev libsdl2-image-dev
sudo apt-get install libsdl2-dev libsdl2-ttf-dev
```

Compile both files:
```sh
gcc -o server server.c
gcc -o client client.c -I/usr/include/SDL2 -lSDL2 -lSDL2_ttf
```

Run the server:
```sh
./server
```

Run N clients:
```sh
./client client 192.168.1.1 8000 # <client> <ip> <port> 
```

## Future

Code is not optimal/readable as it could be, keep in mind this is the first version that worked and I havent tried to rewrite/rearrange anything yet.