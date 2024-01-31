#include "cache.h"

using namespace std;


/* Cache Constructor  */
cache::cache (int a, int b, int c, int d, int e){
    blockSize = a;
	cacheSize = b;
	cacheAssoc = c;
    prefN = d;
    prefM = e;
    ways = c;
    next = nullptr;

    offsetBits = ceil(log2 (blockSize));
    if (cacheAssoc < (cacheSize/blockSize)){
		sets = (cacheSize / ( blockSize * cacheAssoc));
        indexBits = ceil(log2 (cacheSize / ( blockSize * cacheAssoc)));
	}
	else if (cacheAssoc >= (cacheSize/blockSize)){
		sets = 1;
        indexBits = 0;
	}
    tagBits = 32 - offsetBits - indexBits;

    // The Cache memory block structure creation
    // sets of Rows , ways of columns
    memBlocks = new block*[sets]; 
    for(int s=0; s<sets; s++)
    {
        memBlocks[s] = new block[ways];
    }

    // Prefetch Structure and LRU storage creation
    if(prefN != 0){
        prefetchEn = 1;
        streamBuffers = new pfStreamBuffer[prefN];
        pfLRU = new int[prefN];

    }
    
    // Initializing the all the stats of the cache 
    stats = { 0, 0, 0, 0, 0, 0, 0}; 
    pfStat = {0};

    // LRU storage creation for all the memory blocks
    LRU = new int*[sets];
    for(int s=0; s<sets; s++)
    {
        LRU[s] = new int[ways];
    }
    

    initCache();
    initLRU();
    if (prefetchEn==1)
    {
        initPf();
        initPfLRU();
    }

} // End : Constructor 



/* Initializing the memory blocks by making all the valid bits as invalid */
void cache::initCache()
{
    for(int i=0;i<sets;i++)
        for(int j=0;j<ways;j++)
        {
            memBlocks[i][j].validBit = 0;
        }
} // End : initCache

/* Initializing the LRU counters with the ways */
// The column represents the LRU counter , The value stored represents the way
void cache::initLRU()
{
    for (int i = 0; i < sets; i++) {
        for (int j = 0; j < ways; j++) {
            LRU[i][j] = ways - 1 - j;
        }
    }
} // End : initLRU

/* Initializing prefecth unit */
void cache::initPf()
{
    for(unsigned int i=0;i<prefN;i++)
    {
        streamBuffers[i].streamValidBit = 0;
        streamBuffers[i].pfBlockAddr.resize(prefM);
        for(unsigned int j=0;j<prefM;j++)
        {
            streamBuffers[i].pfBlockAddr[j]=0;
        }
        streamBuffers[i].head = 0;
    }
} // End : initPf

/* Initializing the LRU storage for prefetch unit */
void cache::initPfLRU()
{
    for(unsigned int i=0;i<prefN;i++)
    {
        pfLRU[i] = prefN - 1 - i;
    }
}

/* Function to print the Cache Contents */
void cache::printCacheContent()
{
	for(int i=0;i<sets;i++)  {
		cout << "set" << setw(7) << right << dec << i << ":";
        cout << setw(9) << right << hex << ( memBlocks[i][LRU[i][0]].tag << (tagBits + indexBits + offsetBits) );
        if (memBlocks[i][LRU[i][0]].dirtyBit == 1 ) cout << setw(2) << right <<"D";
		else cout << setw(2)<< right <<" ";
		for(int j=1; j<ways;j++) {			
			cout << setw(8) << right << hex << ( memBlocks[i][LRU[i][j]].tag << (tagBits + indexBits + offsetBits) );
			if (memBlocks[i][LRU[i][j]].dirtyBit == 1 ) cout << setw(2) << right <<"D";
			else cout << setw(2)<< right <<" ";
		}
	cout << '\n' ;		
	}
} // End : printCacheContent

/* Print Stream Buffers */
void cache::printPf()
{
    unsigned int i = 0,j = 0;
    for(i=0;i<prefN;i++)
    {
        cout << setw(8) << right << hex << streamBuffers[pfLRU[i]].pfBlockAddr[0];
        for(j=1;j<prefM;j++)
        {
            cout << setw(9) << right << hex << streamBuffers[pfLRU[i]].pfBlockAddr[j];
        }
        cout << '\n';    
    } 
}

/* updating LRU counters with the MRU block */
void cache::updateLRU(unsigned int index, int way)
{
	int i = 0, k = 0, temp = 0;
	for(i=0;i<ways;i++)
	{
		if(LRU[index][i] == way) // Find the MRU 
		{
			k = i; 
			break;
		}
	}
	temp = LRU[index][k]; 
	for(i=k;i>0;i--)
	{
		LRU[index][i] = LRU[index][i-1]; // Increment all the LRU counters below the previous LRU Counter of the MRU
	}
	LRU[index][0] = temp;         //Set MRU

} // End : updateLRU


