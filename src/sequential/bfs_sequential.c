#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

typedef struct {
    int nodes;
    long long edges;

    /*
       offset[i] and offset[i + 1] give the range
       of neighbors belonging to node i.
    */
    int *offset;

    /* Stores all destination nodes */
    int *neighbors;

} Graph;


/* Read the number of nodes from the workload header */
int get_node_count(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open %s\n", filename);
        return -1;
    }

    char line[100];
    int nodes = -1;

    while (fgets(line, sizeof(line), file) != NULL) {

        if (strncmp(line, "# Nodes:", 8) == 0) {

            if (sscanf(line, "# Nodes: %d", &nodes) != 1) {
                nodes = -1;
            }

            break;
        }
    }

    fclose(file);

    if (nodes <= 0) {
        printf("Error: Invalid or missing node count in %s\n",
               filename);
        return -1;
    }

    return nodes;
}


/* Free all memory used by the graph */
void free_graph(Graph *graph)
{
    if (graph == NULL) {
        return;
    }

    free(graph->offset);
    free(graph->neighbors);
    free(graph);
}


/* Build the graph from an edge-list file */
Graph *load_graph(const char *filename)
{
    int nodes = get_node_count(filename);

    if (nodes <= 0) {
        return NULL;
    }

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open %s\n", filename);
        return NULL;
    }

    Graph *graph = malloc(sizeof(Graph));

    if (graph == NULL) {
        printf("Error: Could not allocate memory for graph\n");
        fclose(file);
        return NULL;
    }

    graph->nodes = nodes;
    graph->edges = 0;
    graph->offset = NULL;
    graph->neighbors = NULL;

    /*
       degree[i] stores the number of outgoing
       edges from node i.
    */
    int *degree = calloc(nodes, sizeof(int));

    if (degree == NULL) {
        printf("Error: Could not allocate memory for degree array\n");

        fclose(file);
        free(graph);

        return NULL;
    }

    char line[100];
    int u, v;

    /*
       First pass through the file.

       We count the outgoing edges of every node.
    */
    while (fgets(line, sizeof(line), file) != NULL) {

        /* Ignore header lines */
        if (line[0] == '#') {
            continue;
        }

        /* Ignore malformed lines */
        if (sscanf(line, "%d %d", &u, &v) != 2) {
            continue;
        }

        /* Check that both node IDs are valid */
        if (u < 0 || u >= nodes ||
            v < 0 || v >= nodes) {

            printf("Warning: Ignoring invalid edge %d -> %d\n",
                   u, v);

            continue;
        }

        /*
           Prevent integer overflow in degree[u].
        */
        if (degree[u] == INT_MAX) {
            printf("Error: Too many edges for node %d\n", u);

            free(degree);
            fclose(file);
            free_graph(graph);

            return NULL;
        }

        degree[u]++;
        graph->edges++;
    }

    /*
       offset needs nodes + 1 positions.
    */
    graph->offset =
        malloc((nodes + 1) * sizeof(int));

    if (graph->offset == NULL) {
        printf("Error: Could not allocate memory for offsets\n");

        free(degree);
        fclose(file);
        free_graph(graph);

        return NULL;
    }

    graph->offset[0] = 0;

    /*
       Calculate where each node's neighbors
       will start in the neighbors array.
    */
    for (int i = 0; i < nodes; i++) {

        if (graph->offset[i] > INT_MAX - degree[i]) {
            printf("Error: Graph is too large for this implementation\n");

            free(degree);
            fclose(file);
            free_graph(graph);

            return NULL;
        }

        graph->offset[i + 1] =
            graph->offset[i] + degree[i];
    }

    /*
       The final offset is the total number
       of stored edges.
    */
    if (graph->offset[nodes] != graph->edges) {
        printf("Error: Edge count mismatch while building graph\n");

        free(degree);
        fclose(file);
        free_graph(graph);

        return NULL;
    }

    /*
       Allocate space for all destination nodes.
    */
    if (graph->edges > 0) {

        graph->neighbors =
            malloc(graph->edges * sizeof(int));

        if (graph->neighbors == NULL) {
            printf("Error: Could not allocate memory for edges\n");

            free(degree);
            fclose(file);
            free_graph(graph);

            return NULL;
        }
    }

    /*
       position[i] tells us where the next neighbor
       of node i should be inserted.
    */
    int *position = malloc(nodes * sizeof(int));

    if (position == NULL) {
        printf("Error: Could not allocate memory for positions\n");

        free(degree);
        fclose(file);
        free_graph(graph);

        return NULL;
    }

    for (int i = 0; i < nodes; i++) {
        position[i] = graph->offset[i];
    }

    /*
       Go back to the beginning and read the file again.

       This time we actually store the edges.
    */
    rewind(file);

    while (fgets(line, sizeof(line), file) != NULL) {

        if (line[0] == '#') {
            continue;
        }

        if (sscanf(line, "%d %d", &u, &v) != 2) {
            continue;
        }

        if (u < 0 || u >= nodes ||
            v < 0 || v >= nodes) {
            continue;
        }

        graph->neighbors[position[u]] = v;
        position[u]++;
    }

    free(degree);
    free(position);
    fclose(file);

    return graph;
}


