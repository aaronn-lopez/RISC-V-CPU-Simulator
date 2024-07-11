#include "cache.h"
#include "dogfault.h"
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// DO NOT MODIFY THIS FILE. INVOKE AFTER EACH ACCESS FROM runTrace
void print_result(result r) {
  if (r.status == CACHE_EVICT)
    printf(" [status: miss eviction, victim_block: 0x%llx, insert_block: 0x%llx]",
           r.victim_block_addr, r.insert_block_addr);
  if (r.status == CACHE_HIT)
    printf(" [status: hit]");
  if (r.status == CACHE_MISS)
    printf(" [status: miss, insert_block: 0x%llx]", r.insert_block_addr);
}

/* This is the entry point to operate the cache for a given address in the trace file.
 * First, is increments the global lru_clock in the corresponding cache set for the address.
 * Second, it checks if the address is already in the cache using the "probe_cache" function.
 * If yes, it is a cache hit:
 *     1) call the "hit_cacheline" function to update the counters inside the hit cache 
 *        line, including its lru_clock and access_counter.
 *     2) record a hit status in the return "result" struct and update hit_count 
 * Otherwise, it is a cache miss:
 *     1) call the "insert_cacheline" function, trying to find an empty cache line in the
 *        cache set and insert the address into the empty line. 
 *     2) if the "insert_cacheline" function returns true, record a miss status and the
          inserted block address in the return "result" struct and update miss_count
 *     3) otherwise, if the "insert_cacheline" function returns false:
 *          a) call the "victim_cacheline" function to figure which victim cache line to 
 *             replace based on the cache replacement policy (LRU and LFU).
 *          b) call the "replace_cacheline" function to replace the victim cache line with
 *             the new cache line to insert.
 *          c) record an eviction status, the victim block address, and the inserted block
 *             address in the return "result" struct. Update miss_count and eviction_count.
 */
result operateCache(const unsigned long long address, Cache *cache) {
  /* YOUR CODE HERE */
  result r;

  cache->sets[cache_set(address, cache)].lru_clock += 1;

  if (probe_cache(address, cache)) {
    // Hit
    hit_cacheline(address, cache);
    r.status = CACHE_HIT;
    cache->hit_count += 1;
  } else {
    // Miss
    if(insert_cacheline(address, cache)) {
      // Empty cache line found
      r.status = CACHE_MISS;
      r.insert_block_addr = address_to_block(address, cache);
      cache->miss_count += 1;
    } else {
      // No empty line found
      unsigned long long victim_block_addr = victim_cacheline(address, cache);
      replace_cacheline(victim_block_addr, address, cache);
      r.status = CACHE_EVICT;
      r.victim_block_addr = victim_block_addr;
      r.insert_block_addr = address_to_block(address, cache);
      cache->miss_count += 1;
      cache->eviction_count += 1;
    }
  }
  return r;
}

// HELPER FUNCTIONS USEFUL FOR IMPLEMENTING THE CACHE
// Given an address, return the block (aligned) address,
// i.e., byte offset bits are cleared to 0
unsigned long long address_to_block(const unsigned long long address,
                                const Cache *cache) {
  /* YOUR CODE HERE */
  return (address >> cache->blockBits) << cache->blockBits;
}

// Return the cache tag of an address
unsigned long long cache_tag(const unsigned long long address,
                             const Cache *cache) {
  /* YOUR CODE HERE */
  return address_to_block(address, cache) >> (cache->setBits + cache->blockBits);
}

// Return the cache set index of the address
unsigned long long cache_set(const unsigned long long address,
                             const Cache *cache) {
  /* YOUR CODE HERE */
  return (address_to_block(address, cache) >> cache->blockBits) & ((1U << cache->setBits) - 1);
}

// Check if the address is found in the cache. If so, return true. else return false.
bool probe_cache(const unsigned long long address, const Cache *cache) {
  /* YOUR CODE HERE */
  //Set *set = &cache->sets[cache_set(address, cache)];
  for (int i = 0; i < cache->linesPerSet; i++) {
    //Line *line = &set->lines[i];
    if((cache->sets[cache_set(address, cache)].lines[i].valid) && (cache->sets[cache_set(address, cache)].lines[i].tag == cache_tag(address, cache))
		    && (cache->sets[cache_set(address, cache)].lines[i].block_addr == address_to_block(address, cache)))
    {
      return true;
    }
  }
  return false;
}

// Access address in cache. Called only if probe is successful.
// Update the LRU (least recently used) or LFU (least frequently used) counters.
void hit_cacheline(const unsigned long long address, Cache *cache){
	Set *set = &cache->sets[cache_set(address, cache)];
	for (int i = 0; i < cache->linesPerSet; i++) {
		 Line *line = &set->lines[i];
		if(line->valid && (line->tag == cache_tag(address, cache))) {
			if (cache->lfu == 0) {
				cache->sets[cache_set(address, cache)].lines[i].lru_clock = 
					++(cache->sets[cache_set(address, cache)].lru_clock);
			} else if (cache->lfu == 1) {
				cache->sets[cache_set(address, cache)].lines[i].access_counter++;
			}
		}
	}

  /* YOUR CODE HERE */
 }