/* To get the LRU Block in a given set */
unsigned int cache::getLRU(unsigned int index)
{
	unsigned int i = 0;
	i = LRU[index][ways-1];
	return i;  // Returns the Block with LRU counter
} // End : getLRU


/* Searching one given buffer in the prefetch unit for a given addr*/
int cache::searchPf(int buff,unsigned int blockAddr)
{
    unsigned int i = 0, hit = 0;
    for (i=0;i<prefM;i++)
    {
    if((streamBuffers[buff].streamValidBit == 1) && (streamBuffers[buff].pfBlockAddr[i] == blockAddr))
    {
        hit = 1;
        streamBuffers[buff].head = i+1;
        break;
    }
    }
    return hit;
}

/* Prefetching the next M blocks when cache and prefetch miss or to maintain the specific prefetch buffer*/
void cache::prefetchBuff(int buff, unsigned int blockAddr)
{
    unsigned int numBlocksFill = 0;
    if (streamBuffers[buff].head == 0)
    {
        for(unsigned int i=0;i<prefM;i++)
        {
            streamBuffers[buff].pfBlockAddr[i]=(blockAddr + i);
            pfStat.pfReq++;
            stats.memTraffic++;
        }
    }
    else if (streamBuffers[buff].head != 0)
    {
        streamBuffers[buff].pfBlockAddr.erase(streamBuffers[buff].pfBlockAddr.begin(),(streamBuffers[buff].pfBlockAddr.begin()+streamBuffers[buff].head));
        int a = streamBuffers[buff].pfBlockAddr.size();
        numBlocksFill = prefM - a;
        for(unsigned int i =0;i<numBlocksFill;i++)
        {
            streamBuffers[buff].pfBlockAddr.push_back((blockAddr + a + i));
            pfStat.pfReq++;
            stats.memTraffic++;
        }
        streamBuffers[buff].head = 0;
    }   
} // End : prefetchBuff

/* updating LRU counters with the MRU buffer */
void cache::updatePfLRU(int buff)
{
    unsigned int i = 0, k = 0, temp = 0;
    for (i=0;i<prefN;i++)
    {
        if(pfLRU[i] == buff){
            k = i;
            break;
        }
    }
    temp = pfLRU[k];
    for(i=k;i>0;i--)
    {
        pfLRU[i] = pfLRU[i-1];
    }
    pfLRU[0] = temp;
} // End : updatePfLRU

/* To get the LRU Buff in the prefetch unit */
unsigned int cache::getPfLRU()
{
    unsigned int i = 0;
    i = pfLRU[prefN-1];
    return i;
} // End : getPfLRU

