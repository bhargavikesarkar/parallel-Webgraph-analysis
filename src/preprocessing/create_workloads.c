#include <stdio.h>

#define NUM_WORKLOADS 5

/* 
   Size of each workload.
   These represent the node ID range:
   0 to size-1
*/
int workload_sizes[NUM_WORKLOADS] = {
    50000,
    100000,
    250000,
    500000,
    875713
};

/* Output file for each workload */
char *output_files[NUM_WORKLOADS] = {
    "data/workloads/web-google-50k.txt",
    "data/workloads/web-google-100k.txt",
    "data/workloads/web-google-250k.txt",
    "data/workloads/web-google-500k.txt",
    "data/workloads/web-google-full.txt"
};

int main() {

    FILE *input;
    FILE *output[NUM_WORKLOADS];

    /* Stores the number of edges in each workload */
    long long edge_count[NUM_WORKLOADS] = {0};

    int u, v;

    /*
       Open the original Web-Google dataset
    */
    input = fopen("data/web-Google.txt", "r");

    if (input == NULL) {
        printf("Error: Could not open web-Google.txt\n");
        return 1;
    }

    /*
       Create one output file for each workload
    */
    for (int i = 0; i < NUM_WORKLOADS; i++) {

        output[i] = fopen(output_files[i], "w");

        if (output[i] == NULL) {
            printf("Error: Could not create workload file\n");

            fclose(input);
            return 1;
        }

        /* Add basic information to the beginning of the file */
        fprintf(output[i], "# Web-Google induced subgraph\n");
        fprintf(output[i], "# Nodes: %d\n", workload_sizes[i]);
        fprintf(output[i], "# FromNodeId ToNodeId\n");
    }

    /*
       Read the original dataset one line at a time.

       The original file contains header lines beginning with '#',
       followed by edges such as:

       0 11342
       0 824020
       11342 0
    */
    char line[100];

    while (fgets(line, sizeof(line), input) != NULL) {

        /*
           Skip header/comment lines.
           These lines begin with '#'.
        */
        if (line[0] == '#') {
            continue;
        }

        /*
           Read the two node IDs from the current line.

           u = starting webpage
           v = webpage linked from u
        */
        if (sscanf(line, "%d %d", &u, &v) != 2) {
            continue;
        }

        /*
           Check which workloads contain this edge.

           An edge u -> v belongs to a workload only if
           BOTH u and v are inside that workload.
        */
        for (int i = 0; i < NUM_WORKLOADS; i++) {

            if (u < workload_sizes[i] &&
                v < workload_sizes[i]) {

                /* Save the edge in this workload */
                fprintf(output[i], "%d %d\n", u, v);

                /* Increase edge count */
                edge_count[i]++;
            }
        }
    }

    /*
       Close the original dataset
    */
    fclose(input);

    /*
       Close all workload files
    */
    for (int i = 0; i < NUM_WORKLOADS; i++) {
        fclose(output[i]);
    }

    /*
       Display information about the workloads
    */
    printf("\nWorkloads created successfully!\n\n");

    for (int i = 0; i < NUM_WORKLOADS; i++) {

        printf("Workload: %d nodes\n", workload_sizes[i]);
        printf("Edges: %lld\n", edge_count[i]);
        printf("File: %s\n\n", output_files[i]);
    }

    return 0;
}