/* Standard sequential BFS */
int bfs(Graph *graph, int source, int *distance)
{
    if (graph == NULL || distance == NULL) {
        printf("Error: Invalid BFS input\n");
        return 0;
    }

    if (source < 0 || source >= graph->nodes) {
        printf("Error: Source node %d is outside the graph\n",
               source);
        return 0;
    }

    /*
       The queue can contain each node at most once,
       because we mark a node visited when we discover it.
    */
    int *queue =
        malloc(graph->nodes * sizeof(int));

    if (queue == NULL) {
        printf("Error: Could not allocate BFS queue\n");
        return 0;
    }

    int front = 0;
    int rear = 0;

    /*
       -1 means the node has not been visited.
    */
    for (int i = 0; i < graph->nodes; i++) {
        distance[i] = -1;
    }

    /* Start BFS from the source */
    distance[source] = 0;
    queue[rear++] = source;

    while (front < rear) {

        int current = queue[front++];

        /*
           Find the neighbors of the current node.
        */
        int start = graph->offset[current];
        int end = graph->offset[current + 1];

        for (int i = start; i < end; i++) {

            int next = graph->neighbors[i];

            /*
               If this node has not been visited,
               record its distance and add it to
               the queue.
            */
            if (distance[next] == -1) {

                distance[next] =
                    distance[current] + 1;

                queue[rear++] = next;
            }
        }
    }

    free(queue);

    return 1;
}


/* Get current time in seconds */
double get_time()
{
    struct timespec time_value;

    if (clock_gettime(CLOCK_MONOTONIC, &time_value) != 0) {
        return -1.0;
    }

    return time_value.tv_sec +
           time_value.tv_nsec / 1000000000.0;
}


int main(int argc, char *argv[])
{
    /*
       The program expects exactly one argument:
       the workload file.
    */
    if (argc != 2) {

        printf("Usage: %s <workload_file>\n", argv[0]);

        return 1;
    }

    const char *filename = argv[1];

    /* We use node 0 as the BFS starting point */
    int source = 0;

    printf("Loading graph...\n");

    Graph *graph = load_graph(filename);

    if (graph == NULL) {
        return 1;
    }

    /*
       Make sure our source node exists.
    */
    if (source >= graph->nodes) {

        printf("Error: Source node %d does not exist\n",
               source);

        free_graph(graph);

        return 1;
    }

    printf("Nodes: %d\n", graph->nodes);
    printf("Edges: %lld\n", graph->edges);
    printf("Source: %d\n", source);

    /*
       Allocate distance array.
    */
    int *distance =
        malloc(graph->nodes * sizeof(int));

    if (distance == NULL) {

        printf("Error: Could not allocate distance array\n");

        free_graph(graph);

        return 1;
    }

    /*
       Start timing ONLY the BFS.

       Graph loading and memory allocation are
       intentionally outside the timer.
    */
    double start = get_time();

    if (start < 0) {

        printf("Error: Could not start timer\n");

        free(distance);
        free_graph(graph);

        return 1;
    }

    int success = bfs(graph, source, distance);

    double end = get_time();

    if (!success || end < 0) {

        printf("Error: BFS or timer failed\n");

        free(distance);
        free_graph(graph);

        return 1;
    }

    /*
       Calculate some useful BFS results.
    */
    int reachable = 0;
    int max_distance = 0;

    for (int i = 0; i < graph->nodes; i++) {

        if (distance[i] != -1) {

            reachable++;

            if (distance[i] > max_distance) {
                max_distance = distance[i];
            }
        }
    }

    printf("\nBFS Results\n");
    printf("Reachable nodes: %d\n", reachable);
    printf("Maximum distance: %d\n", max_distance);
    printf("BFS execution time: %.6f seconds\n",
           end - start);

    /*
       Clean up all allocated memory.
    */
    free(distance);
    free_graph(graph);

    return 0;
}