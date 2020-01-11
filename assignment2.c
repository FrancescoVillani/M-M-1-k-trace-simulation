#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_INT 2147483647    /* Maximum positive integer 2^31 - 1 */
#define FILENAME "soccer.dat" /* input data file */

#define START 0.0

double GetArrival(FILE *fp)
{
    double a;
    fscanf(fp, "%lf", &a);
    return (a);
}

int GetSize(FILE *fp)
{
    int s;
    fscanf(fp, "%d\n", &s);
    return (s);
}

void simulation(int queueSize, double internetSpeed, double *packetloss, double *queuedelay, int *incoming, int *delivered, int *lost)
{

    /*used for the queue to keep track of queueing times*/
    typedef struct packet
    {
        double arrival;
        int size;
    } packet;

    /* set debug to 1 to activate debugging, set to 0 to deactivate it */
    int debug = 0;
    /* variables used to see if all the packet arrived have been processed, lost or are in queue, useful to not lose information */
    int count = 0;
    int processed = 0;
    int inqueue = 0;
    int packetLost = 0;

    double averageQueueTime = 0;

    double nextArrival = START;
    /* at the start i don't want to have nextDeparture == nextArrival, or the first packet will go in the queue 
  if it arrives at time 0.000000 */
    double nextDeparture = START;
    int size;

    /*The queue holds the packets, it's a circular array,
  read and write are indexes to remember where the head and the tail are.
  if read == (write + 1) % queueSize then the queue is full
  if read == write then the queue is empty */

    packet queue[queueSize];
    int read = 0;
    int write = 0;

    double startWork = 0;

    internetSpeed = internetSpeed * 1000000;

    FILE *fp;
    fp = fopen(FILENAME, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open input file %s\n", FILENAME);
    }

    while (!feof(fp))
    {
        /*a packet has arrived*/
        count++;

        nextArrival = GetArrival(fp);
        size = GetSize(fp) * 8;

        if (debug)
            printf("---Packet will arrive at: %lf with a size of %db\n", nextArrival, size);

        /*if i have time to work on the queue*/
        if (nextArrival > nextDeparture)
        {
            /* i'll work until the next arrival */
            while (nextDeparture <= nextArrival && write != read)
            {

                if (write == (read + 1) % queueSize)
                {
                    startWork = nextArrival;
                    nextDeparture = nextArrival + (queue[read].size) / internetSpeed;
                }
                else
                {
                    averageQueueTime += nextDeparture - queue[read].arrival;
                    startWork = nextDeparture;
                    nextDeparture = nextDeparture + (queue[read].size) / internetSpeed;
                }
                read = (read + 1) % queueSize;
                processed++;
                inqueue--;

                if (debug)
                    printf("Getting a packet from the queue at time :%lf sized %db, working until: %lf\n", startWork, size, nextDeparture);
            }
        }

        /* then i'll try to add the packet to the queue */
        if (read != (write + 1) % queueSize)
        {
            queue[write].size = size;
            queue[write].arrival = nextArrival;
            write = (write + 1) % queueSize;
            inqueue++;
            if (debug)
                printf("Adding the arrival to the queue, queue size: %d\n", inqueue);
        }
        else
        {
            packetLost += 1;
            if (debug)
                printf("The queue was full :( (%d packets waiting), LOST A PACKET, total number of lost packets :%d\n", inqueue, packetLost);
        }

        /* and then i'll surely work on the arrival */
        while (nextDeparture <= nextArrival && write != read)
        {
            /* i need to be careful of time skips */
            if (write == (read + 1) % queueSize)
            {
                startWork = nextArrival;
                nextDeparture = nextArrival + (queue[read].size) / internetSpeed;
            }
            else
            {
                averageQueueTime += nextDeparture - queue[read].arrival;
                startWork = nextDeparture;
                nextDeparture = nextDeparture + (queue[read].size) / internetSpeed;
            }
            read = (read + 1) % queueSize;
            processed++;
            inqueue--;

            if (debug)
                printf("Getting a packet from the queue at time :%lf sized %db, working until: %lf\n", startWork, size, nextDeparture);
        }
    }

    /* working on the last packets in the queue */
    while (write != read)
    {

        averageQueueTime += nextDeparture - queue[read].arrival;
        nextDeparture = nextDeparture + (queue[read].size) / internetSpeed;
        read = (read + 1) % queueSize;
        processed++;
        inqueue--;

        if (debug)
            printf("Getting a packet from the queue sized %db, working until: %lf\n", size, nextDeparture);
    }

    *packetloss = (double)packetLost / (double)count * 100.0;
    if (processed)
        *queuedelay = averageQueueTime / processed;
    *delivered = processed;
    *incoming = count;
    *lost = packetLost;

    if (debug)
    {
        printf("\npacketloss = %.2f%c\n", ((double)packetLost / (double)count) * 100.0, '%');
        printf("Average time spent in queue: %lf seconds\n", averageQueueTime / processed);
        printf("\nTotal received: %d\nTotal processed: %d\nIn queue: %d\nLost: %d\nProcessed + in queue + lost: %d\n", count, processed, inqueue, packetLost, processed + inqueue + packetLost);
    }

    fclose(fp);
}

int main()
{
    double packetloss;
    double queuedelay;
    int received;
    int delivered;
    int lost;
    printf("\n---TABLE USING A FIXED CONNECTION SPEED (6mbps):\n\n");
    printf("   BUFFER SIZE  |  PACKET LOSS  |    AVG QUEUE DELAY    |   RECEIVED    |   DELIVERED   |  LOST  \n");
    printf("------------------------------------------------------------------------------------------------------\n");
    /* in the way im using the queue, the array needs n+1 to implement a queue that has n size */

    for (int i = 0; i < 31; ++i)
    {
        simulation((i * 10) + 1, 6, &packetloss, &queuedelay, &received, &delivered, &lost);
        printf("\t%d\t|\t%.2f%c\t|\t%lf\t|\t%d\t|\t%d\t|\t%d\t\n", i * 10, packetloss, '%', queuedelay, received, delivered, lost);
    }

    printf("\n---TABLE USING A FIXED BUFFER SIZE (50):\n\n");

    printf("   CONNECTION SPEED     |  PACKET LOSS  |    AVG QUEUE DELAY    |    RECEIVED   |    DELIVERED  |  \tLOST \n");
    printf("---------------------------------------------------------------------------------------------------------------\n");
    simulation(51, 6, &packetloss, &queuedelay, &received, &delivered, &lost);
    printf("\t%s\t\t|\t%.2f%c\t|\t%lf\t|\t%d\t|\t%d\t|\t%d\n", "6mbps", packetloss, '%', queuedelay, received, delivered, lost);
    simulation(51, 11, &packetloss, &queuedelay, &received, &delivered, &lost);
    printf("\t%s\t\t|\t%.2f%c\t|\t%lf\t|\t%d\t|\t%d\t|\t%d\n", "11mbps", packetloss, '%', queuedelay, received, delivered, lost);
    simulation(51, 30, &packetloss, &queuedelay, &received, &delivered, &lost);
    printf("\t%s\t\t|\t%.2f%c\t|\t%lf\t|\t%d\t|\t%d\t|\t%d\n", "30mbps", packetloss, '%', queuedelay, received, delivered, lost);
    simulation(51, 54, &packetloss, &queuedelay, &received, &delivered, &lost);
    printf("\t%s\t\t|\t%.2f%c\t|\t%lf\t|\t%d\t|\t%d\t|\t%d\n", "54mbps", packetloss, '%', queuedelay, received, delivered, lost);
}