#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h> 
#include <sys/socket.h>

using namespace std;

// Most of the comments have been provided in the SimpleEmailServerPhase1.cpp

void NumArguements(const int numarg) {
	if (numarg != 3) {
		cout << "Usage : [filename] [ServerIPAddress]:[PortNumber] [Username] [Password]" << endl;	
		exit(1);
	}
	return;
}


int CreateSocket() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
	perror("ERROR opening socket \nReason ");
	exit(2);
	}
	return sockfd;
}


int ConnectSocket(int sockfd, sockaddr_in address) {
	int connectfd = connect(sockfd, (struct sockaddr *) &address, sizeof(address));
	if (connectfd < 0) {
		perror("ERROR : Connect failed \nReason ");
		exit(2);
	}
	else {
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(address.sin_addr), ip, INET_ADDRSTRLEN);
		cout << "ConnectDone : " << ip << ":" << htons(address.sin_port) << endl;
	}
	return connectfd;
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


int main(int argc, char* argv[]) {
	int numarg = argc - 1;
	string username = argv[2];
	string password = argv[3];
	NumArguements(numarg);

	int clientfd = CreateSocket();

	// Parse arguement to get serverIP and port number
	char* serverIP = strtok(argv[1],":");
	int portNum = atoi(strtok(NULL,":"));

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNum);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);

	// Connect socket to the server
	int connectfd = ConnectSocket(clientfd, serverAddr);
	
	// Send login message
	string login = "User: " + username + " Pass: " + password;
	SendMessage(clientfd, login);

	// Receive reply from server
	cout << "Server : " << ReceiveMessage(clientfd) << endl;
	cout << "Server : " << ReceiveMessage(clientfd) << endl;

	SendMessage(clientfd, "Winter Is Coming!");
	SendMessage(clientfd, "And so is Thanos ;)");
	SendMessage(clientfd, "quit");
}


