#ifndef SINKS_H_
#define SINKS_H_

/**
 * Sink definition and API module
 */

/**
 * Init sinks
 */
void sinks_init(_Bool isPrimary);

/**
 * Poll sinks
 * Returns true if active and require polling
 */
_Bool sinks_poll();

/**
 * The system sink type ID
 */
#define SINK_SYS_ID "SYS "

/**
 * The system sink read functions
 */
__bit sys_read_prim();
__bit sys_read_sec();

/**
 * The system sink write functions
 */
__bit sys_write_prim();
__bit sys_write_sec();

// Returns 1 if more data should be read or written, 0 if done and the socket can  be freed.
// The 'write' function sends data to master. The 'read' function receives data from master.
typedef __bit (*SinkFunction)();

/**
 * The concatenation of all sink IDS
 */
extern const char* const SINK_IDS;

/**
 * The sink count
 */
extern const int SINK_IDS_COUNT;

/**
 * The sink read handlers
 */
extern const SinkFunction sink_readHandlers[];
/**
 * The sink write handlers
 */
extern const SinkFunction sink_writeHandlers[];

#endif
