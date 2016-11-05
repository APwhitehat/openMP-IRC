#ifndef OMPSERVER_H
#define OMPSERVER_H
 
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>             //for ::close()
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
 
#include <omp.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
 
class OmpServer
{
private:
    ///////////////////////////////////////////////
    // Static data
    ///////////////////////////////////////////////
 
    const static int MaximumClients     = 50;   //< Maximum nb clients also == Nb threads in Pool
    const static int NbQueuedClients    = 4;    //< Queued clients in the listen function
    const static int BufferSize         = 256;  //< Message buffer size
 
    // Generic method to test network C return value
    // provides error handling
    static bool IsValidReturn(const int inReturnedValue){
        return inReturnedValue >= 0;
    }
 
    ///////////////////////////////////////////////
    // Attributes
    ///////////////////////////////////////////////
 
    sockaddr_in         address;    //< Internet address
    int                 sock;       //< Server socket
    std::vector< std::pair<int,std::string> >    clients;    //< Clients sockets
    int                 nbClients;  //< Current Nb Clients
    bool                hasToStop;  //< To stop the server

    struct directive                   //data recieved after parsing the message
    {
        int cmd;                        //command id
        std::string buffer;             //1:msg 2:all
        std::string dest,nick;
    };
 
    ///////////////////////////////////////////////
    // Usual socket stuff
    ///////////////////////////////////////////////
 
    /** Bind socket to internet address */
    bool bind(const int inPort){
        address.sin_family       = AF_INET;
        address.sin_addr.s_addr  = INADDR_ANY;
        address.sin_port         = htons ( inPort );
 
        return IsValidReturn( ::bind( sock, (struct sockaddr*) &address, sizeof(address)) );
    }
 
    /** The server starts to listen to the socket */
    bool listen(){
        return IsValidReturn( ::listen( sock, NbQueuedClients ) );
    }
 
    /** Accept a new client, this function is non-blocking */
    int accept(){
        int sizeOfAddress = sizeof(address);
        // Because of the nonblocking mode, if accept returns a
        // negative number we cannot know if there is an error OR no new client
        const int newClientSock = ::accept ( sock, (sockaddr*)&address, (socklen_t*)&sizeOfAddress );
        return (newClientSock < 0 ? -1 : newClientSock);
    }
 
    /** Process a client (manage communication) */
    void processClient(const int inClientSocket){
        
        // adds the new client until server becomes full
        if( !addClient(inClientSocket) ){
            IsValidReturn( write( inClientSocket, "Server is full!\n", 16) );
            ::close(inClientSocket);
            return;
        }
 
        // Try to send a message:: ask for nick
        if( !IsValidReturn( write( inClientSocket, "Welcome : enter your nick :\n", 28)) ){
            removeClient(inClientSocket);
            return;
        }
 
        // Wait message and broadcast
        const size_t BufferSize = 256;
        char recvBuffer[BufferSize];
 
        const int flag = fcntl(inClientSocket, F_GETFL, 0);
        fcntl(inClientSocket, F_SETFL, flag | O_NONBLOCK);
        
        bool FirstTime=true;   // to accept nickname

        while( true ){
            fd_set  readSet;
            FD_ZERO(&readSet);
 
            FD_SET(inClientSocket, &readSet);
 
            struct timeval timeout = {1, 0};
            select( inClientSocket + 1, &readSet, NULL, NULL, &timeout);
 
            // We have to perform the test after the select (may have spent 1s in the select)
            if( hasToStop ){
                break;
            }

            memset(&recvBuffer, 0, sizeof(recvBuffer));
            
            if( FD_ISSET(inClientSocket, &readSet) ){
                FD_CLR(inClientSocket, &readSet);
                const int lenghtRead = read(inClientSocket, recvBuffer, BufferSize);
                if(lenghtRead <= 0){
                    removeClient(inClientSocket);
                    break;
                }
                else if(0 < lenghtRead){
                    if(FirstTime==true)
                    {
                        int userid=SearchClient(inClientSocket);
                        recvBuffer[strlen(recvBuffer)-1]='\0';
                        if(userid>=0)
                            {
                                clients[userid].second=recvBuffer;
                            }

                        //send saved messages if exists 
                        sendsaved(userid);

                        show(userid,recvBuffer);            //debugging
                        show();
                        FirstTime=false;
                    }
                    else
                    {
                        directive temp;
                        std::string x=recvBuffer;
                        temp.nick=clients[SearchClient(inClientSocket)].second;
                        if(parse(x,temp))
                        {
                            show(temp);           //debugging
                            show();
                            broadcast(temp);
                        }
                    }
                    
                }
            }
        }
    }
 
