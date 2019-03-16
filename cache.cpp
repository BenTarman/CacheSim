#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>
#include "misc.h"
#include "cache.h"
#include <random>



//parameter to use random replacement or not
extern int doRandomReplacement;

SetCache::SetCache(unsigned int num_lines, unsigned int assoc)
{
   assert(num_lines % assoc == 0);
   // The set bits of the address will be used as an index
   // into sets. Each set is a set containing "assoc" items
   sets.resize(num_lines / assoc);
   lruLists.resize(num_lines / assoc);
   lruMaps.resize(num_lines / assoc);
   for(unsigned int i=0; i < sets.size(); i++) {
      for(unsigned int j=0; j < assoc; ++j) {
         cacheLine temp;
         temp.tag = j;
         temp.state = INV;
         sets[i].insert(temp);
         lruLists[i].push_front(j);
         lruMaps[i].insert(make_pair(j, lruLists[i].begin()));
      }
   }
}

// Given the set and tag, return the cache lines state
cacheState SetCache::findTag(uint64_t set,
                              uint64_t tag) const
{

	// lookup in the map
	if (lruMaps[set].find(tag) != lruMaps[set].end())
	{
		//value found
		return OWN;
	}

	return INV;

}

// Changes the cache line specificed by "set" and "tag" to "state"
void SetCache::changeState(uint64_t set, uint64_t tag,
                              cacheState state)
{
   cacheLine temp;
   temp.tag = tag;
   std::set<cacheLine>::const_iterator it = sets[set].find(temp);

   if(it != sets[set].end()) {
      cacheLine *target;
      target = (cacheLine*)&*it;
      target->state = state;
   }
}

// A complete LRU is mantained for each set, using a separate
// list and map. The front of the list is considered most recently used.
void SetCache::updateLRU(uint64_t set, uint64_t tag)
{
	// this is called on hits
	// 1. use map to quickly it location fast!
	// 2. remove it using loc
	// 3. put it at begning of link list
	// 4. update locaiton in map i think

	// step 1
  //std::list<uint64_t>::iterator loc = lruMaps[set][tag];
	auto it = lruMaps[set].find(tag);

	// step 2
	lruLists[set].erase(it->second);

	// step 3
	lruLists[set].push_front(tag);
	
	//step 4
	it->second = lruLists[set].begin();


}

// Called if a new cache line is to be inserted. Checks if
// the least recently used line needs to be written back to
// main memory.
bool SetCache::checkWriteback(uint64_t set,
                                 uint64_t& tag) const
{
   cacheLine evict, temp;
   tag = lruLists[set].back();
   temp.tag = tag;
   evict = *sets[set].find(temp);

   return (evict.state == MOD || evict.state == OWN);
}

// Insert a new cache line by popping the least recently used line
// and pushing the new line to the back (most recently used)
void SetCache::insertLine(uint64_t set, uint64_t tag,
                           cacheState state)
{
	// 1. insert tag at begining of list
	// 2. evict the last element of list (use tail pointer)  O(1)
	// 3. value evicted is the key of the map and delete that entry in map O(1)
	// 4. insert new key (ie tag) in the map (same as tag at beining of list)
	// 5. make sure new key points to location of the iterator honestly
	// 6. update sets

	cacheLine newRow, temp;
	newRow.tag = tag;
	newRow.state = state;
	if (doRandomReplacement)
	{
		// step 1
		lruLists[set].push_front(tag);

		// step 2
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 eng(rd()); // seed the generator
		std::uniform_int_distribution<> distr(0, lruLists[set].size() -2);
		int rand = distr(eng);


		auto it = lruMaps[set].begin(); 
		std::advance(it, rand); 


		uint64_t evicted_tag = it->first; 
		lruMaps[set].erase(evicted_tag); 
		lruLists[set].erase(it->second); 
		lruMaps[set].insert(make_pair(tag, lruLists[set].begin())); 
		
		temp.tag = evicted_tag;
		
		// update sets
		sets[set].erase(temp);
		sets[set].insert(newRow);

	}
	else
	{
		// step 1
		lruLists[set].push_front(tag);

		// step 2
		uint64_t key = lruLists[set].back();
		lruLists[set].pop_back();

		// step 3
		lruMaps[set].erase(key);

		// step 4 and step 5
		// list.begin() returns iterator to front element (which is what we want)
		lruMaps[set].insert(make_pair(tag, lruLists[set].begin()));

		temp.tag = key;
		
		// step 6 update sets
		sets[set].erase(temp);
		sets[set].insert(newRow);

	}
	
}

