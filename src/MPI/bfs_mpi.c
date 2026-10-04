#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int src;
  int dest;
} Edge;

/* ---------------- Load Graph (rank 0 only) ---------------- */

int load_graph(const char *filename, Edge **edges, int *num_edges,
               int *num_nodes) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Error opening graph file");
    return 0;
  }

  int capacity = 100000;
  *edges = (Edge *)malloc(capacity * sizeof(Edge));
  if (!*edges) {
    fclose(file);
    return 0;
  }

  *num_edges = 0;
  *num_nodes = 0;
  char line[256];

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#')
      continue;
    int u, v;
    if (sscanf(line, "%d %d", &u, &v) == 2) {
      if (*num_edges >= capacity) {
        capacity *= 2;
        *edges = (Edge *)realloc(*edges, capacity * sizeof(Edge));
        if (!*edges) {
          fclose(file);
          return 0;
        }
      }
      (*edges)[*num_edges].src = u;
      (*edges)[*num_edges].dest = v;
      (*num_edges)++;
      if (u + 1 > *num_nodes) *num_nodes = u + 1;
      if (v + 1 > *num_nodes) *num_nodes = v + 1;
    }
  }
  fclose(file);
  return 1;
}

/* ---------------- Build CSR adjacency (rank 0 only) ---------------- */

void build_adjacency(Edge *edges, int num_edges, int num_nodes, int *adj_start,
                     int *adj_edges) {
  memset(adj_start, 0, (num_nodes + 1) * sizeof(int));

  for (int i = 0; i < num_edges; i++)
    adj_start[edges[i].src + 1]++;

  for (int i = 1; i <= num_nodes; i++)
    adj_start[i] += adj_start[i - 1];

  int *position = (int *)malloc(num_nodes * sizeof(int));
  memcpy(position, adj_start, num_nodes * sizeof(int));

  for (int i = 0; i < num_edges; i++)
    adj_edges[position[edges[i].src]++] = edges[i].dest;

  free(position);
}

/* ---------------- MPI BFS (replicated graph, split frontier) ---------------- */

int mpi_bfs(int source, int n, int *adj_start, int *adj_edges, int rank,
            int size, int *levels_out, double *comm_time_out) {
  char *visited = (char *)calloc(n, sizeof(char));  /* global, identical on all ranks */
  int *local_mark = (int *)calloc(n, sizeof(int));  /* per-level dedup of local finds */

  int *frontier = (int *)malloc(n * sizeof(int));
  int *next = (int *)malloc(n * sizeof(int));
  int *local_new = (int *)malloc(n * sizeof(int));

  int gathered_cap = 1024;
  int *gathered = (int *)malloc(gathered_cap * sizeof(int));

  int *counts = (int *)malloc(size * sizeof(int));
  int *displs = (int *)malloc(size * sizeof(int));

  int fsize = 1;
  frontier[0] = source;
  visited[source] = 1;
  int total_visited = 1;
  int level = 0;
  double comm_time = 0.0;

  while (fsize > 0) {
    level++;
    int local_count = 0;

    /* Each rank expands a cyclic share of the frontier */
    for (int i = rank; i < fsize; i += size) {
      int u = frontier[i];
      for (int j = adj_start[u]; j < adj_start[u + 1]; j++) {
        int v = adj_edges[j];
        if (!visited[v] && local_mark[v] != level) {
          local_mark[v] = level;
          local_new[local_count++] = v;
        }
      }
    }

    /* Exchange what everyone found */
    double t0 = MPI_Wtime();
    MPI_Allgather(&local_count, 1, MPI_INT, counts, 1, MPI_INT, MPI_COMM_WORLD);

    int total = 0;
    for (int r = 0; r < size; r++) {
      displs[r] = total;
      total += counts[r];
    }
    if (total > gathered_cap) {
      gathered_cap = total;
      gathered = (int *)realloc(gathered, gathered_cap * sizeof(int));
    }

    MPI_Allgatherv(local_new, local_count, MPI_INT, gathered, counts, displs,
                   MPI_INT, MPI_COMM_WORLD);
    comm_time += MPI_Wtime() - t0;

    /* Every rank deduplicates in the same order -> identical next frontier */
    int nsize = 0;
    for (int i = 0; i < total; i++) {
      int v = gathered[i];
      if (!visited[v]) {
        visited[v] = 1;
        next[nsize++] = v;
      }
    }
    total_visited += nsize;

    int *tmp = frontier;
    frontier = next;
    next = tmp;
    fsize = nsize;
  }

  *levels_out = level;
  *comm_time_out = comm_time;

  free(visited);
  free(local_mark);
  free(frontier);
  free(next);
  free(local_new);
  free(gathered);
  free(counts);
  free(displs);
  return total_visited;
}

/* ---------------- Main ---------------- */

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 2) {
    if (rank == 0)
      printf("Usage: mpirun -np <procs> %s <graph_file>\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  int dims[2] = {0, 0}; /* num_nodes, num_edges */
  Edge *edges = NULL;

  if (rank == 0) {
    printf("Loading graph...\n");
    if (!load_graph(argv[1], &edges, &dims[1], &dims[0])) {
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    printf("Nodes: %d\n", dims[0]);
    printf("Edges: %d\n", dims[1]);
  }

  MPI_Bcast(dims, 2, MPI_INT, 0, MPI_COMM_WORLD);
  int num_nodes = dims[0];
  int num_edges = dims[1];

  int *adj_start = (int *)malloc((num_nodes + 1) * sizeof(int));
  int *adj_edges = (int *)malloc(num_edges * sizeof(int));

  if (rank == 0) {
    printf("Building adjacency list...\n");
    build_adjacency(edges, num_edges, num_nodes, adj_start, adj_edges);
    free(edges);
  }

  MPI_Bcast(adj_start, num_nodes + 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(adj_edges, num_edges, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    printf("Running MPI BFS...\n");
    printf("Processes: %d\n", size);
  }

  int levels = 0;
  double comm_time = 0.0;

  MPI_Barrier(MPI_COMM_WORLD);
  double t_start = MPI_Wtime();

  int visited = mpi_bfs(0, num_nodes, adj_start, adj_edges, rank, size,
                        &levels, &comm_time);

  MPI_Barrier(MPI_COMM_WORLD);
  double elapsed = MPI_Wtime() - t_start;

  double max_elapsed, max_comm;
  MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&comm_time, &max_comm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    printf("Visited nodes: %d\n", visited);
    printf("BFS levels: %d\n", levels);
    printf("Communication time: %.6f seconds\n", max_comm);
    printf("Execution time: %.6f seconds\n", max_elapsed);
  }

  free(adj_start);
  free(adj_edges);
  MPI_Finalize();
  return 0;
}