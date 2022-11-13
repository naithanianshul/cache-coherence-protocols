/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
    
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MSI 1:MSI BusUpgr 2:MESI 3:MESI Snoop FIlter */
    char *fname        = (char *) malloc(20);
    fname              = argv[6];

    //printf("===== Simulator configuration =====\n");
    std::cout << "===== 506 Coherence Simulator Configuration =====" << '\n';
    // print out simulator configuration here
    std::cout << "L1_SIZE: " << cache_size << '\n';
    std::cout << "L1_ASSOC: " << cache_assoc << '\n';
    std::cout << "L1_BLOCKSIZE: " << blk_size << '\n';
    std::cout << "NUMBER OF PROCESSORS: " << num_processors << '\n';
    switch (protocol)
    {
        case 0:
            std::cout << "COHERENCE PROTOCOL: MSI" << '\n';
            break;
        case 1:
            std::cout << "COHERENCE PROTOCOL: MSI BusUpgr" << '\n';
            break;
        case 2:
            std::cout << "COHERENCE PROTOCOL: MESI" << '\n';
            break;
        case 3:
            std::cout << "COHERENCE PROTOCOL: MESI Snoop FIlter" << '\n';
            break;
    }
    std::cout << "TRACE FILE: " << fname << '\n';
    
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
        cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size, i);
    }

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;

    ulong line = 1;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
#ifdef _DEBUG
        //printf("%lu %c %lu\n", proc, op, addr);
#endif
        /*if (line > 15000)
        {
            printf("%lu %lu %c %lu\n", line, proc, op, addr);
            for(int i = 0 ; i < num_processors ; i++)
                cacheArray[i] -> printCacheState();
        }*/
        // propagate request down through memory hierarchy
        // by calling cachesArray[processor#]->Access(...)
        line++;
        std::string bus_signal = "";
        bool C = true;                      // C = true means the block is the only copy among all the cores' cache
        
        if (protocol == 2)
        {
            for(ulong i = 0; i < num_processors; i++)
            {
                if (i != proc)
                {
                    C = cacheArray[i] -> checkCopies(addr);
                    if (C == false)
                    {
                        break;
                    }
                }
            }
        }
        
        switch(protocol)
        {
            case 0:
                bus_signal = cacheArray[proc] -> MSIAccess(addr, op, proc, "");
                break;
            case 1:
                bus_signal = cacheArray[proc] -> MSIBusUpgrAccess(addr, op, proc, "");
                break;
            case 2:
                bus_signal = cacheArray[proc] -> MESIAccess(addr, op, proc, "", C);
                break;
        }

        for(ulong i = 0; i < num_processors; i++)
        {
            if (i != proc)
            {
                switch(protocol)
                {
                    case 0:
                        cacheArray[i] -> MSIAccess(addr, op, proc, bus_signal);
                        break;
                
                    case 1:
                        cacheArray[i] -> MSIBusUpgrAccess(addr, op, proc, bus_signal);
                        break;
                    
                    case 2:
                        cacheArray[i] -> MESIAccess(addr, op, proc, bus_signal, C);
                        break;
                }
            }
        }

        /*if (line > 20000)
            break;*/
    }

    fclose(pFile);

    //********************************//
    //print out all caches' statistics //
    //********************************//
    for(ulong i = 0; i < num_processors; i++)
    {
        cacheArray[i] -> printStats(i);
        //cacheArray[i] -> printCacheState();
    }
}
