#include "dogfault.h"
#include "cache.h"
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "config.h"

// HELPER FUNCTIONS USEFUL FOR IMPLEMENTING THE CACHE

unsigned long long address_to_block(const unsigned long long address, const Cache *cache) {
  return (address >> cache->blockBits) << cache->blockBits;
}

unsigned long long cache_tag(const unsigned long long address, const Cache *cache) {
  return address_to_block(address, cache) >> (cache->setBits + cache->blockBits);
}

unsigned long long cache_set(const unsigned long long address, const Cache *cache) {
  return (address_to_block(address, cache) >> cache->blockBits) & ((1U << cache->setBits) - 1);
}

bool probe_cache(const unsigned long long address, const Cache *cache) {
  for (int i = 0; i < cache->linesPerSet; i++) {
    if((cache->sets[cache_set(address, cache)].lines[i].valid) && (cache->sets[cache_set(address, cache)].lines[i].tag == cache_tag(address, cache)))
    {
      return true;
    }
  }
  return false;
}

void hit_cacheline(const unsigned long long address, Cache *cache) {
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
}

bool insert_cacheline(const unsigned long long address, Cache *cache) {
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

unsigned long long victim_cacheline(const unsigned long long address, const Cache *cache) {
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

void replace_cacheline(const unsigned long long victim_block_addr, const unsigned long long insert_addr, Cache *cache) {
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

void cacheSetUp(Cache *cache, char *name) {
    cache->hit_count = 0;
    cache->hit_count = 0;
    /*YOUR CODE HERE*/
  cache->hit_count = 0;
    /*YOUR CODE HERE*/
  cache->setBits = CACHE_SET_BITS;
  cache->linesPerSet = CACHE_LINES_PER_SET;
  cache->blockBits = CACHE_BLOCK_BITS;
  cache->lfu = CACHE_LFU;
  cache->displayTrace = CACHE_DISPLAY_TRACE;
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

void deallocate(Cache *cache) {
  for (int i = 0; i < (1 << cache->setBits); i++) {
    free(cache->sets[i].lines);
  }
  free(cache->sets);
}

result operateCache(const unsigned long long address, Cache *cache) {
  result r;
  cache->sets[cache_set(address, cache)].lru_clock += 1;

  if (probe_cache(address, cache)) {
    // Hit
    hit_cacheline(address, cache);
    r.status = CACHE_HIT;
    cache->hit_count += 1;
  } 
  else {
    // Miss
    if(insert_cacheline(address, cache)) {
      // Empty cache line found
      r.status = CACHE_MISS;
      r.insert_block_addr = address_to_block(address, cache);
      cache->miss_count += 1;
    } 
    else {
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
  #ifdef PRINT_CACHE_TRACES 
  if (r.status == CACHE_HIT) {
  printf(CACHE_HIT_FORMAT, address); 
  } else if (r.status == CACHE_MISS) {
    printf(CACHE_MISS_FORMAT, address);
  } else if (r.status == CACHE_EVICT) {
    printf(CACHE_EVICTION_FORMAT, address);
  }
  #endif 
  return r;
}

int processCacheOperation(unsigned long address, Cache *cache) {
  result r;
  r = operateCache(address, cache);

  if (r.status == CACHE_HIT) {
    return CACHE_HIT_LATENCY;
  } else if (r.status == CACHE_MISS)
  {
    return CACHE_MISS_LATENCY;
  }
  else
  {
    return CACHE_OTHER_LATENCY;
  }
}
