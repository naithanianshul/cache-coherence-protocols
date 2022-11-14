# cache-coherence-protocols
A simulator to compare different Coherence protocol optimizations for parallel architecture designs. This project implements a N-processor system with MSI, MSI+BusUpgr and MESI Bus-based Coherence protocols.

To run the simulator first compile the code by executing the command 'make'.<br/>
The format of the command to run the cache coherence simulator is:

>./smp_cache<br/>
>&emsp;&emsp;<cache_size><br/>
>&emsp;&emsp;<associativity><br/>
>&emsp;&emsp;<block_size><br/>
>&emsp;&emsp;<num_processors><br/>
>&emsp;&emsp;<protocol><br/>
>&emsp;&emsp;<trace_file><br/>

Here <protocol> is:
0 - MSI
1 - MSI + BusUpgr
2 - MESI
3 - MESI + Snoop Filter (History Filter)