    ///////////////////////////////////////////////
    // Clients socket management
    ///////////////////////////////////////////////
 
    /** Remove a client from the vector (CriticalClient)*/
    void removeClient(const int inClientSocket){
        #pragma omp critical(CriticalClient)
        {
            int socketPosition = -1;
            for(int idxClient = 0 ; idxClient < nbClients ; ++idxClient){
                if( clients[idxClient].first == inClientSocket ){
                    socketPosition = idxClient;
                    break;
                }
            }
            if( socketPosition != -1 ){
                ::close(inClientSocket);
                --nbClients;
                // we switch only the last client and the client to removed
                // because the vector is not ordered
                // if nbClient was 1 => clients[0] = clients[0]
                // if socketPosition is nbClient - 1 => clients[socketPosition] = clients[socketPosition]
                clients[socketPosition] = clients[nbClients];
            }
        }
    }
 
    /** Add a client in the vector if possible (CriticalClient)*/
    bool addClient(const int inClientSocket){
        bool cliendAdded = false;
        #pragma omp critical(CriticalClient)
        {
            if(nbClients != MaximumClients){
                clients[nbClients++].first = inClientSocket;
                cliendAdded = true;
            }
        }
        return cliendAdded;
    }
 
    ////sends messages to client ////
    ////or broadcasts them to everyone////
    void broadcast(const directive& data){
        if(data.cmd==2)
        {
            #pragma omp critical(CriticalClient)
            {
                for(int idxClient = 0 ; idxClient < nbClients ; ++idxClient){

                        std::string tempBuffer="msg from " +data.nick+ ": "+data.buffer;
                        // write to client , if it fails 
                        // remove the client
                        if( write(clients[idxClient].first, tempBuffer.c_str(), std::strlen(tempBuffer.c_str()) ) <= 0 ){
                            ::close(clients[idxClient].first);
                            --nbClients;
                            clients[idxClient] = clients[nbClients];
                            --idxClient;
                        }
                }
            }
        }
        else if(data.cmd==1)
        {
            for(int idxClient = 0 ; idxClient < nbClients ; ++idxClient){
                bool found=false;
                    if(clients[idxClient].second.compare(data.dest)==0)
                    {
                        std::string tempBuffer="msg from " +data.nick+ ": " +data.buffer;
                        // write to client , if it fails 
                        // remove the client
                        if( write(clients[idxClient].first, tempBuffer.c_str(), std::strlen(tempBuffer.c_str()) ) <= 0 ){
                            ::close(clients[idxClient].first);
                            --nbClients;
                            clients[idxClient] = clients[nbClients];
                            --idxClient;
                        }
                        found =true;
                    }
                //if client not online save the msg
                if(found==false)
                    save(data.dest,"msg from " +data.nick+ ": " +data.buffer);
            }
        }
    }
 
    /** Forbid copy */
    OmpServer(const OmpServer&){}
    OmpServer& operator=(const OmpServer&){ return *this; }

    //search clients
    int SearchClient(int SockId)
    {
        for(int i=0;i<nbClients;i++)
            if(clients[i].first==SockId)
                return i;
        return -1;
    }

///////////////////////////////////////////////
//parsing function ////////////////////////////
////////This is where the magic happens ///////    
///////////////////////////////////////////////

bool parse(std::string& buffer,directive &temp)
    {
        if(buffer.compare(0,4,"/msg")==0)
        {
            temp.cmd=1;
            buffer=buffer.substr(5);

            temp.dest=buffer.substr(0,buffer.find(" "));
            buffer=buffer.substr(buffer.find(" ")+1);

            temp.buffer=buffer;

            return true;
        }
        else if(buffer.compare(0,4,"/all")==0)
        {
            temp.cmd=2;
            temp.buffer=buffer.substr(5);
            return true;
        }
        else
            return false;
    }

////save messages for offline users/////
void save(const std::string name,const std::string msg)
{
    std::string newname=name+".dat";
    std::ofstream fout( newname.c_str() , std::ios::out|std::ios::app);
    fout<<msg.c_str()<<"\n";
    fout.close();
}
// send out saved messages if present
void sendsaved(const int userid)
{
    std::string newname=clients[userid].second+".dat";
    const size_t BufferSize = 256;
    if(check(newname))
    {
        std::ifstream fin(newname.c_str());
        char buffer[BufferSize];
        fin.read( buffer, BufferSize);
        int lenghtRead= strlen( buffer );
        while(!fin.eof())
        {
            if( write(clients[userid].first, buffer, lenghtRead) <= 0 ){
                    ::close(clients[userid].first);
                    --nbClients;
                    clients[userid] = clients[nbClients];
                }

            fin.read( buffer, BufferSize);
            int lenghtRead= strlen( buffer );
        }

        // Remove the file after reading it :)
        if(std::remove(newname.c_str()) != 0)
            std::cerr<<"error deleting file :"<<newname<<std::endl;
    }
}

// simple function to check if a file exists or not ////
//          uses unix system call stat              ////
//                  supposedly fast                 ////
//at least faster than opening and closing the file ////

inline bool check(const std::string name) {
  struct stat buffer;   
  return ( stat(name.c_str(), &buffer) == 0 ); 
}

