/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b, int c)
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   this->cache_number = c;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//

   cache_to_cache_transfers = memory_transactions = interventions = invalidations = flushes = BusRdX = BusUpgr = 0;
   useful_snoops = wasted_snoops = filtered_snoops = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
         cache[i][j].initializeMSI();
      }
   }

   this->history_filter_enabled = false;
}

void Cache::setHistoryFilterSize(int rows, int cols)
{
   this->history_filter_enabled = true;
   this->hrows = rows;
   this->hcols = cols;

   // History Filter 1D Array
   history = new cacheLine*[this->hrows];
   for(ulong i=0; i<this->hrows; i++)
   {
      history[i] = new cacheLine[this->hcols];
      for(ulong j=0; j<this->hcols; j++) 
      {
         history[i][j].invalidate();
      }
   }
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op)
{
   currentCycle++;//per cache global counter to maintain LRU order 
                  //among cache ways, updated on every cache access
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)  //miss
   {
      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      if(op == 'w') newline->setFlags(DIRTY);
   }
   else
   {
      //since it's a hit, update LRU and update dirty flag
      updateLRU(line);
      if(op == 'w') line->setFlags(DIRTY);
   }
}

// --------------- //
// Cache Functions //
// --------------- //
/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if( (victim->getFlags() == DIRTY) && (victim->getState() == 'M') )
   {
      writeBack(addr);
   }
   
   tag = calcTag(addr);
   victim->setTag(tag);
   victim->setFlags(VALID);
   victim->setState('I');
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

