#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "data.h"


char           *messages[] =
{
    "inserted packet",
    "CRC code matches",
    "CRC code does not match",
    "duplicate block"
};

/*
 * Construct an ordered linked list of packets by their block_num.
 * If a packet of a given block_num already exists in the linked
 * list, replace it witht he newer one.
 *
 * If logging is enabled, write events to a log file.
 * newPacket: Packet to be added to the list.
 * head: List to add the packet to.
 * log_enabled: Set to 0 iff logging is to be disabled.
 */

struct list *addPacket(struct packet *newPacket, struct list *head, int log_enabled)
{
    // If there is no head, ie: empty linked list, new packet is at the head.
    if (!head)
    {
        head = malloc(sizeof(struct list));
        if (head == NULL)
        {
            perror("Cannot allocate memory");
            exit(1);
        }
        head->next = NULL;
        head->p = *newPacket;
    }
    else
    {
        // Create a node for the new packet.
        struct list *temp, *prevNode, *nextNode;
        temp = malloc(sizeof(struct list));
        if (temp == NULL)
        {
            perror("Cannot allocate memory");
            exit(1);
        }
        temp->p = *newPacket;
        temp->next = NULL;
        prevNode = NULL;
        nextNode = head;
        // Traverse the linked list till the right place for the new packet is found.
        while (nextNode && (nextNode->p.block_num < newPacket->block_num))
        {
            prevNode = nextNode;
            nextNode = nextNode->next;
        }
        /*
         * If the end of the linked list has been reached, new packet becomes
         * the end of the linked list.
         */
        if (!nextNode)
        {
            prevNode->next = temp;
            if (log_enabled)
            {
                log_message(newPacket, 3, logfp);
            }

        }
        else
        {
            // If theere is already a packet with the same block_num, replace it.
            if ((nextNode->p.block_num) == (newPacket->block_num))
            {
                prevNode->next = temp;
                temp->next = nextNode->next;
                // Don't need to hold the older packet in memory, it can be freed.
                free(nextNode->p.payload);
                free(nextNode);
                if (log_enabled)
                {
                    log_message(newPacket, 3, logfp);
                }
            }
            else
            {
                /*
                 * If we are in the middle of the linked list, place the new packet
                 * in it's appropriate position/
                 */
                if (prevNode)
                {
                    temp->next = prevNode->next;
                    prevNode->next = temp;
                }
                else
                {
                    temp->next = head;
                    head = temp;
                }
                if (log_enabled)
                {
                    log_message(newPacket, 0, logfp);
                }
            }
        }

    }
    return head;
}

/*
 * Free the memory allocated for nodes and payloads in the linked list.
 */
void free_list(struct list *head)
{
    struct list *temp;
    struct packet *pack;
    // Traverse the linked list
    while (head)
    {
        temp = head;
        // Free the memory allocated to the payload.
        free(temp->p.payload);
        // Free the memory allocated to the node itself.
        head = head->next;
        pack = &(temp->p);
        free(temp);
    }
}

/*
 * Write message defined in the call to a logfile filestream.
 */
void log_message(struct packet *p, int message_no, FILE *logfp)
{
    fprintf(logfp, "%hu\t%hu\t%s", p->block_num, p->block_size,
            messages[message_no]);
}