/* Main Cache Operation Function */
void cache::Request(const char *c, unsigned int address)
{
    // Address Parsing into tag and index
	 			
	unsigned int mask = 4294967295; // 32-bit 
	unsigned int blockAddr = 0; 
    blockAddr = address >> offsetBits;	
    tag = blockAddr >> indexBits; // Tag

    //Index
    mask = mask >> (tagBits + offsetBits);
    if (indexBits!=0)
    {
        index = (address >> offsetBits) & mask;
    }
    else 
    {
        index = 0;
    }

	int hitWay = -1;
    int emptyWay = -1;
	int emptyBlock = -1;
	int hit = 0;
    int pfhit = 0;
    int hitBuff = -1;
    int emptyBuff = -1;
    int emptyBuffFlag = -1;
    unsigned int LRUBuff = 0;
	unsigned int LRUBlock = 0;    

    /// ******************************* Read or Write Request *********************************** ///
    if(*c == 'r'){ 
        stats.reads++; }
    else if (*c =='w'){ 
        stats.writes++; }


    // Searching the cache in the set by the index for a hit 
    for(int i=0;i<ways;i++)
    {
        if( (memBlocks[index][i].validBit == 1) && (memBlocks[index][i].tag == tag) )   
        {	//Request Hit
            hit = 1; 
            hitWay = i;	
            break;   }
        else hit = 0;
    }

    if(prefetchEn==1)
    {
        for (unsigned int i=0;i<prefN;i++)
        {
            pfhit = searchPf(pfLRU[i],blockAddr);
            if(pfhit==1){
                hitBuff = pfLRU[i];
                break;
            }
        }
    }
    

    if(hit==1)
    {   
        if(*c == 'r')
        {
            updateLRU(index,hitWay); // Read Hit update the LRU with the MRU 
        }
        else if (*c =='w')
        {
            // Write Hit , write to the block and make dirtyBit = 1 , upadte LRU
            memBlocks[index][hitWay].tag = tag;
            memBlocks[index][hitWay].validBit = 1;
            memBlocks[index][hitWay].dirtyBit = 1;
            memBlocks[index][hitWay].index = index;
            //Update LRU
            updateLRU(index, hitWay);
        }

        if(prefetchEn==1)
        {
            // Managing prefetch  
            if(pfhit==1){
                prefetchBuff(hitBuff,blockAddr+1);
                streamBuffers[hitBuff].streamValidBit = 1;
                updatePfLRU(hitBuff);
            } 
        }

    } // Cache hit block
    else if (hit==0)
    {
        // ********** Cache Management ************* //
        // Searching for emptyblocks
        for (int i=0;i<ways;i++)
        {
            if( (memBlocks[index][i].validBit == 0) )    
            {	emptyBlock = 1;
                emptyWay = i;
                break;	}
            else emptyBlock = 0;
        }

        // Fetch from next level if empty block available if not start eviction process and then fetch
        if (emptyBlock == 1)
        {
            if(prefetchEn==1)
            {
                if(pfhit==0)
                {
                    if(*c == 'r'){ 
                        stats.rMiss++; }
                    else if (*c =='w'){
                        stats.wMiss++; }
                    if (next != nullptr){
                        next->Request("r",address); // Read Request to nextLevel
                    }
                    else if(next == nullptr)
                    {
                        stats.memTraffic++;
                    }
                    // *** Prefetch Management *** //
                    // Searching for emptybuffers
                    for (unsigned int i=0;i<prefN;i++)
                    {
                        if( (streamBuffers[i].streamValidBit == 0) )    
                        {	emptyBuffFlag = 1;
                            emptyBuff = i;
                            break;	}
                        else emptyBuffFlag = 0;
                    }

                    // Prefetching
                    if(emptyBuffFlag == 1)
                    {
                        prefetchBuff(emptyBuff,blockAddr+1);
                        streamBuffers[emptyBuff].streamValidBit = 1;
                        updatePfLRU(emptyBuff);
                    }
                    else if(emptyBuffFlag == 0)
                    {
                        LRUBuff = getPfLRU();
                        prefetchBuff(LRUBuff,blockAddr+1);
                        streamBuffers[LRUBuff].streamValidBit = 1;
                        updatePfLRU(LRUBuff);
                    }
                }
                else if(pfhit==1)
                {
                    prefetchBuff(hitBuff,blockAddr+1);
                    streamBuffers[hitBuff].streamValidBit = 1;
                    updatePfLRU(hitBuff); 
                }
            }
            else if(prefetchEn==0)
            {
                if(*c == 'r'){ 
                    stats.rMiss++; }
                else if (*c =='w'){
                    stats.wMiss++; }
                if (next != nullptr){
                    next->Request("r",address); // Read Request to nextLevel
                }
                else if(next == nullptr)
                {
                    stats.memTraffic++;
                }
            }

            // Installing the fectched block 
            memBlocks[index][emptyWay].tag = tag;
            memBlocks[index][emptyWay].validBit = 1;
            if(*c == 'r'){ 
                memBlocks[index][emptyWay].dirtyBit = 0;} // Block clean if read
            else if(*c =='w'){
                memBlocks[index][emptyWay].dirtyBit = 1;} // Block dirty if Write
            memBlocks[index][emptyWay].index = index;
            //Update LRU
            updateLRU(index, emptyWay);
        }
        else if (emptyBlock == 0) // cache full , need eviction
        {
            LRUBlock = getLRU(index);
            if(memBlocks[index][LRUBlock].dirtyBit == 1) // Dirty Block needs WB
            {
                stats.writeBack++;
                unsigned int WBAddr; // WB address
                WBAddr = (memBlocks[index][LRUBlock].tag << (offsetBits + indexBits)) + (memBlocks[index][LRUBlock].index << offsetBits);

                if (next != nullptr){
                    next->Request("w",WBAddr); // WB
                }
                else if(next == nullptr)
                {
                    stats.memTraffic++;
                }

                if(prefetchEn==1)
                {
                    if(pfhit==0)
                    {
                        if(*c == 'r'){ 
                            stats.rMiss++; }
                        else if (*c =='w'){
                            stats.wMiss++; }
                        if (next != nullptr){
                            next->Request("r",address); // Read Request to nextLevel
                        }
                        else if(next == nullptr)
                        {
                            stats.memTraffic++;
                        }
                        // *** Prefetch Management *** //
                        // Searching for emptybuffers
                        for (unsigned int i=0;i<prefN;i++)
                        {
                            if( (streamBuffers[i].streamValidBit == 0) )    
                            {	emptyBuffFlag = 1;
                                emptyBuff = i;
                                break;	}
                            else emptyBuffFlag = 0;
                        }

                        // Prefetching
                        if(emptyBuffFlag == 1)
                        {
                            prefetchBuff(emptyBuff,blockAddr+1);
                            streamBuffers[emptyBuff].streamValidBit = 1;
                            updatePfLRU(emptyBuff);
                        }
                        else if(emptyBuffFlag == 0)
                        {
                            LRUBuff = getPfLRU();
                            prefetchBuff(LRUBuff,blockAddr+1);
                            streamBuffers[LRUBuff].streamValidBit = 1;
                            updatePfLRU(LRUBuff);
                        }
                    }
                    else if(pfhit==1)
                    {
                        prefetchBuff(hitBuff,blockAddr+1);
                        streamBuffers[hitBuff].streamValidBit = 1;
                        updatePfLRU(hitBuff);
                    }
                }
                else if(prefetchEn==0)
                {
                    if(*c == 'r'){ 
                        stats.rMiss++; }
                    else if (*c =='w'){
                        stats.wMiss++; }
                    if (next != nullptr){
                        next->Request("r",address); // Read Request to nextLevel
                    }
                    else if(next == nullptr)
                    {
                        stats.memTraffic++;
                    }
                }
                // Installing the fectched block 
                memBlocks[index][LRUBlock].tag = tag;
                memBlocks[index][LRUBlock].validBit = 1;
                if(*c == 'r'){ 
                    memBlocks[index][LRUBlock].dirtyBit = 0;} // Block clean if Read
                else if(*c =='w'){
                    memBlocks[index][LRUBlock].dirtyBit = 1;} // Block dirty if Write
                memBlocks[index][LRUBlock].index = index;
                //Update LRU
                updateLRU(index, LRUBlock);
            }

            else if (memBlocks[index][LRUBlock].dirtyBit == 0) // No WB
            {
                if(prefetchEn==1)
                {
                    if(pfhit==0)
                    {
                        if(*c == 'r'){ 
                            stats.rMiss++; }
                        else if (*c =='w'){
                            stats.wMiss++; }
                        if (next != nullptr){
                            next->Request("r",address); // Read Request to nextLevel
                        }
                        else if(next == nullptr)
                        {
                            stats.memTraffic++;
                        }
                        // *** Prefetch Management *** //
                        // Searching for emptybuffers
                        for (unsigned int i=0;i<prefN;i++)
                        {
                            if( (streamBuffers[i].streamValidBit == 0) )    
                            {	emptyBuffFlag = 1;
                                emptyBuff = i;
                                break;	}
                            else emptyBuffFlag = 0;
                        }

                        // Prefetching
                        if(emptyBuffFlag == 1)
                        {
                            prefetchBuff(emptyBuff,blockAddr+1);
                            streamBuffers[emptyBuff].streamValidBit = 1;
                            updatePfLRU(emptyBuff);
                        }
                        else if(emptyBuffFlag == 0)
                        {
                            LRUBuff = getPfLRU();
                            prefetchBuff(LRUBuff,blockAddr+1);
                            streamBuffers[LRUBuff].streamValidBit = 1;
                            updatePfLRU(LRUBuff);
                        }
                    }
                    else if(pfhit==1)
                    {
                        prefetchBuff(hitBuff,blockAddr+1);
                        streamBuffers[hitBuff].streamValidBit = 1;
                        updatePfLRU(hitBuff);
                    }
                }
                else if(prefetchEn==0)
                {
                    if(*c == 'r'){ 
                        stats.rMiss++; }
                    else if (*c =='w'){
                        stats.wMiss++; }
                    if (next != nullptr){
                        next->Request("r",address); // Read Request to nextLevel
                    }
                    else if(next == nullptr)
                    {
                        stats.memTraffic++;
                    }
                }

                // Installing the fectched block 
                memBlocks[index][LRUBlock].tag = tag;
                memBlocks[index][LRUBlock].validBit = 1;
                if(*c == 'r'){ 
                    memBlocks[index][LRUBlock].dirtyBit = 0;} // Block clean if Read
                else if(*c =='w'){
                    memBlocks[index][LRUBlock].dirtyBit = 1;} // Block dirty if Write
                memBlocks[index][LRUBlock].index = index;
                memBlocks[index][LRUBlock].index = index;
                //Update LRU
                updateLRU(index, LRUBlock);
            }
        }
        //************ Cache Management End *************//

    } // Cache miss block
} // End : Request