// -------------- //
// History Filter //
// -------------- //
/*look up line*/
cacheLine * Cache::historyFindLine(ulong addr)
{
   if (this->history_filter_enabled == false)
      return NULL;
   
   ulong i, j, tag, pos;
   
   pos = hcols;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<hcols; j++)
   if(history[i][j].isValid()) {
      if(history[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == hcols) {
      return NULL;
   }
   else {
      return &(history[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::historyUpdateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::historyGetLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = hcols;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<hcols;j++)
   {
      if(history[i][j].isValid() == 0) { 
         return &(history[i][j]); 
      }   
   }

   for(j=0;j<hcols;j++)
   {
      if(history[i][j].getSeq() <= min) { 
         victim = j; 
         min = history[i][j].getSeq();}
   } 

   assert(victim != hcols);
   
   return &(history[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::historyFindLineToReplace(ulong addr)
{
   cacheLine * victim = historyGetLRU(addr);
   historyUpdateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
void Cache::historyFillLine(ulong addr)
{ 
   if (this->history_filter_enabled == true)
   {
      ulong tag;
   
      cacheLine *victim = historyFindLineToReplace(addr);
      assert(victim != 0);
      
      tag = calcTag(addr);
      victim->setTag(tag);
      victim->setFlags(VALID);
      /**note that this cache line has been already 
         upgraded to MRU in the previous function (historyFindLineToReplace)**/
   }
}

// ------------------ //
// Protocol Functions //
// ------------------ //
string Cache::MSIAccess(ulong addr,uchar op, int proc, std::string bus_signal)
{
   if (proc == this->cache_number)                                   // Requester
   {
      currentCycle++;/*per cache global counter to maintain LRU order 
                     among cache ways, updated on every cache access*/
            
      if(op == 'w') writes++;
      else          reads++;
      
      cacheLine * line = findLine(addr);
      if(line == NULL)/*miss*/
      {
         cacheLine *newline = fillLine(addr);
         if(op == 'w')
         {
            writeMisses++;
            if (newline->getState() == 'I')
            {
               this->BusRdX++;
               bus_signal = "BusRdX";
               newline->setState('M');
            }
         }
         else
         {
            readMisses++;
            if (newline->getState() == 'I')
            {
               this->BusRd++;
               bus_signal = "BusRd";
               newline->setState('S');
            }
         }

         if(op == 'w')
            newline->setFlags(DIRTY);
         
         this->memory_transactions++;

         return bus_signal;
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         if(op == 'w')
         {
            line->setFlags(DIRTY);
            if (line->getState() == 'S')
            {
               this->BusRdX++;
               bus_signal = "BusRdX";
               line->setState('M');
               this->memory_transactions++;
            }
         }

         return bus_signal;
      }
   }
   else                                                                    // Snooper
   {
      cacheLine * line = findLine(addr);
      if (line != NULL)    //hit
      {
         if (bus_signal == "BusRdX")
         {
            if (line->getState() == 'S')
            {
               line->invalidate();
               this->invalidations++;
            }
            if (line->getState() == 'M')
            {
               if(line->getFlags() == DIRTY)
               {
                  writeBack(addr);
               }
               line->invalidate();
               this->invalidations++;
               this->flushes++;
            }
         }
         if (bus_signal == "BusRd")
         {
            if (line->getState() == 'M')
            {
               if(line->getFlags() == DIRTY)
               {  
                  writeBack(addr);
               }
               line->setState('S');
               this->interventions++;
               this->flushes++;
            }
         }
      }
      return "";
   }
}

string Cache::MSIBusUpgrAccess(ulong addr,uchar op, int proc, std::string bus_signal)
{
   if (proc == this->cache_number)                                   // Requester
   {
      currentCycle++;/*per cache global counter to maintain LRU order 
                     among cache ways, updated on every cache access*/
            
      if(op == 'w') writes++;
      else          reads++;
      
      cacheLine * line = findLine(addr);
      if(line == NULL)/*miss*/
      {
         cacheLine *newline = fillLine(addr);
         if(op == 'w')
         {
            writeMisses++;
            if (newline->getState() == 'I')
            {
               this->BusRdX++;
               bus_signal = "BusRdX";
               newline->setState('M');
            }
         }
         else
         {
            readMisses++;
            if (newline->getState() == 'I')
            {
               this->BusRd++;
               bus_signal = "BusRd";
               newline->setState('S');
            }
         }

         if(op == 'w')
            newline->setFlags(DIRTY);
         
         this->memory_transactions++;
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         if(op == 'w')
         {
            line->setFlags(DIRTY);
            if (line->getState() == 'S')
            {
               this->BusUpgr++;
               bus_signal = "BusUpgr";
               line->setState('M');
            }
         }
      }
      return bus_signal;
   }
   else                                                                    // Snooper
   {
      cacheLine * line = findLine(addr);
      if (line != NULL)    //hit
      {
         if (bus_signal == "BusRdX")
         {
            if (line->getState() == 'S')
            {
               line->invalidate();
               this->invalidations++;
            }
            if (line->getState() == 'M')
            {
               if(line->getFlags() == DIRTY)
               {
                  writeBack(addr);
               }
               line->invalidate();
               this->invalidations++;
               this->flushes++;
            }
         }
         if (bus_signal == "BusRd")
         {
            if (line->getState() == 'M')
            {
               if(line->getFlags() == DIRTY)
               {
                  writeBack(addr);
               }
               line->setState('S');
               this->interventions++;
               this->flushes++;
            }
         }
         if (bus_signal == "BusUpgr")
         {
            if (line->getState() == 'S')
            {
               line->invalidate();
               this->invalidations++;
            }
         }
      }
      return "";
   }
}

bool Cache::checkCopies(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return true;        // copy does not exist in this cache
   }
   else {
      return false;         // copy exist in this cache
   }
}

string Cache::MESIAccess(ulong addr,uchar op, int proc, std::string bus_signal, bool C)
{
   if (proc == this->cache_number)                                   // Requester
   {
      currentCycle++;/*per cache global counter to maintain LRU order 
                     among cache ways, updated on every cache access*/
            
      if(op == 'w') writes++;
      else          reads++;

      cacheLine *history_line = historyFindLine(addr);
      if (history_line != NULL)
         history_line->invalidate();

      cacheLine * line = findLine(addr);
      if(line == NULL)/*miss*/
      {
         cacheLine *newline = fillLine(addr);
         if(op == 'w')
         {
            writeMisses++;
            if (newline->getState() == 'I')
            {
               this->BusRdX++;
               bus_signal = "BusRdX";
               newline->setState('M');
            }
            
            if (C == true)
               this->memory_transactions++;
            else
               this->cache_to_cache_transfers++;
         }
         else
         {
            readMisses++;
            if (newline->getState() == 'I' && C == true)
            {
               this->BusRd++;
               bus_signal = "BusRd";
               newline->setState('E');
               this->memory_transactions++;
            }
            else if (newline->getState() == 'I' && C == false)
            {
               this->BusRd++;
               bus_signal = "BusRd";
               newline->setState('S');
               this->cache_to_cache_transfers++;
            }
         }

         if(op == 'w')
            newline->setFlags(DIRTY);
         
      }
      else
      {
         /**since it's a hit, update LRU and update dirty flag**/
         updateLRU(line);
         if(op == 'w')
         {
            line->setFlags(DIRTY);
            if (line->getState() == 'S')
            {
               this->BusUpgr++;
               bus_signal = "BusUpgr";
               line->setState('M');
            }
            else if (line->getState() == 'E')
            {
               line->setState('M');
            }
         }
      }
      return bus_signal;
   }
   else                                                                    // Snooper
   {
      cacheLine * history_line = historyFindLine(addr);
      if (history_line != NULL)     // hit
      {
         // Definitely not in Cache
         this->filtered_snoops++;
         historyUpdateLRU(history_line);
      }
      else                          // miss
      {
         // Maybe its in Cache, or maybe not. We have to check.
         cacheLine * line = findLine(addr);
         if (line != NULL)    //hit
         {
            this->useful_snoops++;
            if (bus_signal == "BusRdX")
            {
               if ( (line->getState() == 'S') || (line->getState() == 'E') || (line->getState() == 'M') )
               {
                  if (line->getState() == 'M') // && ( line->getFlags() == DIRTY )
                  {
                     writeBack(addr);
                  }
                  
                  if (line->getState() == 'M')
                     this->flushes++;

                  line->invalidate();
                  this->invalidations++;
                  
                  historyFillLine(addr);
               }
            }
            else if (bus_signal == "BusRd")
            {
               if ( (line->getState() == 'M') || (line->getState() == 'E') )
               {
                  if (line->getFlags() == DIRTY)
                  {
                     writeBack(addr);
                  }

                  if (line->getState() == 'M')
                     this->flushes++;

                  line->setState('S');
                  this->interventions++;
               }
            }
            else if (bus_signal == "BusUpgr")
            {
               if (line->getState() == 'S')
               {
                  line->invalidate();
                  this->invalidations++;

                  historyFillLine(addr);
               }
            }
         }
         else
         {
            this->wasted_snoops++;
            historyFillLine(addr);
         }
      }
      
      return "";
   }
}

// ---------------- //
// Print Statements //
// ---------------- //
void Cache::printCacheState()
{
   for(ulong i=0; i<sets; i++)
   {
      for(ulong j=0; j<assoc; j++)
      {
         std::cout << cache[i][j].getTag() <<" ";
         //std::cout << cache[i][j].getTag() << '(' << cache[i][j].getFlags() << ')' <<'(' << cache[i][j].getState() << ')' <<" ";
      }
      std::cout << '\n';
   }
   std::cout << '\n';
}

void Cache::printHistoryState()
{
   for(ulong i=0; i<16; i++)
   {
      for(ulong j=0; j<1; j++)
      {
         std::cout << history[i][j].getTag() <<" ";
         //std::cout << cache[i][j].getTag() << '(' << cache[i][j].getFlags() << ')' <<'(' << cache[i][j].getState() << ')' <<" ";
      }
      std::cout << '\n';
   }
   std::cout << '\n';
}

void Cache::printStats(int core_number, int protocol)
{
   std::cout << "============ Simulation results (Cache " << core_number << ") ============\n";
   /****print out the rest of statistics here.****/
   std::cout << "01. number of reads: " << getReads() << '\n';
   std::cout << "02. number of read misses: " << getRM() << '\n';
   std::cout << "03. number of writes: " << getWrites() << '\n';
   std::cout << "04. number of write misses: " << getWM() << '\n';
   float miss_rate = (getReads() > 0 || getWrites() > 0) ? ((float)getRM() + (float)getWM()) * 100 /((float)getReads() + (float)getWrites()) : 0.0000;
   std::cout << "05. total miss rate: " << std::fixed << std::setprecision(2) << miss_rate << '%' << '\n';
   std::cout << "06. number of writebacks: " << writeBacks << '\n';
   std::cout << "07. number of cache-to-cache transfers: " << cache_to_cache_transfers << '\n';
   std::cout << "08. number of memory transactions: " << memory_transactions << '\n';
   std::cout << "09. number of interventions: " << interventions << '\n';
   std::cout << "10. number of invalidations: " << invalidations << '\n';
   std::cout << "11. number of flushes: " << flushes << '\n';
   std::cout << "12. number of BusRdX: " << BusRdX << '\n';
   std::cout << "13. number of BusUpgr: " << BusUpgr << '\n';
   /****follow the ouput file format**************/
   if (protocol == 3)
   {
      std::cout << "14. number of useful snoops: " << useful_snoops << '\n';
      std::cout << "15. number of wasted snoops: " << wasted_snoops << '\n';
      std::cout << "16. number of filtered snoops: " << filtered_snoops << '\n';
   }
}
