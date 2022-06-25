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

// #include "json.h"
// #include "json_tokener.h"

int socket_create(char *host, int portno)
{
    printf("got input: %s : %i\n", host, portno);
    // struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        DebugMessage(M64MSG_ERROR, "ERROR opening socket");
        return -1;
    }
    // if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
    // {
    //     DebugMessage(M64MSG_ERROR, "Error setting flags");
    //     return -1;
    // }
    /* fill in the structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    // memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    /* connect the socket */
    // if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    // {
    //     DebugMessage(M64MSG_INFO, "ERROR connecting to %s (%s) on port %d, please start bot server.", host, inet_ntoa(serv_addr.sin_addr), portno);
    //     return -1;
    // }
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
    // printf("reading basic...");
    // loop
    while (1)
    {
        // printf("reading...\n");
        memset(chunk, 0, CHUNK_SIZE); // clear the variable
        if ((size_recv = recv(s, chunk, CHUNK_SIZE, 0)) < 0)
        {
            break;
        }
        else
        {
            if (chunk[0] == 0)
                break;
            // memcpy(a + (total_size * sizeof(char)), chunk, size_recv);
            total_size += size_recv;
            char received_all = 0;
            for (int i = 0; i < CHUNK_SIZE && chunk[i] != 0; i++)
            {
                // printf(">%c<\n", chunk[i]);
                // printf("same equal: %i\n", '#' == '#');
                // printf("same equal2: %i\n", chunk[i] == chunk[i]);
                // printf("next to another: >%c< >%c<\n", chunk[i], '#');
                a[i] = chunk[i];
                if (chunk[i] == '#')
                {
                    // printf("arrived there\n");
                    received_all = 1;
                    break;
                }
                // else
                // {
                //     // printf("not equal\n");
                // }
            }
            // printf("now rall: %i\n", received_all);
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
    // printf("Message: >%s< len: %i\n", msg, rec_len);
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
static clock_t start, end;

int read_controller(int Control, int socket, int client_socket)
{
    // time(&start);
    start = clock();
    struct sockaddr_in client;
    if (--frames_to_skip > 0)
    {
        return client_socket;
    }
    socklen_t namelen = sizeof(client);
    if (client_socket < 0 && (client_socket = accept(socket, (struct sockaddr *)&client, &namelen)) == -1)
    {
        // DebugMessage(M64MSG_ERROR, "ERROR storing complete response from socket");
        return -1;
    }
    // printf("accepted!");
    char msg[48];
    int rec_len = receive_basic(client_socket, msg);
    if (send(client_socket, "", 1, 0) < 0)
    {
        return -1;
    }
    // close(client_socket);
    // printf("reiceved all!\n");
    int *values;
    values = parse_message(msg, rec_len);
    // printf("parsed all!\n");
    // for (int x = 0; x < 17; x++)
    //     printf("%i   ", values[x]);
    // printf("\n");
    // fflush(stdout);

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
    // time(&end);
    printf("time until called %f seconds\n", (double)(start - end) / CLOCKS_PER_SEC);
    printf("%f calls per second\n", CLOCKS_PER_SEC / (double)(start - end));
    end = clock();
    printf("all took %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    return client_socket;
    // printf("frames to skip: %i\n", frames_to_skip);
    // sleep(1);

    //     int sockfd = socket_connect(controller[Control].host, controller[Control].port);

    //     if (sockfd == -1) {
    //         clear_controller(Control);
    //         return;
    //     }

    //     int bytes, sent, received, total;
    //     char message[1024], response[4096]; // allocate more space than required.
    //     sprintf(message, "GET /%d HTTP/1.0\r\n\r\n", Control);

    //     /* send the request */
    //     total = strlen(message);
    //     sent = 0;
    //     do {
    //         bytes = write(sockfd, message + sent, total - sent);
    //         if (bytes < 0)
    //             DebugMessage(M64MSG_ERROR, "ERROR writing message to socket");
    //         if (bytes == 0)
    //             break;
    //         sent += bytes;
    //     } while (sent < total);

    //     /* receive the response */
    //     memset(response, 0, sizeof(response));
    //     total = sizeof(response) - 1;
    //     received = 0;
    //     do {
    //         bytes = read(sockfd, response + received, total - received);
    //         if (bytes < 0)
    //             DebugMessage(M64MSG_ERROR, "ERROR reading response from socket");
    //         if (bytes == 0)
    //             break;
    //         received += bytes;
    //     } while (received < total);

    //     if (received == total)
    //         DebugMessage(M64MSG_ERROR, "ERROR storing complete response from socket");

    // /* print the response */
    // #ifdef _DEBUG
    //     DebugMessage(M64MSG_INFO, response);
    // #endif

    //     /* parse the http response */
    //     char *body = strstr(response, "\r\n\r\n");
    //     body = body + 4;

    //     /* parse the body of the response */
    //     json_object *jsonObj = json_tokener_parse(body);

    // /* print the object */
    // #ifdef _DEBUG
    //     DebugMessage(M64MSG_INFO, json_object_to_json_string(jsonObj));
    // #endif
    // sleep(2);
    // controller[Control].buttons.START_BUTTON = a % 2;
    // a++;
    // a %= 2;

    //     controller[Control].buttons.R_DPAD =
    //         json_object_get_int(json_object_object_get(jsonObj, "R_DPAD"));
    //     controller[Control].buttons.L_DPAD =
    //         json_object_get_int(json_object_object_get(jsonObj, "L_DPAD"));
    //     controller[Control].buttons.D_DPAD =
    //         json_object_get_int(json_object_object_get(jsonObj, "D_DPAD"));
    //     controller[Control].buttons.U_DPAD =
    //         json_object_get_int(json_object_object_get(jsonObj, "U_DPAD"));
    //     controller[Control].buttons.START_BUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "START_BUTTON"));
    //     controller[Control].buttons.Z_TRIG =
    //         json_object_get_int(json_object_object_get(jsonObj, "Z_TRIG"));
    //     controller[Control].buttons.B_BUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "B_BUTTON"));
    //     controller[Control].buttons.A_BUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "A_BUTTON"));
    //     controller[Control].buttons.R_CBUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "R_CBUTTON"));
    //     controller[Control].buttons.L_CBUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "L_CBUTTON"));
    //     controller[Control].buttons.D_CBUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "D_CBUTTON"));
    //     controller[Control].buttons.U_CBUTTON =
    //         json_object_get_int(json_object_object_get(jsonObj, "U_CBUTTON"));
    //     controller[Control].buttons.R_TRIG =
    //         json_object_get_int(json_object_object_get(jsonObj, "R_TRIG"));
    //     controller[Control].buttons.L_TRIG =
    //         json_object_get_int(json_object_object_get(jsonObj, "L_TRIG"));
    //     controller[Control].buttons.X_AXIS =
    //         json_object_get_int(json_object_object_get(jsonObj, "X_AXIS"));
    //     controller[Control].buttons.Y_AXIS =
    //         json_object_get_int(json_object_object_get(jsonObj, "Y_AXIS"));

    //     // Mark the JSON object to be freed:
    //     json_object_put(jsonObj);
}
