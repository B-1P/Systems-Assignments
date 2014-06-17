#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>


#include "crc16.h"
#include "xmodemserver.h"
#include "helper.h"
#include "wrappers.h"

#ifndef PORT
#define PORT 57171
#endif

#define FILE_DIR "filestore"
#define MAXCLIENTS 255

Client *start = NULL; // Pointer to the start of the list of clients;

/*Search the first inbuf characters of buf for a network newline ("\r\n").
  Return the location of the '\r' if the network newline is found,
  or -1 otherwise.
  Definitely do not use strchr or any other string function in here. (Why not?)
*/

int find_network_newline(char *buf, int inbuf) {
    // Step 1: write this function
    int count;
    for (count = 0; count < inbuf; count++) {
        if (buf[count] == '\r') {
            if (buf[count + 1] == '\n') {
                return count;
            }
        }
    }
    return -1; // return the location of '\r' if found
}

/*
 * Removes client drop from the linked list and frees up memory.
 */
Client *drop_client(Client *drop) {
    Client *temp = NULL;

    // Close client's file descriptors.
    fclose(drop->fp);
    Close(drop->fd);

    if (drop == start) {
        start = drop->next;
    }

    if (drop->next != NULL) {
        drop->next->prev = drop->prev;
    }

    if (drop->prev != NULL) {
        drop->prev->next = drop->next;
    }

    temp = drop->next;
    free(drop);
    return (temp);
}

