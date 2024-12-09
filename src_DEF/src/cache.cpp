///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// Author: Suryaa Senthilkumar Shanthi

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
    Cache* cache = (Cache*)calloc(1, sizeof(Cache)); // Memory allocation for the structure cache
    cache->replacement_policy = replacement_policy; // Choose the replacement policy
    cache->number_of_ways = associativity; 
    cache->number_of_sets = size / (associativity * line_size); // Cache size / (number of ways * size of 1 line)
    cache->cacheset = (CacheSet*)calloc(cache->number_of_sets, sizeof(CacheSet)); // Allocating memory to all sets
    return cache;
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
    // TODO: If is_write is true, mark the resident line as dirty.
    // TODO: Update the appropriate cache statistics.
    unsigned int check_index = line_addr % (c->number_of_sets-1);
    unsigned int ctag = line_addr / c->number_of_sets;
    // Check if read access or write access and update accordingly
    switch (is_write)
    {
    case 0:
        c->stat_read_access++;
        break;
    case 1:
        c->stat_write_access++;
        break;
    }

    for (unsigned int m = 0; m < c->number_of_ways; m++)
    {
        // Check if cacheline is valid
        if (c->cacheset[check_index].cacheline[m].is_valid == true) 
        {  
            // Check if tag is updated
           if (c->cacheset[check_index].cacheline[m].tag == ctag)
           {
                if (is_write == 1)
                {
                    c->cacheset[check_index].cacheline[m].is_dirty = true;
                }
                return HIT;
           } 
        }
    }
    if (is_write)
    {
        c->stat_write_miss++;
    }
    else
    {
        c->stat_read_miss++;
    }
    return MISS;
}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
void cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.
    // TODO: Initialize the victim entry with the line to install.
    // TODO: Update the appropriate cache statistics.
    unsigned int check_index = line_addr % (c->number_of_sets-1);
    unsigned int ctag = line_addr/c->number_of_sets;
    //Finding victim using the cache_find_victim function. 
    unsigned int vic_index = cache_find_victim(c, check_index, core_id);
    c->last_evicted_line = c->cacheset[check_index].cacheline[vic_index];
    //Initialising evicted entry
    if (c->last_evicted_line.is_dirty == true)
    {  
        if (is_write)
        {
            c->stat_dirty_evicts++;
        }
    }
    // Updating cache statistics
    c->cacheset[check_index].cacheline[vic_index].is_valid = true;
    c->cacheset[check_index].cacheline[vic_index].is_dirty = is_write;
    c->cacheset[check_index].cacheline[vic_index].core_id = core_id;
    c->cacheset[check_index].cacheline[vic_index].tag = ctag;
    c->cacheset[check_index].cacheline[vic_index].previous_time_access = current_cycle;
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.
    // TODO: In part A, implement the LRU and random replacement policies.
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.
    unsigned int vic_index = 0;
    unsigned int lru = c->number_of_ways;
    unsigned int old_time = -1;
    if (c->replacement_policy == 0)
    {
        for (unsigned int l = 0; l < c->number_of_ways; l++)
        {
            if (!c->cacheset[set_index].cacheline[l].is_valid)
            {
                return l;
            }
            // Checking if the previous access time is less than the oldest time
            if (c->cacheset[set_index].cacheline[l].previous_time_access < old_time)
            {
                // If true, update the oldest time with the previous access time
                old_time = c->cacheset[set_index].cacheline[l].previous_time_access;
                lru = l;
            }
        }
        if (lru != c->number_of_ways)
        {
            vic_index = lru;
        }
    }
    else if (c->replacement_policy == 1)
    {
        vic_index = rand() % c->number_of_ways;
    }
    else if (c->replacement_policy == 2)
    {
        unsigned int m;
        if (core_id == 0)
        {
            m = 0;
            while (m < SWP_CORE0_WAYS)
            {
                if (c->cacheset[set_index].cacheline[m].previous_time_access < old_time)
                {
                    // If true update the oldest time with the previous access time
                    old_time = c->cacheset[set_index].cacheline[m].previous_time_access;
                    lru = m;
                }
                m++;
            }
        }
        else
        {
            m = SWP_CORE0_WAYS;
            while (m < c->number_of_ways)
            {
                if (c->cacheset[set_index].cacheline[m].previous_time_access < old_time)
                {
                    old_time = c->cacheset[set_index].cacheline[m].previous_time_access;
                    lru = m;
                }
                m++;
            }
        }
        if (lru != c->number_of_ways)
        {
            vic_index = lru;
        }
    }
    return vic_index;
}

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}
