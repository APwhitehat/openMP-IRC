<center>An IRC made as an personal Project/Assignment</center>

I don't intend to infringe copyrights 

<h3>#CREDITS</h3>
Code starting template
<a href="http://berenger.eu/blog/c-a-tcpip-server-using-openmp-linux-socket/" > A tcp/ip server using OpenMP </a> <br>
Pacheco Introduction To Parallel Programming Morgan Kaufmann (openMP concepts) <br>
www.tutorialspoint.com/unix_sockets/index.htm (for understanding Sockets & networking ) <br>
www.linuxhowtos.org/C_C++/socket.htm (for understanding Sockets & networking) <br>
<a href="berenger.eu/blog/">Berenger Bramas's blog </a>    (for multithreading & non-Blocking sockets) <br>
<a href="http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html" > Beej's guide to networks </a> (for network/socket system calls reference )
<br>
<h2> Introduction </h2>
This uses NON-Blocking sockets to communicate between server and client 
Uses openMP to handle multiple clients
<p>
<h2>How to use</h2> <br>
</p>

To Setup a client , one need to compile the client.cpp onto their pc .
call the client with hostname of the server and respective port

To setup Server , one needs to compile server.cpp with server.h
but here it gets a little tricky 
your gcc version must support openMP and copilation needs to done with an 
<code> -fopenmp </code> option

Refer <a href="https://www.dartmouth.edu/~rc/classes/intro_openmp/compile_run.html" > how to compile openMP </a>

