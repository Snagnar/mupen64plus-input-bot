#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "plugin.h"
#include "controller.h"

int socket_create(char *host, int portno)
{
    struct sockaddr_in serv_addr;
    int sockfd;

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        DebugMessage(M64MSG_ERROR, "ERROR opening socket");
        return -1;
    }
    /* fill in the structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        DebugMessage(M64MSG_INFO, "ERROR binding.", host, inet_ntoa(serv_addr.sin_addr), portno);
        return -1;
    }

    if (listen(sockfd, 1) != 0)
    {
        DebugMessage(M64MSG_INFO, "ERROR listening with addr %s %s server.", host, inet_ntoa(serv_addr.sin_addr), portno);
        return -1;
    }

    return sockfd;
}

void clear_controller(int Control)
{
    controller[Control].buttons.R_DPAD = 0;
    controller[Control].buttons.L_DPAD = 0;
    controller[Control].buttons.D_DPAD = 0;
    controller[Control].buttons.U_DPAD = 0;
    controller[Control].buttons.START_BUTTON = 0;
    controller[Control].buttons.Z_TRIG = 0;
    controller[Control].buttons.B_BUTTON = 0;
    controller[Control].buttons.A_BUTTON = 0;
    controller[Control].buttons.R_CBUTTON = 0;
    controller[Control].buttons.L_CBUTTON = 0;
    controller[Control].buttons.D_CBUTTON = 0;
    controller[Control].buttons.U_CBUTTON = 0;
    controller[Control].buttons.R_TRIG = 0;
    controller[Control].buttons.L_TRIG = 0;
    controller[Control].buttons.X_AXIS = 0;
    controller[Control].buttons.Y_AXIS = 0;
}

#define CHUNK_SIZE 48

int receive_basic(int s, char *a)
{
    int size_recv, total_size = 0;
    char chunk[CHUNK_SIZE];
    while (1)
    {
        memset(chunk, 0, CHUNK_SIZE); // clear the variable
        if ((size_recv = recv(s, chunk, CHUNK_SIZE, 0)) < 0)
        {
            break;
        }
        else
        {
            if (chunk[0] == 0)
                break;
            total_size += size_recv;
            char received_all = 0;
            for (int i = 0; i < CHUNK_SIZE && chunk[i] != 0; i++)
            {
                a[i] = chunk[i];
                if (chunk[i] == '#')
                {
                    received_all = 1;
                    break;
                }
            }
            if (received_all == 1)
                break;
        }
    }
    a[total_size] = '\0';

    return total_size;
}

int *parse_message(char *msg, int rec_len)
{
    static int values[17];
    char number[16];
    int ni = 0, vi = 0;
    for (int x = 0; x < rec_len; x++)
    {
        if (msg[x] == '|' || msg[x] == '#')
        {
            number[ni] = '\0';
            values[vi++] = atoi(number);
            memset(number, 0, sizeof number);
            ni = 0;
        }
        else
            number[ni++] = msg[x];
    }
    return values;
}

static int frames_to_skip = 0;

int read_controller(int Control, int socket, int client_socket)
{
    struct sockaddr_in client;
    if (--frames_to_skip > 0)
    {
        return client_socket;
    }
    socklen_t namelen = sizeof(client);
    if (client_socket < 0 && (client_socket = accept(socket, (struct sockaddr *)&client, &namelen)) == -1)
    {
        return -1;
    }
    char msg[48];
    int rec_len = receive_basic(client_socket, msg);
    if (send(client_socket, "a", 1, 0) < 0)
    {
        return -1;
    }
    int *values;
    values = parse_message(msg, rec_len);

    controller[Control].buttons.X_AXIS = values[0];
    controller[Control].buttons.Y_AXIS = values[1];
    controller[Control].buttons.A_BUTTON = values[2];
    controller[Control].buttons.B_BUTTON = values[3];
    controller[Control].buttons.R_TRIG = values[4];
    controller[Control].buttons.L_TRIG = values[5];
    controller[Control].buttons.Z_TRIG = values[6];
    controller[Control].buttons.R_CBUTTON = values[7];
    controller[Control].buttons.L_CBUTTON = values[8];
    controller[Control].buttons.D_CBUTTON = values[9];
    controller[Control].buttons.U_CBUTTON = values[10];
    controller[Control].buttons.R_DPAD = values[11];
    controller[Control].buttons.L_DPAD = values[12];
    controller[Control].buttons.D_DPAD = values[13];
    controller[Control].buttons.U_DPAD = values[14];
    controller[Control].buttons.START_BUTTON = values[15];
    frames_to_skip = values[16];

    return client_socket;
}
