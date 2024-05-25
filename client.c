#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define NI_MAXHOST 1025
#define NI_MAXSERV 32
#define MAX_MESSAGE_LENGTH 1024

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

#define MAX_MESSAGES 15

typedef struct {
    char* text;
    SDL_Color color;
} Message;

typedef struct {
    char name[50];
	int fd; //cfd
} ClientData;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font* gFont = NULL;
SDL_Color white = {255, 255, 255, 255};
Message messages[MAX_MESSAGES];
int messageCount = 1;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("Chat App", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF Error: %s\n", TTF_GetError());
        return false;
    }

    return true;
}

int close_all() {
    TTF_CloseFont(gFont);
    gFont = NULL;

    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;

    TTF_Quit();
    SDL_Quit();
    return 0;
}

bool loadMedia() {
    gFont = TTF_OpenFont("/usr/share/fonts/truetype/lato/Lato-Medium.ttf", 24); // Replace with the path to your font file
    if (gFont == NULL) {
        printf("Failed to load font! TTF Error: %s\n", TTF_GetError());
        return false;
    }

    return true;
}

void renderText(SDL_Renderer* renderer, const char* text, TTF_Font* font, SDL_Color color, int x, int y) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    if (textSurface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void *server_message_handler(void *arg) {
    // TODO: make messageCOunt a pointer maybe
    int fd = *((int *)arg);
    free(arg);
    ssize_t bytes_received;
    char received_message[1024];
    
    while(1) {
        memset(&received_message, 0, sizeof(received_message));
        if ((bytes_received = recv(fd, received_message, sizeof(received_message) - 1, 0)) > 0) {
            received_message[bytes_received] = '\0';
            printf("%s\n", received_message);

           
            if (messageCount < MAX_MESSAGES) {
                messages[messageCount].text = strdup(received_message); // Allocate and copy string
                messages[messageCount].color = (SDL_Color){255, 255, 255, 255}; // Set color (white)
                messageCount++;
            }
        
        } else if (bytes_received == 0) {
            // Connection closed by server
            printf("Server closed the connection\n");
            break;
        } else {
            perror("recv error:");
            break;
        }
    }
    return NULL;
}

ClientData client(const char *server_ip, int port, int * message_counter) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error:");
       
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short)port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton error:");
        close(fd);
        
    }

    // Connect to server
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("connect error:");
        close(fd);
      
    }

    char name[50];
    printf("Please enter a username:\n");

    if (fgets(name, sizeof(name), stdin) == NULL) {
        printf("Failed to read input.\n");
        close(fd);
    }
    // Remove newline character from name
    name[strcspn(name, "\n")] = 0;
    name[sizeof(name)] = '\0';

    if (send(fd, name, strlen(name), 0) == -1) {
        perror("send error:");
        close(fd);
    }

    printf("Entering server with username: %s\n", name);

    // Create a thread to receive messages from the server
    pthread_t tid;
    int *fd_ptr = malloc(sizeof(int)); // I have no idea why sending &fd wasnt working, so just do it manually
    if (fd_ptr == NULL) {
        perror("malloc error:");
        exit(EXIT_FAILURE);
    }
    *fd_ptr = fd;

    if (pthread_create(&tid, NULL, server_message_handler, fd_ptr) != 0) {
        perror("pthread_create error:");
        close(fd);
        free(fd_ptr);
    }

    ClientData client_data = {};
    strncpy(client_data.name, name, sizeof(client_data.name) - 1);
    client_data.name[sizeof(client_data.name) - 1] = '\0';
    client_data.fd = fd;
    return client_data;
}

int main(int argc, char* argv[]) {
    if (!init()) {
        printf("Failed to initialize!\n");
        return -1;
    }

    if (!loadMedia()) {
        printf("Failed to load media!\n");
        return -1;
    }

    ClientData client_data;
    if (argc > 1 && strcmp(argv[1], "client") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s client <server_ip> <port>\n", argv[0]);
            return -1;
        }

        const char *server_ip = argv[2];
        int port;
        sscanf(argv[3], "%d", &port);

        client_data = client(server_ip, port, &messageCount);
    } else {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }

    bool quit = false;
    SDL_Event e;
    int TypingyOffset = SCREEN_HEIGHT - 40;
    int yOffset = 40;

    char message[MAX_MESSAGE_LENGTH];
    char formatted_message[MAX_MESSAGE_LENGTH + 50 + 3];
    memset(&formatted_message, 0, sizeof(formatted_message));
    int message_len = 0;

    // Enable text input
    SDL_StartTextInput();
    messages[0].text = ">>> Starting the Group Chat";
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_TEXTINPUT) {
                // Append new text input to the message buffer
                if (message_len + strlen(e.text.text) < MAX_MESSAGE_LENGTH) {
                    strcat(message, e.text.text);
                    message_len += strlen(e.text.text);
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    // Clear the message buffer when Enter is pressed
                    message[strcspn(message, "\n")] = 0;
                    if (strlen(message) > 0) {
                        if (send(client_data.fd, message, strlen(message), 0) == -1) {
                            perror("send error:");
                            break;
                        }
                        memset(message, 0, sizeof(message));
                        message_len = 0;
                    }
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && message_len > 0) {
                    // Handle backspace
                    message[--message_len] = '\0';
                }
            }
        }

        // Render screen
        yOffset = 10;
        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
        SDL_RenderClear(gRenderer);
        
        for (int i = 0; i < messageCount; ++i) {
            renderText(gRenderer, messages[i].text, gFont, white, 10, yOffset);
            yOffset += 30; // Adjust as necessary for spacing
        }

        snprintf(formatted_message, sizeof(formatted_message), "(%s): %s", client_data.name, message);
        renderText(gRenderer, formatted_message, gFont, white, 10, TypingyOffset);

        SDL_RenderPresent(gRenderer);
    }

    // Disable text input
    SDL_StopTextInput();

    close_all();

    return 0;
}
