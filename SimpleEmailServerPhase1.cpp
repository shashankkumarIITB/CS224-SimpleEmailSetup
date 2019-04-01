#include <arpa/inet.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string.h> 
#include <sys/socket.h>

using namespace std;


void NumArguements(const int numarg) {
	if (numarg != 2) {
		cout << "Usage : [filename] [PortNumber] [Passwordfile]" << endl;	
		exit(1);
	}
	return;
}


void FileExists(const string& filename, map<string, string>& userdata) {
  ifstream ifile(filename.c_str());
  if (not (bool)ifile) {
  	cout << "File : " << filename << " Does not exist." << endl;
  	exit(3);
  }
  else if (ifile.good() == 0) {
  	cout << "File : " << filename << " Read permission denied." << endl;
  	exit(3);
  }
  else {
  	string line;
	while (getline(ifile, line))
	{
	    istringstream isstring(line);
	    string username, password;
	    if (!(isstring >> username >> password)) { 
	    	continue;
	    }
	    userdata.insert({username,password});
	}
  }
  return;
}


int CreateSocket() {
	// Create a socket with TCP protocol
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	/* 
	The first agrument specifies the address domain.
	1. AF_INET => The Internet domain for any two hosts on the internet
	2. AF_UNIX => The Unix domain for two process sharing common file structure
	
	The second arguement tells about the type of socket.
	1. SOCK_STREAM => Data read as continuous stream
	2. SOCK_DGRAM => Data read in chunks
	
	The arguement represents the protocol used.
	0 => The system will choose the most appropriate protocol.
	TCP for SOCK_STREAM and UDP for SOCK_DGRAM 
	*/
	if (sockfd < 0) {
		perror("ERROR opening socket \nReason ");
		exit(2);
	}
	return sockfd;
}


void DeleteSocket(int sockfd) {
	/*int isetoption = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&isetoption, sizeof(isetoption));*/
	shutdown(sockfd, 2);
}


int BindSocket(int sockfd, const sockaddr_in address) {
	// Bind the socket with the address
	int bindfd = bind(sockfd, (struct sockaddr *)&address, sizeof(address));
	if (bindfd < 0) {
		perror("ERROR : Bind failed \nReason ");
		DeleteSocket(sockfd);
		exit(2);
	}
	else {
		cout << "BindDone : " << htons(address.sin_port) << endl;
	}
	return bindfd;
}


int ListenSocket(int sockfd, int portNum) {
	// Specify the length of queue
	int queuelen = 5;

	// Listen on the socket for the queue length specified
	int listenfd = listen(sockfd, queuelen);
	if (listenfd < 0) {
		perror("ERROR : Listen failed \nReason ");
		DeleteSocket(sockfd);
		exit(2);
	}
	else {
		cout << "ListenDone : " << portNum << endl;
	}
	return listenfd;
}


int AcceptConnection(int sockfd, const sockaddr_in address) {
	socklen_t addrlen = sizeof(address);
	int acceptfd = accept(sockfd, (struct sockaddr *)&address, &(addrlen));
	if (accept < 0) {
		perror("ERROR : Accept failed \nReason ");
		DeleteSocket(sockfd);
		exit(2);
	}
	else {		
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(address.sin_addr), ip, INET_ADDRSTRLEN);
		cout << "Client : " << ip << " : " << address.sin_port << endl;
	}
	return acceptfd;
}


string ReceiveMessage(int sockfd) {
	int bufferlen = 1024;
	char* buffer = new char[bufferlen + 1];
	int bytesRecv = recv(sockfd, buffer, bufferlen + 1, 0);
	return string(buffer);
	/*while (bytesRecv != 0 or bytesRecv != -1) {
		msg += string(buffer);
		bytesRecv = recv(sockfd, buffer, bufferlen + 1, 0);
	}*/
}


void SendMessage(int sockfd, string msg) {// char* buffer, int bufferlen) {
	int bufferlen = 1024;
	int bytesToSend = msg.length();
	int bytesSent = 0;
	while (bytesToSend != 0) {
		int currentBufferlen = min(bufferlen, bytesToSend);
		char* buffer = new char[currentBufferlen + 1];
		strcpy(buffer, msg.substr(bytesSent, currentBufferlen).c_str());
		int sent = send(sockfd, buffer, currentBufferlen  + 1, 0);
		bytesToSend = bytesToSend - sent + 1;
		bytesSent = bytesSent + sent - 1;
	}
}


void Validate(int sockfd, string s, map<string, string>& userdata, string& user) {
	string User, Pass;
	string username;
	string password;
	
	istringstream isstring(s);
	if (!(isstring >> User >> username >> Pass >> password)) {
		if (User != "User:" or Pass != "Pass:") {
		cout << "Unknown command." << endl;
		DeleteSocket(sockfd);
		exit(4);
		}
	}
	else {
	// Check if the user and password specifies belongs to the database 
		bool userfound = false;
		for (map<string, string>::iterator it = userdata.begin(); it != userdata.end(); it++) {
			if (it->first == username) {
				if (it->second == password) {
					user = username; 
					userfound = true;
					string welcome = "Welcome, " + username;
					cout << welcome << endl;
					SendMessage(sockfd, welcome);
				}
				else {
					cout << "Password incorrect." << endl;
					DeleteSocket(sockfd);
					exit(4);	
				}
			}
			else {
				continue;
			}
		}
		if (!userfound){
			cout << "Invalid User : " << username << endl;
			DeleteSocket(sockfd);
			exit(4);
		}
	}
}

int main(int argc, char*argv[]) {
	// Check the number of arguements
	int numarg = argc - 1;
	
	// Check for the right number of arguements
	NumArguements(numarg);
	string filename = argv[2];
	map<string, string> userdata;
	
	// Check if the file exists
	FileExists(filename,userdata);
	
	// Create a file descripter
	int serverfd = CreateSocket();
	// Get the port number from the input
	int portNum = atoi(argv[1]);

	// Create an entity of type sockaddr	
	struct sockaddr_in serverAddr;
	// Should match that of the socket
	serverAddr.sin_family = AF_INET;
	// Assign the port number entered by the user
	serverAddr.sin_port = htons(portNum);
	// Address of the host, for server = IP address of the machine
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket
	BindSocket(serverfd, serverAddr);
			
	// Listen on the socket
	ListenSocket(serverfd, portNum);

	// Accept connection request on the socket
	struct sockaddr_in clientAddr;
	int acceptfd = AcceptConnection(serverfd, clientAddr);
	
	// Receive messages from the client
	string recvmsg = ReceiveMessage(acceptfd);
	
	// Validate username and password
	string username;
	Validate(acceptfd, recvmsg, userdata, username);
	
	// Begin sending and receiving messages after authentication 
	SendMessage(acceptfd, "It's getting cold.");
	bool recvquit = false;
	for (int i = 0; i < 100; i++) {
		string recvmsg = ReceiveMessage(acceptfd);
		if (recvmsg == "quit") {
			recvquit = true;
			cout << "Bye, " << username << endl;
			DeleteSocket(acceptfd);
			exit(5);
		}
		wait();
	}
	if (!recvquit) {
		cout << "ERROR Closing connection.\nReason : Idle for too long." << endl;
		exit(5);
	}
	
	// Problem with the existence of file
	// Clear the socket using setsocketopt
	// Network byte order 10:00
	// inet_aton() 12:41
	// inet_noa()
}





