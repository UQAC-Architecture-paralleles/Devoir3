#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BLOCK_LOW(id, p, n) (((id) * (n)) / (p))
#define BLOCK_HIGH(id, p, n) (BLOCK_LOW((id) + 1, p, n) - 1)
#define BLOCK_SIZE(id, p, n) (BLOCK_LOW((id) + 1, p, n) - BLOCK_LOW(id, p, n))
#define BLOCK_OWNER(index, p, n) ((((p) * (index) + 1) - 1) / (n))

// Printing aray
void printarray(int myarray[], int size)
{
    printf("My array is [");
    if (size < 1000)
    {
        int i;
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
    int *marked;

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

    printf("Debug from id %d: low_value=%d\n", id, low_value);
    printf("Debug from id %d: high_value=%d\n", id, high_value);
    printf("Debug from id %d: size=%d\n", id, size);

    proc0_size = (n - 1) / p;
    if ((2 + proc0_size) < (int)sqrt((double)n))
    {
        if (!id)
            printf("Too many processes\n");
        MPI_Finalize();
        exit(1);
    }
    marked = (int *)malloc(size);
    if (marked == NULL)
    {
        printf("Cannot allocate enough memory\n");
        MPI_Finalize();
        exit(1);
    }

    for (i = 0; i < size; i++)
        marked[i] = 0;

    index = 0;
    prime = 2;
    do
    {
        printf("Debug from id %d:---------------------------------\n", id);
        printf("Debug from id %d: prime=%d\n", id, prime);
        printf("Debug from id %d: index=%d\n", id, index);
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
        printf("Debug from id %d: first=%d\n", id, first);
        for (i = first; i < size; i += prime)
        {
            marked[i] = 1;
        }
        printf("Debug from id %d: Array\n", id);
        printarray(marked, size);

        int found_next_prime = 0;
        while (!found_next_prime)
        {
            printf("efr\n");
            if (id == BLOCK_OWNER(index, p, n))
            {
                printf("Debug from id %d: I check value index =%d\n", id, index + 1);
                while (!marked[++index])
                {
                    prime = index + 2;
                }
                printf("Debug from id %d: The next prime is =%d\n", id, prime);
                found_next_prime = 1;
            }
            MPI_Bcast(&found_next_prime, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }
        MPI_Bcast(&index, 1, MPI_INT, 0, MPI_COMM_WORLD);
    } while (prime * prime <= n);

    count = 0;
    for (i = 0; i < size; i++)
        if (!marked[i])
            count++;
    MPI_Reduce(&count, &global_count, 1, MPI_INT, MPI_SUM,
               0, MPI_COMM_WORLD);
    elapsed_time += MPI_Wtime();
    if (!id)
    {
        printf("%d primes are less than or equal to %d\n", global_count, n);
        printarray(marked, size);
    }
    MPI_Finalize();
    return 0;
}