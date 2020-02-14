## Project 1: Ping-Pong and World Wide Web

### Team:

Shlok Singh Sobti (sss10)
Advait Balaji (ab114)
Tianyang Pan (tp36)


##### Server testing: 
Server handles data fragmentation and waits to recieve a given amount of data - basically deals with errno 11. We briefly ran into errno 88 but we fixed it by increasing the buffer size to the max 65535. To our knowledge the server works as per the specifications of the assignment, though the caveat is that it will continue to listen to the socket till the correct  message is received.

##### Client testing: 
Client only accepts arguments as per the given conditions and ranges as per the problem statement. The client performs the said number of ping pong transactions and then closes the socket. We modified the client.c slightly to create our measurement.c file to help with data independent delay and bandwidth calculations which sends data over a range of sizes (upto the  maximum limit). One change we made was that the acceptable clear servers were not up to date in the assignment so we check for the present clear servers.


##### WWW testing: 
Once we start the server in the www mode we send a GET request via a browser. To test the robustness we send multiple requests at the same time. Further we responded to the GET request with various sizes of HTML files. we implemented some of the error codes such as ERROR 404, ERROR 500, ERROR 400 - although we understand that we don't account for every possible scenario these error implementations are a demonstration of the servers capability to handle these exceptions.