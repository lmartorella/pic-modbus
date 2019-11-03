#ifndef FLOWCOUNTER_H
#define	FLOWCOUNTER_H

// Implement a flow counter using a interrupt counter.
#ifdef HAS_DIGITAL_COUNTER 
#include "../hardware/counter.h"

#define SINK_FLOW_COUNTER "FLOW"

bit flow_write();

#endif

#endif	/* FLOWCOUNTER_H */