    /////////////////////////////
    /////debugging functions/////
    /////////////////////////////
void show(const directive t)
{
    std::cout<<"cmd :"<<t.cmd<<"\n";
    std::cout<<"dest :|"<<t.dest<<"|\n";
    std::cout<<"buffer :"<<t.buffer<<"\n";
}
void show(const int userid,const char recvBuffer[])
{
    std::cout<<"clients nick :"<<clients[userid].second<<"\n";
    std::cout<<"recvBuffer :|"<<recvBuffer<<"|\n";
}
void show()
{
    for(int i = 0 ; i < nbClients ; ++i)
    {
        std::cout<<"client sockid :"<<clients[i].first<<"\n";
        std::cout<<"client nickname:|"<<clients[i].second<<"|\n";
    }
}
 
public:
    ///////////////////////////////////////////////
    // Public methods
    ///////////////////////////////////////////////
 
    /** The constructor initialises the socket, but not only that 
      * It also installs the socket with the bind and
      * starts the listen.
      * So, as soon the server is created, the clients
      * can be queued even if no call to run has been
      * performed.
      */

    OmpServer(const int inPort)
        : sock(-1), clients(MaximumClients,std::make_pair(-1,"")), nbClients(0), hasToStop(true) {
        memset(&address, 0, sizeof(address));
 
        if( IsValidReturn( sock = socket(AF_INET, SOCK_STREAM, 0)) ){
            // Set option to true
            const int optval = 1;
            if( !IsValidReturn(setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) ) ){
                close();
            }
 
            if( !bind(inPort) ){
                close();
            }
 
            if( !listen() ){
                close();
            }
 
            const int flag = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flag | O_NONBLOCK);
        }
    }
 
    /** Destructor closes the server socket
      * The clients have already been closed (at end of run)
      */
    virtual ~OmpServer(){
        close();
    }
 
    /** Isvalid checks if sock is valid i.e !=-1 */
    bool isValid() const {
        return sock != -1;
    }
 
    /** Simply set the loop boolean to true */
    void stopRun(){
        hasToStop = true;
    }
 
    /** This function closes the socket 
    uses ::close() defined in unistd.h 
    ::close closes a file descriptor */
    bool close(){
        if(isValid()){           
            const bool noErrorCheck = IsValidReturn(::close(sock));
            sock = -1;
            return noErrorCheck;
        }
        return false;
    }
 
    /** The run starts to accept the clients and execute
      * one thread per client.
      * When hasToStop is set to true (stopRun called),
      * the master thread stops to accept clients and waits all
      * others threads.
      * Then the clients sockets are closed
    */

    bool run(){
        if( omp_in_parallel() || !isValid() ){
            return false;
        }
 
        hasToStop = false;
 
        #pragma omp parallel num_threads(MaximumClients)
        {
            #pragma omp single nowait
            {
                while( !hasToStop ){
                    const int newSocket = accept();
                    if( newSocket != -1 ){
                        #pragma omp task untied
                        {
                            processClient(newSocket);
                        }
                    }
                    else{
                        usleep(200);
                    }
                }
                #pragma omp taskwait
            }
        }
 
        // Close sockets with default threads number this time
        #pragma omp parallel for
        for(int idxClient = 0 ; idxClient < nbClients ; ++idxClient ){
            ::close(clients[idxClient].first);
        }
        nbClients = 0;
 
        return true;
    }
};
 
#endif // OMPSERVER_H