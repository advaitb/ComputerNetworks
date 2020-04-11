## Project 3: Intra-Domain RoutingProtocols for Bisco GSR9999

### Team:
- Advait Balaji (ab114)
- Tianyang Pan (tp36)
- Shlok Singh Sobti (sss10)

### Outline:
We provide C++ code that runs the simulator in DV Protocol (poison reverse) and LS protocol (in LS_Protocol.cc and LS_Protocol.h) files. We also implement the ping pong functionality to ascertain distance. All alarms are set and checked for regular updates. We added our own tests to the provided ones and have included the description in the test.txt file. We tested for linkdying, linkcomingup and changedelay.  We also tested for potential memory leaks and observed that our code is memory efficient and uses unordered_map as data structures for tables to provide O(1) lookups thus maintaining runtime efficiency.
