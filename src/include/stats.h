#ifndef _NET_STATS_H
#define _NET_STATS_H

// Table of stats
typedef struct {
    size_t* durationsTable;
    int durationsSize;
    int durationsCount;
} Stats;

Stats* stats_new();
void stats_delete(Stats* stats);
void stats_add(Stats* stats, size_t item);
void stats_clear(Stats* stats);

#endif /* _NET_STATS_H */

