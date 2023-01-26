#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

double total_elapsed_time;

// Function to swap two values
void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

// Function to partition the array for quicksort
int partition(int *array, int low, int high) {
  // Choose pivot element
  int pivot = array[high];
  int i = (low - 1);

  // Partition the array
  for (int j = low; j <= high - 1; j++) {
    if (array[j] < pivot) {
      i++;
      swap(&array[i], &array[j]);
    }
  }
  swap(&array[i + 1], &array[high]);
  return (i + 1);
}

// Quicksort function
void quicksort(int *array, int low, int high) {
  if (low < high) {
    int partition_index = partition(array, low, high);

    // Sort elements before and after partition
    quicksort(array, low, partition_index - 1);
    quicksort(array, partition_index + 1, high);
  }
}

int main(int argc, char **argv) {
  clock_t start_time;
  clock_t end_time;

  // Initialize MPI
  MPI_Init(&argc, &argv);

  // Get the number of processes
  int num_procs;
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

  // Get the rank of the process
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Seed the random number generator
  srand(time(NULL));

  // Generate an array of random integers
  const int array_size = 100;
  int chunk_size = array_size / num_procs;
  int *array = malloc(array_size * sizeof(int));

  start_time = clock();

  // Master process: send chunks of the array to other processes
  if (rank == 0) {
    printf("----- Random Array -----\n");
    for (int i = 0; i < array_size; i++) {
      array[i] = rand() % 100;
      // printf("%d  ", array[i]);
      // if((i+1) % 7 == 0){
      //   printf("\n");
      // }
    }

    // Divide the array into chunks
    for (int i = 1; i < num_procs; i++) {
      int start = (i - 1) * chunk_size;
      MPI_Send(array + start, chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    // Sort the chunk assigned to the master process
    quicksort(array, 0, chunk_size - 1);
  } 
  // Worker processes: receive chunks of the array
  else {
    // Receive chunk of the array
    int *chunk = malloc(chunk_size * sizeof(int));
    MPI_Recv(chunk, chunk_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Sort the received chunk
    quicksort(chunk, 0, chunk_size - 1);

    // Send the sorted chunk back to the master process
    MPI_Send(chunk, chunk_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    free(chunk);
  }

  // Master process: receive sorted chunks from worker processes and merge them
  if (rank == 0) {
    // Allocate space for the merged array
    int *merged = malloc(array_size * sizeof(int));

    // Copy the sorted chunk assigned to the master process into the merged array
    for (int i = 0; i < chunk_size; i++) {
      merged[i] = array[i];
    }

    // Receive and merge the sorted chunks from the worker processes
    for (int i = 1; i < num_procs; i++) {
      int start = (i - 1) * chunk_size;

      // Receive sorted chunk
      int *chunk = malloc(chunk_size * sizeof(int));
      MPI_Recv(chunk, chunk_size, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      // Merge the sorted chunk into the merged array
      int m = i * chunk_size;
      int n = 0;
      int p = start;
      while (m < array_size && n < chunk_size) {
        if (merged[m] < chunk[n]) {
          array[p] = merged[m];
          m++;
        } else {
          array[p] = chunk[n];
          n++;
        }
        p++;
      }
      while (m < array_size) {
        array[p] = merged[m];
        m++;
        p++;
      }
      while (n < chunk_size) {
        array[p] = chunk[n];
        n++;
        p++;
      }
      free(chunk);
    }

    free(merged);
  }

  end_time = clock();

  double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  total_elapsed_time += elapsed_time;

  if (rank == num_procs-1) {
    printf("Total elapsed time: %f seconds\n", total_elapsed_time);
  }

  // Print the sorted array
  // printf("\n----- Sorted Array -----\n");
  // for (int i = 0; i < array_size; i++) {
  //   printf("%d  ", array[i]);
  //   if((i+1) % 7 == 0){
  //     printf("\n");
  //   }
  // }

  // Clean up and finalize MPI
  free(array);
  MPI_Finalize();
  return 0;
}