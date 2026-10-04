#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_THREADS 128

typedef struct {
  int src;
  int dest;
} Edge;

typedef struct {
  int *vertices;
  int size;
  int capacity;
} Frontier;

/* ---------------- Frontier Functions ---------------- */

void init_frontier(Frontier *f, int capacity) {
  f->vertices = (int *)malloc(capacity * sizeof(int));
  f->size = 0;
  f->capacity = capacity;
}

void add_to_frontier(Frontier *f, int vertex) {
  if (f->size >= f->capacity) {
    f->capacity *= 2;

    f->vertices = (int *)realloc(f->vertices, f->capacity * sizeof(int));
  }

  f->vertices[f->size++] = vertex;
}

void free_frontier(Frontier *f) {
  free(f->vertices);
  f->vertices = NULL;
  f->size = 0;
  f->capacity = 0;
}

/* ---------------- Load Graph ---------------- */

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

    /* Skip comments */
    if (line[0] == '#') {
      continue;
    }

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

      if (u + 1 > *num_nodes)
        *num_nodes = u + 1;

      if (v + 1 > *num_nodes)
        *num_nodes = v + 1;
    }
  }

  fclose(file);

  return 1;
}

/* ---------------- Build Adjacency List ---------------- */

void build_adjacency(Edge *edges, int num_edges, int num_nodes, int **adj_start,
                     int **adj_edges) {
  *adj_start = (int *)calloc(num_nodes + 1, sizeof(int));

  *adj_edges = (int *)malloc(num_edges * sizeof(int));

  /* Count outgoing edges */
  for (int i = 0; i < num_edges; i++) {
    (*adj_start)[edges[i].src + 1]++;
  }

  /* Prefix sum */
  for (int i = 1; i <= num_nodes; i++) {
    (*adj_start)[i] += (*adj_start)[i - 1];
  }

  int *position = (int *)malloc(num_nodes * sizeof(int));

  memcpy(position, *adj_start, num_nodes * sizeof(int));

  /* Fill adjacency array */
  for (int i = 0; i < num_edges; i++) {

    int u = edges[i].src;
    int v = edges[i].dest;

    (*adj_edges)[position[u]++] = v;
  }

  free(position);
}

/* ---------------- Parallel BFS ---------------- */

int parallel_bfs(int source, int num_nodes, int *adj_start, int *adj_edges,
                 int num_threads) {
  int *visited = (int *)calloc(num_nodes, sizeof(int));

  if (!visited) {
    printf("Memory allocation failed\n");
    return 0;
  }

  Frontier current_frontier;

  init_frontier(&current_frontier, 1024);

  visited[source] = 1;

  add_to_frontier(&current_frontier, source);

  int total_visited = 1;

  while (current_frontier.size > 0) {

    Frontier local_frontiers[MAX_THREADS];

    for (int i = 0; i < num_threads; i++) {
      init_frontier(&local_frontiers[i], 1024);
    }

#pragma omp parallel num_threads(num_threads)
    {
      int tid = omp_get_thread_num();

#pragma omp for schedule(dynamic, 64)
      for (int i = 0; i < current_frontier.size; i++) {

        int u = current_frontier.vertices[i];

        int start = adj_start[u];
        int end = adj_start[u + 1];

        for (int j = start; j < end; j++) {

          int v = adj_edges[j];

          int newly_visited = 0;

/*
 * Protect shared visited array.
 */
#pragma omp critical(visited_check)
          {
            if (!visited[v]) {
              visited[v] = 1;
              newly_visited = 1;
            }
          }

          if (newly_visited) {

            add_to_frontier(&local_frontiers[tid], v);
          }
        }
      }
    }

    Frontier next_frontier;

    init_frontier(&next_frontier, 1024);

    /* Merge thread-local frontiers */

    for (int t = 0; t < num_threads; t++) {

      for (int i = 0; i < local_frontiers[t].size; i++) {

        add_to_frontier(&next_frontier, local_frontiers[t].vertices[i]);

        total_visited++;
      }

      free_frontier(&local_frontiers[t]);
    }

    free_frontier(&current_frontier);

    current_frontier = next_frontier;
  }

  free_frontier(&current_frontier);

  free(visited);

  return total_visited;
}

/* ---------------- Main ---------------- */

int main(int argc, char *argv[]) {

  if (argc != 3) {

    printf("Usage: %s <graph_file> <threads>\n", argv[0]);

    return 1;
  }

  const char *filename = argv[1];

  int num_threads = atoi(argv[2]);

  if (num_threads <= 0 || num_threads > MAX_THREADS) {

    printf("Invalid number of threads.\n");

    return 1;
  }

  Edge *edges = NULL;

  int num_edges = 0;
  int num_nodes = 0;

  printf("Loading graph...\n");

  if (!load_graph(filename, &edges, &num_edges, &num_nodes)) {

    return 1;
  }

  printf("Nodes: %d\n", num_nodes);

  printf("Edges: %d\n", num_edges);

  int *adj_start = NULL;
  int *adj_edges = NULL;

  printf("Building adjacency list...\n");

  build_adjacency(edges, num_edges, num_nodes, &adj_start, &adj_edges);

  free(edges);

  printf("Running OpenMP BFS...\n");

  printf("Threads: %d\n", num_threads);

  double start_time = omp_get_wtime();

  int visited = parallel_bfs(0, num_nodes, adj_start, adj_edges, num_threads);

  double end_time = omp_get_wtime();

  printf("Visited nodes: %d\n", visited);

  printf("Execution time: %.6f seconds\n", end_time - start_time);

  free(adj_start);
  free(adj_edges);

  return 0;
}