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

#include <sys/stat.h>
#include <X11/XWDFile.h>

#include "plugin.h"
#include "controller.h"

int screen_width = -1, screen_height = -1;
char * file_path = NULL;

void get_screen_resolution() {
    if (screen_width != -1 && screen_height != -1)
        return;
    int screen_size;
    CoreDoCommand(M64CMD_CORE_STATE_QUERY, M64CORE_VIDEO_SIZE, &screen_size);
    screen_width = (screen_size >> 16) & 0xffff;
    screen_height = screen_size & 0xffff;

    DebugMessage(M64MSG_INFO, "screen size %i x %i.", screen_width, screen_height);
}

void get_fb_file_path() {
    if(file_path != NULL)
        return;
    
    DebugMessage(M64MSG_INFO, "getting fb path...");
    file_path = getenv("XVFB_FB_PATH");
    if (!file_path) {
        DebugMessage(M64MSG_ERROR, "file path could not be loaded from XVFB_FB_PATH.");
        DebugMessage(M64MSG_INFO, "file path could not be loaded from XVFB_FB_PATH.");
        file_path = NULL;
    }
    else {
        DebugMessage(M64MSG_INFO, "fb path: %s.", file_path);

    }
}

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

int receive_basic(int socket, char *msg)
{
    int size_recv, total_size = 0;
    char chunk[CHUNK_SIZE];
    while (1)
    {
        memset(chunk, 0, CHUNK_SIZE); // clear the variable
        if ((size_recv = recv(socket, chunk, CHUNK_SIZE, 0)) < 0)
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
                msg[i] = chunk[i];
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
    msg[total_size] = '\0';

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

int get_emulator_image(unsigned char** image) {
    int image_size = screen_width * screen_height * 3;
    // int image_size = 4;
    // char * stop = "!#~~Stop it]";
    int buffer_size = image_size;

    unsigned char * pixels = (unsigned char *) malloc(buffer_size);
    // CoreDoCommand(M64CMD_READ_SCREEN, 1, pixels);
    // // count = 0;
    // // for (int x = 0; x<buffer_size; x++) {
    // //     if (pixels[x] == 0) {
    // //         count++;
    // //     }
    // // }
    // // DebugMessage(M64MSG_INFO, "image zero afterwards: %i", count);
    // // strcpy(image + image_size, stop);
    // // DebugMessage(M64MSG_INFO, "image in between: %i\n", image);
    // return buffer_size;
    get_fb_file_path();
    // DebugMessage(M64MSG_INFO, "now reading file.");
    FILE * fb_file = fopen(file_path, "r");

    struct stat st;
    stat(file_path, &st);
    int file_size = st.st_size;
    DebugMessage(M64MSG_INFO, "file read, bits in file: %i.", file_size);
    unsigned char * buf = (unsigned char *) malloc(file_size);
    // DebugMessage(M64MSG_INFO, "file opened.");
    if(file_size != fread(buf, 1, file_size, fb_file)) {
        DebugMessage(M64MSG_ERROR, "fb file could not be read.");
        return -1;
    }
    int header_size = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    int bits_per_pixel = (buf[44] << 24) | (buf[45] << 16) | (buf[46] << 8) | buf[47];
    int bits_per_rgb = (buf[68] << 24) | (buf[69] << 16) | (buf[70] << 8) | buf[71];
    int ncolors = (buf[76] << 24) | (buf[77] << 16) | (buf[78] << 8) | buf[79];
       /* Close the file */
    DebugMessage(M64MSG_INFO, "file read, bits per pixel: %i per rgb: %i.", bits_per_pixel, bits_per_rgb);
    int pixel_offset = header_size + ncolors * 12;

    DebugMessage(M64MSG_INFO, "offset: %i, header size: %i, nc: %i.", pixel_offset, header_size, ncolors * 12);
    memcpy(pixels, buf + pixel_offset, buffer_size);
    // int count = 0;
    int pc = 0;
    for (int x = pixel_offset; x<file_size && pc < buffer_size; x++) {
        // DebugMessage(M64MSG_INFO, "x: %i buf value: %i, diff: %i", x, buf[x], (x - pixel_offset));
        if ((x - pixel_offset - 3) % 4 == 0) {
            // DebugMessage(M64MSG_INFO, "skip");

            continue;
        }
        pixels[pc++] = buf[x];
        // sleep(1);
    }
    // DebugMessage(M64MSG_INFO, "image zero: %i", count);
    if( EOF == fclose(fb_file) ) {
        free(pixels);
        free(buf);
        return -1;
    }
    free(buf);
    *image = pixels;
    // DebugMessage(M64MSG_INFO, "returning.");
    return buffer_size;
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
    get_screen_resolution();
    char msg[48];
    int rec_len = receive_basic(client_socket, msg);
    unsigned char * image;
    // DebugMessage(M64MSG_INFO, "getting image.");
    // DebugMessage(M64MSG_INFO, "image before: %i", image);
    int buffer_size = get_emulator_image(&image);
    // int count = 0;
    // for (int x = 0; x<buffer_size; x++) {
    //     if (image[x] == 0) {
    //         count++;
    //     }
    // }
    // DebugMessage(M64MSG_INFO, "image zero when send: %i", count);

    // for (int x = 0; x<5; x++) {
        // DebugMessage(M64MSG_INFO, "%i\n", image[x]);
    // // }
    DebugMessage(M64MSG_INFO, "now sending stuff of size %i.", buffer_size);

    // if (send(client_socket, image, 1, 0) < 0)

    if (send(client_socket, image, buffer_size, 0) < 0)
    {
        return -1;
    }
    free(image);
    // DebugMessage(M64MSG_INFO, "done sending stuff.");
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
