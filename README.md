# cache-coherence-protocols
A simulator to compare different Coherence protocol optimizations for parallel architecture designs. This project implements a N-processor system with MSI, MSI+BusUpgr and MESI Bus-based Coherence protocols.

To run the simulator first compile the code by executing the command 'make'.<br/>
The format of the command to run the cache coherence simulator is:

>./smp_cache<br/>
>&emsp;&emsp;<cache_size><br/>
>&emsp;&emsp;<cache_associativity><br/>
>&emsp;&emsp;<block_size><br/>
>&emsp;&emsp;<num_processors><br/>
>&emsp;&emsp;<protocol_id><br/>
>&emsp;&emsp;<trace_file><br/>

Here **<protocol_id>** is:<br/>
0 *for MSI*<br/>
1 *for MSI + BusUpgr*<br/>
2 *for MESI*<br/>
3 *for MESI + Snoop Filter (History Filter)*<br/>

Example - The cache specifications are:
* Cache Size = 8192
* Cache Associativity = 8
* Block Size = 4
* Protocol to be run: MESI = 2
* Addresses to be accessed present in 'canneal.04t.longTrace' file

```./smp_cache 8192 8 64 4 2 trace/canneal.04t.longTrace```

The History Filter in the code is hardcoded as a 16 block and 1 associativity array.
