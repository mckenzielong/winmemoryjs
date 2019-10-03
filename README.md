# winmemoryjs &middot; [![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mckenzielong/winmemoryjs/blob/master/LICENSE.md) 

winmemoryjs began life as a fork of https://github.com/Rob--/memoryjs. I started this as a learning project to play around with electron and ffxiv memory reading, but after it didn't compile on newer versions of node, I kind of went down a rabbit hole and ended up with this mess. Basically, since memoryjs was written using the v8 structures, winmemoryjs began life as a port to node-addon-api. It quickly turned into its own gross thing. Ideally the feature set of this will cover a similar range of memoryjs, however that project is much more mature.

# Features

- List all processes (sync, async, callback)
- List all modules associated with a process
- List all threads associated with a process
- Open and close a process handle

# Missing features
- Read process memory
- Write process memory
- Read buffers from memory
- Write buffer to memory
- Change memory protection
- Reserve/allocate, commit or change regions of memory
- Fetch a list of memory regions within a process
- Pattern scanning
- Execute a function within a process
- Hardware breakpoints (find out what accesses/writes to this address etc)
