#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BLOCK_LOW(id, p, n) (((id) * (n)) / (p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id) + 1, p, n) - 1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id) + 1, p, n) - BLOCK_LOW(id, p, n))
#define BLOCK_OWNER(index, p, n) ((((p)*index + 1) - 1) / (n))

// Printing aray
void printarray(char myarray[], int size)
{
    printf("My array is [");
    if (size < 1000)
    {
        char i;
        for (i = 0; i < size; i++)
        {
            printf("%d", myarray[i]);
            if (i != size - 1)
            {
                printf(",");
            }
        }
        printf("]\n");
    }
    else
    {
        int i;
        for (i = 0; i < 100; i++)
        {
            printf("%d", myarray[i]);
            if (i != size - 1)
            {
                printf(",");
            }
        }
        printf(" ... ");
        for (i = size - 100; i < size; i++)
        {
            printf("%d", myarray[i]);
            if (i != size - 1)
            {
                printf(",");
            }
        }
        printf("]\n");
    }
}

int main(int argc, char *argv[])
{
    int count;
    int global_count;
    int i;
    int id;
    int p;

    int n;
    int low_value;
    int high_value;
    int size;
    int proc0_size;

    int index;
    int prime;
    int first;

    double elapsed_time;
    char *marked;

    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    elapsed_time = -MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (argc != 2)
    {
        if (!id)
        {
            printf("Command line: %s <m>\n", argv[0]);
        }
        MPI_Finalize();
        exit(1);
    }
    n = atoi(argv[1]);
    low_value = 2 + BLOCK_LOW(id, p, n - 1);
    high_value = 2 + BLOCK_HIGH(id, p, n - 1);
    size = BLOCK_SIZE(id, p, n - 1);
    proc0_size = (n - 1) / p;

    if ((2 + proc0_size) < (int)sqrt((double)n))
    {
        if (!id)
            printf("Too many processes\n");
        MPI_Finalize();
        exit(1);
    }
    marked = (char *)malloc(size * sizeof(char));
    if (marked == NULL)
    {
        printf("Cannot allocate enough memory\n");
        MPI_Finalize();
        exit(1);
    }

    for (i = 0; i < size; i++)
        marked[i] = 0;

    if (!id)
        index = 0;

    prime = 2;
    do
    {

        if (prime * prime > low_value)
        {
            first = prime * prime - low_value;
        }
        else
        {
            if (!(low_value % prime))
            {
                first = 0;
            }
            else
            {
                first = prime - (low_value % prime);
            }
        }

        for (i = first; i < size; i += prime)
        {
            marked[i] = 1;
        }

        if (!id)
        {
            while (marked[++index])
                ;
            prime = index + 2;
        }
        MPI_Bcast(&prime, 1, MPI_INT, 0, MPI_COMM_WORLD);
    } while (prime * prime <= n);

    // First step we compute the local twin number
    count = 0;
    for (i = 0; i + 2 < size; i++)
    {
        if (i + 2 < size)
        {
            if (!marked[i] && !marked[i + 2])
                count++;
        }
    }

    // Second step we send the 2 last value to the next process
    int last_last_index = marked[size - 2];
    int last_index = marked[size - 1];
    if (id != p - 1)
    {
        MPI_Send(&last_last_index, 1, MPI_INT, id + 1, 0, MPI_COMM_WORLD);
        MPI_Send(&last_index, 1, MPI_INT, id + 1, 1, MPI_COMM_WORLD);
    }

    // Third step we receive the 2 last value from the before process
    if (id != 0)
    {
        MPI_Recv(&last_last_index, 1, MPI_INT, id - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&last_index, 1, MPI_INT, id - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (!last_last_index && !marked[0])
            count++;
        if (!last_index && !marked[1])
            count++;
    }

    MPI_Reduce(&count, &global_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    elapsed_time += MPI_Wtime();
    if (!id)
    {
        printf("%d primes twin are less than or equal to %d\n", global_count, n);
    }
    MPI_Finalize();
    return 0;
}