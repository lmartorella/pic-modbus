#include <stdio.h>
#include <stdlib.h>
#include "stats.h"

#define MIN_SIZE 16

Stats* stats_new() {
    Stats* stats = (Stats*)malloc(sizeof(Stats));
    stats->durationsTable = malloc(sizeof(size_t) * MIN_SIZE);
    stats->durationsSize = MIN_SIZE;
    stats->durationsCount = 0;
    return stats;
}

void stats_delete(Stats* stats) {
    free(stats);
}

void stats_add(Stats* stats, size_t item) {
    if (stats->durationsCount >= stats->durationsSize) {
        stats->durationsSize = stats->durationsSize * 2;
        stats->durationsTable = realloc(stats->durationsTable, sizeof(size_t) * stats->durationsSize);
    }
    stats->durationsTable[stats->durationsCount++] = item;
}

void stats_clear(Stats* stats) {
    stats->durationsTable = realloc(stats->durationsTable, sizeof(size_t) * MIN_SIZE);
    stats->durationsSize = MIN_SIZE;
    stats->durationsCount = 0;
}
