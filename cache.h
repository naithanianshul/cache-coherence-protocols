/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
#include <iomanip>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   INVALID = 0,
   VALID = 1,
   DIRTY = 2
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
   char state;    // M, S, I
 
public:
   cacheLine()                { tag = 0; Flags = 0;}
   ulong getTag()             { return tag;}
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) { Flags = flags;}
   void setTag(ulong a)       { tag = a;}
   void invalidate()          { tag = 0; Flags = INVALID; state = 'I';} //useful function
   bool isValid()             { return ((Flags) != INVALID); }

   char getState()            { return state;}
   void setState(char State)  { this->state = State;}
   void initializeMSI()       { state = 'I';}
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   int cache_number = 0;

   //******///
   //add coherence counters here///
   ulong cache_to_cache_transfers, memory_transactions, interventions, invalidations, flushes, BusRd, BusRdX, BusUpgr;
   //******///

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:
   ulong currentCycle;  
   
   Cache(int,int,int,int);
   ~Cache() { delete cache; }
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM()     {return readMisses;}
   ulong getWM()     {return writeMisses;}
   ulong getReads()  {return reads;}
   ulong getWrites() {return writes;}
   ulong getWB()     {return writeBacks;}
   
   void writeBack(ulong) {writeBacks++; memory_transactions++;}
   void Access(ulong,uchar);
   void updateLRU(cacheLine *);
   void printStats(int core_number);

   //******///
   //add other functions to handle bus transactions///
   //******///
   void printCacheState();
   std::string MSIAccess(ulong,uchar,int,std::string);
   std::string MSIBusUpgrAccess(ulong,uchar,int,std::string);
   std::string MESIAccess(ulong,uchar,int,std::string, bool C);
   bool checkCopies(ulong addr);
};

#endif
