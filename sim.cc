#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include "cache.h"

using namespace std;

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.



   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   int l1_prefN = 0;
   int l1_prefM = 0;
   int l2_prefN = 0;
   int l2_prefM = 0;
   if(params.L2_SIZE != 0){
      l1_prefN = 0;
      l1_prefM = 0;
      l2_prefN = params.PREF_N;
      l2_prefM = params.PREF_M;
   } 
   else if (params.L2_SIZE == 0){
      l1_prefN = params.PREF_N;
      l1_prefM = params.PREF_M;
      l2_prefN = 0;
      l2_prefM = 0;
   }

   cache L1(params.BLOCKSIZE,params.L1_SIZE,params.L1_ASSOC,l1_prefN,l1_prefM);
   cache L2(params.BLOCKSIZE,params.L2_SIZE,params.L2_ASSOC,l2_prefN,l2_prefM);

   if(params.L2_SIZE != 0)
   {
      L1.next = &L2;
   }

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);

   // Read requests from the trace file 
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	
      L1.Request(&rw,addr);
   }

   cout << '\n' ;
	cout << "===== L1 Contents =====" << endl;
   L1.printCacheContent();
   if(params.L2_SIZE != 0)
   {
      cout << '\n' ;
      cout << "===== L2 Contents =====" << endl;
      L2.printCacheContent();
      if(params.PREF_N != 0)
      {
         cout << '\n' ;
         cout << "===== Stream Buffer(s) contents =====" << endl;
         L2.printPf();
      }
   }
   else if(params.L2_SIZE == 0)
   {
      if(params.PREF_N != 0)
      {
         cout << '\n' ;
         cout << "===== Stream Buffer(s) contents =====" << endl;
         L1.printPf();
      }
   }

   // Printing Measurements 
   int a = L1.stats.reads;
   int b = L1.stats.rMiss;
   int c = L1.stats.writes;
   int d = L1.stats.wMiss;
   double e = double (L1.stats.rMiss + L1.stats.wMiss) / double (L1.stats.reads + L1.stats.writes);
   int f = L1.stats.writeBack;
   int g = L1.pfStat.pfReq;
   int h = L2.stats.reads;
   int i = L2.stats.rMiss;
   int j = 0;
   int k = 0;
   int l = L2.stats.writes;
   int m = L2.stats.wMiss;
   double n = 0;
   if(params.L2_SIZE == 0){
      n = 0;  
   }
   else if(params.L2_SIZE != 0){
      n = double (L2.stats.rMiss) / double (L2.stats.reads);
   }
   int o = L2.stats.writeBack;
   int p = L2.pfStat.pfReq;
   int q = 0;
   if(params.L2_SIZE == 0){
      q =  L1.stats.memTraffic; 
   }
   else if(params.L2_SIZE != 0){
      q =  L2.stats.memTraffic;
   }

   cout << '\n' ;
	cout << "===== Measurements =====" << endl;
   cout << setw(31) << left << "a. L1 reads:"<< left << dec << a << '\n';
   cout << setw(31) << left << "b. L1 read misses:"<< left << dec << b << '\n';
   cout << setw(31) << left << "c. L1 writes:"<< left << dec << c << '\n';
   cout << setw(31) << left << "d. L1 write misses: "<< left << dec << d << '\n';
   cout << setw(31) << left << "e. L1 miss rate:"<< left << fixed << setprecision(4) << e << '\n';
   cout << setw(31) << left << "f. L1 writebacks:"<< left << dec << f << '\n';
   cout << setw(31) << left << "g. L1 prefetches:"<< left << dec << g << '\n';
   cout << setw(31) << left << "h. L2 reads (demand): "<< left << dec << h << '\n';
   cout << setw(31) << left << "i. L2 read misses (demand):"<< left << dec << i << '\n';
   cout << setw(31) << left << "j. L2 reads (prefetch):"<< left << dec << j << '\n';
   cout << setw(31) << left << "k. L2 read misses (prefetch):"<< left << dec << k << '\n';
   cout << setw(31) << left << "l. L2 writes:"<< left << dec << l << '\n';
   cout << setw(31) << left << "m. L2 write misses:"<< left << dec << m << '\n';
   cout << setw(31) << left << "n. L2 miss rate:"<< left << fixed << setprecision(4) << n << '\n';
   cout << setw(31) << left << "o. L2 writebacks:"<< left << dec << o << '\n';
   cout << setw(31) << left << "p. L2 prefetches:"<< left << dec << p << '\n';
   cout << setw(31) << left << "q. memory traffic:"<< left << dec << q << '\n';

   
   return(0);
}
