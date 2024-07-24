#ifndef __CONFIG_H__
#define __CONFIG_H__

// For each test, uncomment all its macros, and disable all other macros.
// macros for MS3 will be added later

// required for MS1 (test_simulator_ms1.sh)
//#define DEBUG_CACHE_TRACE	// prints the cache trace at the end of program
// enable `DEBUG_CYCLE` this after completing the code in each stage
//#define DEBUG_CYCLE
//#define MEM_LATENCY 0		// before ms3, we ignore memory access latency

// required for MS2 (test_simulator_ms2.sh)
#define DEBUG_REG_TRACE
#define DEBUG_CYCLE
#define PRINT_STATS		// prints overall stats
#define MEM_LATENCY 0

// required for MS2: vec_xprod.input (test_simulator_ms2_extended.sh)
//#define PRINT_STATS
//#define MEM_LATENCY 0

// required for MS3: (test_simulator_ms3.sh)
//#define DEBUG_REG_TRACE
//#define DEBUG_CYCLE
//#define PRINT_STATS
//#define DEBUG_CACHE_TRACE	// prints the cache trace at the end of program
//#define MEM_LATENCY 10

#endif // __CONFIG_H__
