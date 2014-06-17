#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "data.h"

FILE           *logfp = NULL;


/* Read a file, break it into packets. */

/*
 * Notes: getopt is a useful library function to make it easier to read in
 * command line arguments, especially those with options. Read the man page
 * (man 3 getopt) for more information.
 */

int main(int argc, char *argv[])
{
    FILE *infp = stdin;
    FILE *outfp = NULL;
    unsigned char *readBuffer = malloc(MAXSIZE);


    struct packet *currentPacket;
    unsigned int size;
    unsigned int blockNum = 0;
    unsigned short crc;
    unsigned char *payload;

    struct list *head;
    struct list *curr;

    head = NULL;

    char opt;
    extern int optind;
    extern char *optarg;

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            infp = fopen(optarg, "r");
            if (!infp)
            {
                perror("fopen");
                exit(1);
            }
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-f inputfile ] outputfile\n",
                    argv[0]);
            exit(1);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Expected outputfile name\n");
        exit(1);
    }

    if (!(outfp = fopen(argv[optind], "w")))
    {
        perror("fopen");
        exit(1);
    }

    // Try to read bytes <= MAXSIZE into the readBuffer.
    // size stores the number of bytes successfully read by fread.
    size = fread(readBuffer, 1, MAXSIZE, infp);
    // Keep processing fread cannot read any more bytes (EOF).
    while (size > 0)
    {
        /*
         * Since fread doesn't distinguish between EOF and error, use ferror to check for
         * file reading errors.
         */
        if (ferror(infp))
        {
            perror("fread");
            exit(1);
        }
        // Allocate memory to store a single packet.
        currentPacket = malloc(sizeof(struct packet));
        // Allocate memory to store the payload depending on the number of bytes read by fread.
        payload = malloc(size);

        // Copy the bytes successfully read from the buffer to the payload memory.
        memcpy(payload, readBuffer, size);

        // Caclulate the CRC16 of the payload w/ XMODEM_KEY.
        crc = crc_message(XMODEM_KEY, payload, size);

        // Assemble the packet
        currentPacket->block_size = size;
        currentPacket->block_num = blockNum;
        currentPacket->crc = crc;
        currentPacket->payload = payload;

        // Add the packet to the linked list.
        head = addPacket(currentPacket, head, 0);

        // Try to read the next block of bytes <= MAXSIZE into the buffer.
        size = fread(readBuffer, 1, MAXSIZE, infp);
        blockNum += 1;

    }


    // Free the memory allocated to the Buffer.
    free(readBuffer);

    // Iterate through the linked list and write the packets to file.
    curr = head;
    while (curr)
    {

        // Write the packet struct.
        fwrite(&(curr->p), sizeof(struct packet), 1, outfp);

        // Write the payload.
        fwrite(curr->p.payload, curr->p.block_size, 1, outfp);

        // Check for file writing errors.
        if (ferror(outfp))
        {
            perror("fwrite");
            exit(1);
        }
        curr = curr->next;
    }

    // Close file I/O streams.
    fclose(infp);
    fclose(outfp);

    // Free memor used for linked list.
    free_list(head);

    return 0;
}
