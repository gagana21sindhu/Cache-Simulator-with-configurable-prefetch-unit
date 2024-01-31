#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <iomanip>
#include <vector>

using namespace std;

typedef struct  {
    int reads;
    int rMiss;
    int writes;
    int wMiss;
    int missRate;
    int writeBack; 
    int memTraffic;
} cacheStats;

typedef struct {
    int validBit;		     
    int dirtyBit;
    unsigned int tag;		 
    unsigned int index;     
} block;

typedef struct {
    vector<unsigned int> pfBlockAddr;
    int streamValidBit;
    int head;
} pfStreamBuffer;	

typedef struct {
    int pfReq;
} pfStats;

class cache {
	public:
	unsigned int blockSize = 0;
	unsigned int cacheSize = 0;
	unsigned int cacheAssoc = 0;  
    unsigned int prefN = 0;
    unsigned int prefM = 0;
		
	int sets = 0;
	int ways = 0;
	unsigned int offsetBits = 0;
	unsigned int indexBits = 0;
	unsigned int tagBits = 0;
	unsigned int address = 0;
	unsigned int index = 0;
	unsigned int tag = 0;
    int prefetchEn = 0;

	//block struct to hold information for each memory block in cache.
	block **memBlocks;   	

	//cacheStats struct to hold run statistics.
	cacheStats stats;

    pfStats pfStat;

	//Array to hold LRU information 
	int **LRU;

    cache *next;

	pfStreamBuffer *streamBuffers; // PF unit
    
    int *pfLRU;

	cache (int a, int b, int c, int d, int e);    //Constructor
	void initCache();
    void initLRU();
	void Request(const char *c, unsigned int address);
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void printCacheContent();	
	void printLRU();

    void initPf();
    void initPfLRU();
    int searchPf(int buff, unsigned int addr); // should return hit flag if the block hit in the buffer 'buff' and set the new head to the index
    void prefetchBuff(int buff, unsigned int addr);
    void updatePfLRU(int buff);
    unsigned int getPfLRU();
    void printPf();
  
};

#endif