/* This function is only called if probe_cache returns false, i.e., the address is
 * not in the cache. In this function, it will try to find an empty (i.e., invalid)
 * cache line for the address to insert. 
 * If it found an empty one:
 *     1) it inserts the address into that cache line (marking it valid).
 *     2) it updates the cache line's lru_clock based on the global lru_clock 
 *        in the cache set and initiates the cache line's access_counter.
 *     3) it returns true.
 * Otherwise, it returns false.  
 */ 
bool insert_cacheline(const unsigned long long address, Cache *cache) {
  /* YOUR CODE HERE */
  for (int i = 0; i < cache->linesPerSet; i++) {
      Line *line = &cache->sets[cache_set(address, cache)].lines[i];
      // Cache line validity indicated with an non-empty line being true and an empty line being false
      if(line->valid == false)
      {
        line->block_addr = address_to_block(address, cache);
        line->tag = cache_tag(address, cache);
        line->valid = true;
        line->lru_clock = cache->sets[cache_set(address, cache)].lru_clock;
        line->access_counter = 1;
        return true;
      }
    }
   return false;
}

// If there is no empty cacheline, this method figures out which cacheline to replace
// depending on the cache replacement policy (LRU and LFU). It returns the block address
// of the victim cacheline; note we no longer have access to the full address of the victim
unsigned long long victim_cacheline(const unsigned long long address, const Cache *cache) {
  /* YOUR CODE HERE */
  Set *set = &cache->sets[cache_set(address, cache)];
  Line *victim_line = &set->lines[0];
  // Create variable to hold the smallest access_counter
  int smallest = victim_line->access_counter; 
  for (int i = 1; i < cache->linesPerSet; i++) {
    Line *line = &set->lines[i];
    if (cache->lfu == 0) {
      // Find the cacheline that has been used the least recently
      if (line->lru_clock < victim_line->lru_clock) {
        victim_line = line;
      }
    } else if (cache->lfu == 1) {
      // Find the cache line that has been used the least frequently, with a tie-breaker based on the least recently used
      if ((line->access_counter < smallest) || ((line->access_counter ==  smallest) && line->lru_clock < victim_line->lru_clock)) {
        smallest = line->access_counter;
        victim_line = line;
      }
    }
  }
   return victim_line->block_addr;
}

/* Replace the victim cacheline with the new address to insert. Note for the victim cachline,
 * we only have its block address. For the new address to be inserted, we have its full address.
 * Remember to update the new cache line's lru_clock based on the global lru_clock in the cache
 * set and initiate the cache line's access_counter.
 */
void replace_cacheline(const unsigned long long victim_block_addr,
		       const unsigned long long insert_addr, Cache *cache) {
  /* YOUR CODE HERE */
  Set *set = &cache->sets[cache_set(insert_addr, cache)];
  for (int i = 0; i < cache->linesPerSet; i++) {
    Line *line = &set->lines[i];
    if (line->block_addr == victim_block_addr) {
      // Replace the victim cache line with the new address
      line->block_addr = address_to_block(insert_addr, cache);
      line->tag = cache_tag(insert_addr, cache);
      line->lru_clock = set->lru_clock; // Update the lru_clock based on the global lru_clock
      line->access_counter = 1; // Initialize the access_counter
      line->valid = true; // Mark the cache line as valid
      break;
    }
  }
}

// allocate the memory space for the cache with the given cache parameters
// and initialize the cache sets and lines.
// Initialize the cache name to the given name 
void cacheSetUp(Cache *cache, char *name) {
  /* YOUR CODE HERE */
  // Allocate memory for the sets array
  cache->sets = (Set *)malloc((1 << cache->setBits) * sizeof(Set));
  // Allocate and initialize lines for each set
  for (int i = 0; i < (1 << cache->setBits); i++) {
    cache->sets[i].lines = (Line *)malloc(cache->linesPerSet * sizeof(Line));
    // Initialize each line in the set
    for (int j = 0; j < cache->linesPerSet; j++) {
      cache->sets[i].lines[j].valid = false;
      cache->sets[i].lines[j].lru_clock = 0;
      cache->sets[i].lines[j].access_counter = 0;
      cache->sets[i].lines[j].block_addr = 0;
    }
     // Initialize the global lru_clock for the set
    cache->sets[i].lru_clock = 0;
  }
  // Set the cache name
  cache->name = strdup(name);
  // Initialize cache statistics
  cache->hit_count = 0;
  cache->miss_count = 0;
  cache->eviction_count = 0;
}

// deallocate the memory space for the cache
void deallocate(Cache *cache) {
  /* YOUR CODE HERE */
  for (int i = 0; i < (1 << cache->setBits); i++) {
    free(cache->sets[i].lines);
  }
  free(cache->sets);
}

// print out summary stats for the cache
void printSummary(const Cache *cache) {
  printf("%s hits: %d, misses: %d, evictions: %d\n", cache->name, cache->hit_count,
         cache->miss_count, cache->eviction_count);
}
