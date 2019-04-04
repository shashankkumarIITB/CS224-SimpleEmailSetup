#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h> 
#include <string.h> 
#include <sys/stat.h>
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
	string recvmsg = string(buffer);
	delete(buffer);
	return recvmsg;
}


void SendMessage(int sockfd, string msg) {
	int bufferlen = 1024;
	int bytesToSend = msg.length();
	int bytesSent = 0;
	while (bytesToSend >= 0) {
		// int currentBufferlen = min(bufferlen, bytesToSend);
		char* buffer = new char[bufferlen + 1];
		strcpy(buffer, msg.substr(bytesSent, bufferlen).c_str());
		int sent = send(sockfd, buffer, bufferlen + 1, 0);
		bytesToSend = bytesToSend - sent + 1;
		bytesSent = bytesSent + sent - 1;
		delete(buffer);
	}
	return;
}


bool FolderExists (string folderpath) {
	// Check if the specified folder exists
	struct stat info;
	char* buffer = new char[folderpath.length() + 1];
	strcpy(buffer, folderpath.c_str());

	if (stat(buffer, &info) != 0) {
	// Check if the directory exists
		delete buffer;
		return false;
	}
	else if (info.st_mode & S_IFDIR) {
	// Check if the directory is accessible
		delete buffer;
		return true;
	}
	else {
		delete buffer;
		return false;
	}
}


void CreateFile(string filepath) {
	// Create a file in the username directory
	ofstream file; 
	file.open(filepath.c_str());
	if (!file) {
		cout << "File : " << filepath << " could not be created." << endl;
		return;
	}
	cout << "File : " << filepath << " created successfully." << endl; 
	file.close();
	return;
}


void ReceiveFile(int sockfd, string username, string filename, int filesize) {
	if (! FolderExists(username)) {
		char *directory = new char[username.length()];
		mkdir(strcpy(directory, username.c_str()),0777);
		cout << "Directory : " << directory << " created successfully." << endl;
	}

	else {
		cout << "Directory exists." << endl;
	}
	/*string filepath = username + "/" + filename;
	CreateFile(filepath);
	
	// Write to the file
	int bytesRecv = 0;
	int bufferlen = 1024;
	ofstream file;
	file.open(filepath.c_str(), ofstream::binary | ofstream::app);
	while (bytesRecv <= filesize) {
		char* buffer = new char[bufferlen];
		int currentBytesRecv = recv(sockfd, buffer, bufferlen, 0);
		file.write(buffer, bufferlen);
		bytesRecv += currentBytesRecv;
		delete buffer;
	}
	file.close();*/
}

int main(int argc, char* argv[]) {
	int numarg = argc - 1;
	NumArguements(numarg);
	string username = argv[2];
	string password = argv[3];
	
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
	cout << "Server : " << ReceiveMessage(clientfd) << endl;
	
	/*
	SendMessage(clientfd, "LIST");
	cout << "Server : " << ReceiveMessage(clientfd) << endl;
	*/
	/*
	SendMessage(clientfd, "RETRV 4");
	string filename = "4.txt";
	string filesizeString = ReceiveMessage(clientfd);
	cout << "Server : " << filesizeString << endl;
	int filesize = atoi(filesizeString.substr(12).c_str());
	cout << "Filesize : " << filesize << endl;
	if (filesize != 0) {
		ReceiveFile(clientfd, username, filename, filesize);
	}
	else {
		CreateFile(username + "/" + filename);
	}
	*/
	
	SendMessage(clientfd, "quit");
		
	/*
	Exit status :
	1 => Wrong number of arguements
	2 => 
		a) Socket creation failed
		b) Bind failed
	*/

}


