#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "cachelab.h"

void solve(int s, int E, int b, int v) {
    long S = 1LL << s;
    long *valid, *tag, *timestamp;
    int  hit_count = 0, miss_count = 0, eviction_count = 0;

    valid     = (long*)malloc(sizeof(long) * (S * E));
    tag       = (long*)malloc(sizeof(long) * (S * E));
    timestamp = (long*)malloc(sizeof(long) * (S * E));

    memset(valid,     0, sizeof(sizeof(long) * (S * E)));
    memset(timestamp, 0, sizeof(sizeof(long) * (S * E)));

    char opt;
    long addr, sz;
    for (long T = 1; ; T++) {
        opt = getchar();
        if (opt == EOF) break;
        if (opt == 'I') {
            scanf("%lx,%ld", &addr, &sz);
            getchar();
            if (v) printf("I %lx,%ld\n", addr, sz);
            continue;
        }

        opt = getchar();
        scanf("%lx,%ld", &addr, &sz);
        while(getchar() != '\n');
        if (v) printf("%c %lx,%ld ", opt, addr, sz);

        long set_id   = (addr >> b) % (1 << s);
        long addr_tag = addr >> (s + b);

        long id = -1;
        for (int i = 0; i < E; i++) {
            if (valid[set_id * E + i] && tag[set_id * E + i] == addr_tag) {
                id = i;
                break;
            }
        }

        // hit
        if (id != -1) {
            hit_count++;
            timestamp[set_id * E + id] = T;
            if (v) printf("hit ");
            if (opt == 'M') {
                hit_count++;
                if (v) printf("hit ");
            }
            if (v) printf("\n");
            continue;
        }

        // miss
        if (v) printf("miss ");
        miss_count++;
        int min_t = 0x7fffffff;
        for (int i = 0; i < E; i++) {
            if (!valid[set_id * E + i]) {
                id = i;
                min_t = 0x7fffffff;
                break;
            } else if (timestamp[set_id * E + i] < min_t) {
                id = i;
                min_t = timestamp[set_id * E + i];
            }
        }

        if (min_t != 0x7fffffff)  {
            eviction_count++;
            if (v) printf("eviction ");
        }
        valid[set_id * E + id] = 1;
        tag[set_id * E + id] = addr_tag;
        timestamp[set_id * E + id] = T;

        if (opt == 'M')  {
            hit_count++;
            if (v) printf("hit ");
        }
        if (v) printf("\n");
    }

    free(tag);
    free(timestamp);
    free(valid);
    printSummary(hit_count, miss_count, eviction_count);
}

void printErrorMessage(char *name) {
    fprintf(stderr,
        "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
        "Options:\n"
        "  -h         Prlong this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n",
        name);
}

int main(int argc, char *argv[]) {
    int v = 0, s = -1, E = -1, b = -1, opt;
    char *trace_file;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'v':
            v = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'h':
        default: /* '?' */
            printErrorMessage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (s == -1 || E == -1 || b == -1 || trace_file == NULL) {
        fprintf(stderr, "%s: Missing required command line argument\n", argv[0]);
        printErrorMessage(argv[0]);
        exit(EXIT_FAILURE);
    }

    freopen(trace_file, "r", stdin);
    solve(s, E, b, v);

    return 0;
}
