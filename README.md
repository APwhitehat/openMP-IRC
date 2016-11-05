An IRC made as an personal Project/Assignment 

i don't intend to infringement copyrights 

#CREDITS
Pacheco_IntroductionToParallelProgramming_MorganKaufmann (openMP concepts)
www.tutorialspoint.com/unix_sockets/index.htm (for understanding Sockets & networking )
www.linuxhowtos.org/C_C++/socket.htm (for understanding Sockets & networking)

berenger.eu/blog/    (for multithreading & non-Blocking sockets)



/////////////////////////////////////////////////////////////

This uses NON-Blocking sockets to communicate between server and client 
Uses openMP to handle multiple clients

////////////////////////////////////////////////////////////
////////////		How to use 			////////////////////
////////////////////////////////////////////////////////////

To Setup a client , one need to compile the client.cpp onto their pc .
call the client with hostname of the server and respective port

To setup Server , one needs to compile ompserver.cpp with ompserver.h
but here it gets a little tricky 
your gcc version must support openMP and copilation needs to done with an 
-fopenmp option

