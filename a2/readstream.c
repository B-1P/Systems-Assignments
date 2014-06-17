#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "data.h"

FILE           *logfp = NULL;




/* Read a packet file, and coalesce packets */

/*
 * Print to stdout the reconstructed data.  If a packet is missing, insert
 * ### into the output.
 */

int main(int argc, char *argv[])
{
    FILE *infp;
    char opt;

    int readPacket = 0;
    int readPayload = 0;
    int writeTofile = 0;
    int packet_counter = 0;

    unsigned char *payload;
    unsigned short newCRC;

    struct packet *currentPacket = malloc(sizeof(struct packet));
    if (currentPacket == NULL)
    {
        perror("Cannot allocate memory");
        exit(1);
    }

    struct list *head, *curr;
    head = NULL;

    extern int optind;
    extern char *optarg;

    while ((opt = getopt(argc, argv, "l:")) != -1)
    {
        switch (opt)
        {
        case 'l':
            logfp = fopen(optarg, "w");
            writeTofile = 1;
            if (!logfp)
            {
                writeTofile = 0;
                perror("fopen");
                exit(1);
            }
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-l logfile ] inputfile\n",
                    argv[0]);
            exit(1);
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "Expected inputfile name\n");
        exit(1);
    }

    if (argc != 2 && argc != 4)
    {
        fprintf(stderr, "Usage: %s [-l logfile ] inputfile\n", argv[0]);
        exit(1);
    }

    if ((infp = fopen(argv[optind], "r")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    // Get the first packet.
    // readPacket holds the size of the packet fread wasa able to successfully read.
    readPacket = fread(currentPacket, 1, sizeof(struct packet), infp);

    // Keep looping and processing packets while there are more packets.
    while (readPacket == sizeof(struct packet))
    {
        // Checkk for file reading errors.
        if (ferror(infp))
        {
            perror("fread");
            exit(1);
        }
        // Allocate memory for the payload according to the block_size.
        payload = malloc(currentPacket->block_size);
        if (payload == NULL)
        {
            perror("Cannot allocate memory");
            exit(1);
        }
        // Fetch payload bytes.
        readPayload = fread(payload, 1, currentPacket->block_size, infp);
        // Calculate CRC16 w/ XMODEM_KEY for the new payload
        newCRC = crc_message(XMODEM_KEY, payload, readPayload);
        // Assign the payload to its corresspoinding packet struct.
        currentPacket->payload = payload;
        //Check for validity of the new packet's payload by comparing CRC16.
        if (newCRC == currentPacket->crc)
        {
            // If logging is enabled, write "CRC code matches" to logfile.
            if (writeTofile)
            {
                log_message(currentPacket, 1, logfp);
            }
            // Add packet to the list.
            head = addPacket(currentPacket, head, writeTofile);
            //printf("Head block_num:\t%d\n", head->p.block_num);
            free(currentPacket);
        }
        else
        {
            if (writeTofile)
            {
                // If CRC16 check failed and logging is enabled, write "CRC code does not match" to logfile.
                log_message(currentPacket, 2, logfp);
            }
            // Free the memory because the packet was invalid.
            free(currentPacket->payload);
            free(currentPacket);
        }

        /* Check for file writing errors for the log file.
         * Also takes care of errors from addPacket() because the stream is shared/global.
         */
        if (writeTofile)
        {
            if (ferror(logfp))
            {
                perror("fwrite");
                exit(1);
            }
        }


        // Allocate memory for the next packet.
        currentPacket = malloc(sizeof(struct packet));
        if (currentPacket == NULL)
        {
            perror("Cannot allocate memory");
            exit(1);
        }
        // Read the next packet into curr.
        readPacket = fread(currentPacket, 1, sizeof(struct packet), infp);
    }

    // Free memory allocated during the last iteration. EOF - No packet stored into this memory space.
    free(currentPacket);

    curr = head;

    // Iterate throught he linked list, printing the payloads in order.
    while (curr)
    {
        // If a packet is found in the right order, print its payload.
        if (curr->p.block_num == packet_counter)
        {
            printf("%.*s", curr->p.block_size, curr->p.payload);
            //printf("Head block_num:\t%d\n", curr->p.block_num);
            curr = curr->next;
        }
        else
        {
            // If a packet is not found (lost or CRC16 mismatch), print "###" as a placeholder.
            printf("###");
        }
        packet_counter++;

    }

    fclose(infp);
    if (logfp)
    {
        fclose(logfp);
    }

    free_list(head);


    return 0;
}