int main() {
    int listenfd, clientfd, maxfd;
    fd_set cset, rset;
    Client *clients = NULL;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    int one = 1;

    unsigned char temp_block_num, temp_block_inverse;
    unsigned short temp_crc, block_crc;
    char temp = 'C';

    /*If a client disconnects while we are writing an ACK/NAK to them it sends
     * a SIGPIPE which needs to be taken care of
     */
    signal(SIGPIPE, SIG_IGN);


    //Setup the main listening socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)))
            == -1) {
        perror("setsockopt");
    }
    Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    Listen(listenfd, 5);
    FD_ZERO(&cset);
    FD_SET(listenfd, &cset);
    maxfd = listenfd;


    while (1) {
        /*
        * Accept any clients that want to connect.
        */
        rset = cset;
        Select(maxfd + 1, &rset, NULL, NULL, 0);

        // Check for new clients.
        if (FD_ISSET(listenfd, &rset)) {
            printf("New Client Connected\n");
            Client *temp = Malloc(sizeof(Client));
            clilen = sizeof(cliaddr);
            clientfd = Accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
            FD_SET(clientfd, &cset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }

            // Populate the client struct with default starter data.
            temp->fd = clientfd;
            temp->inbuf = 0;
            temp->state = initial;
            temp->next = start;
            temp->prev = NULL;
            if (start) {
                start->prev = temp;
            }
            start = temp;
            continue;
        }

        /*
         * Loop through all the clients in the linked list,
         * checking their file descriptors for activity.
         */

        clients = start;
        while (clients) {
            clientfd = clients->fd;
            if (FD_ISSET(clientfd, &rset)) {
                /*
                 * Using switch statement for states is too messy in terms of proper flow control
                 * so just using if statements instead; they're cleaner and easier to follow.
                 */
                if (clients->state == initial) {

                    // Read name of the file being sent.
                    clients->inbuf += Read(clientfd, &(clients->filename)[clients->inbuf], 20 - clients->inbuf);

                    if (find_network_newline(clients->filename, clients->inbuf) > 0) {

                        clients->filename[clients->inbuf - 2] = '\0';

                        // Open/create a file with the name.
                        clients->fp = open_file_in_dir(clients->filename, FILE_DIR);

                        // Assign defaults to client struct.
                        clients->state = pre_block;
                        clients->inbuf = 0;
                        clients->current_block = 1;

                        // Let the client know that the server is ready for the data.
                        temp = 'C';
                        Write(clientfd, &temp, 1);
                    }
                }

                if (clients->state == pre_block) {
                    // Find out what state the client is in.
                    Read(clientfd, (clients->buf), 1);
                    switch (clients->buf[0]) {
                    case EOT:
                        clients->state = finished;
                        break;
                    case SOH:
                        clients->state = get_block;
                        clients->blocksize = 132;
                        break;
                    case STX:
                        clients->state = get_block;
                        clients->blocksize = 1028;
                        break;
                    default:
                        temp = NAK;
                        Write(clientfd, &temp, 1);
                    }

                }

                if (clients->state == get_block) {
                    // Keep getting more of the payload till you have the required minimum amount.
                    clients->inbuf += Read(clientfd, (clients->buf) + (clients->inbuf), clients->blocksize - clients->inbuf);
                    if (clients->inbuf == clients->blocksize) {
                        clients->state = check_block;
                    }
                }

                if (clients->state == check_block) {
                    // Fetch block numbers from the packet.
                    temp_block_num = (clients->buf)[0];
                    temp_block_inverse = (clients->buf)[1];

                    // Make sure the block numbers are correct - drop the client if they're not.
                    if ((temp_block_num != (255 - temp_block_inverse) || (temp_block_num > clients->current_block))) {
                        FD_CLR(clientfd, &cset);
                        clients = drop_client(clients);
                        /*
                         * Since drop_client gives us the client after the one we dropped,
                         * we use continue to go back to start of the loop.
                         */
                        break;
                    }

                    // Fetch and assemble the CRC given by client for the payload recieved.
                    temp_crc = 0;
                    temp_crc = (unsigned char)(clients->buf)[(clients->blocksize) - 2];
                    temp_crc <<= 8;
                    temp_crc |= (unsigned char)((clients->buf)[(clients->blocksize) - 1]);

                    // Calculate the actual CRC for the payload recieved.
                    block_crc = crc_message(XMODEM_KEY, (unsigned char *)((clients->buf) + 2), (clients->blocksize) - 4);

                    // Compare the given and calculated CRC to ensure payload integrity.
                    if ((temp_block_num == clients->current_block) && (temp_crc == block_crc)) {

                        printf("File: %s \t Block:%d Successfully recieved\n", clients->filename, clients->current_block);

                        // Write the payload to file if CRCs match
                        if (fwrite((clients->buf) + 2, 1, ((clients->blocksize) - 4), clients->fp) == 0) {
                            perror("Write to File error");
                            exit(-1);
                        }

                        // Send the client an ACK
                        temp = ACK;
                        Write(clientfd, &temp, 1);

                        // Increment service side block number counter and prepare for next block.
                        clients->state = pre_block;
                        clients->current_block += 1;

                        // Deal w/ wrap around when block number hits 255.
                        if (clients->current_block == 256) {
                            clients->current_block = 0;
                        }

                        // Reset the buffer position.
                        clients->inbuf = 0;

                    } else if (temp_crc != block_crc) {
                        printf("File: %s \t Block:%d CRC mismatch.\nExpected:%x, Got %x\n",
                         clients->filename, clients->current_block, temp_crc, block_crc);

                        // Sent the client a NAK if the CRCs didn't match and prepare for repeat block.
                        temp = NAK;
                        Write(clientfd, &temp, 1);
                        clients->state = pre_block;
                        clients->inbuf = 0;

                    } else if (temp_block_num < clients->current_block) {

                        // If we got a repeat block, just send the client an ACK.
                        temp = ACK;
                        Write(clientfd, &temp, 1);

                    }
                }

                if (clients->state == finished) {
                    printf("File Transfer Finished: %s\n", clients->filename);

                    // Cleanup after the client when transfer is finished.
                    // Send the client a final ACK.
                    temp = ACK;
                    Write(clientfd, &temp, 1);

                    // Remove the client and their socket from our list/set of clients.
                    FD_CLR(clientfd, &cset);
                    clients = drop_client(clients);

                }
            }

            // Move on to the next client in the list (if any).
            if (clients) {
                clients = clients->next;
            }

        }
    }

}
