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
	if (numarg != 5) {
		cout << "Usage : [filename] [Server IPAddress]:[Port Number] [Username] [Password] [File List] [Local Folder]" << endl;	
		exit(1);
	}
	return;
}


void ParseList (string lst) {
	int lstlen = lst.length();
	bool parseable = true;
	bool comma = false;
	bool space = false;
	for (int i = 0; i < lstlen; i++) {
		if (isdigit(lst[i]) and !space) {
			comma = true;
		}
		else if (lst[i] == ',' and comma and i != lstlen - 1) {
			comma = true;
			space = true;
		}
		else if (lst[i] == ' ' and space and i != lstlen - 1) {
			space = false;
		}
		else {
			parseable = false;
			break;
		}
	}
	if (! parseable) {
		cout << "ERROR : Unable to parse the list of file(s)." << endl;
		exit (3);
	}
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


void CreateDirectory (string path) {
	char *directory = new char[path.length()];
	int check = mkdir(strcpy(directory, path.c_str()),0777);
	if (check) {
		perror("ERROR : Unable to create local directory.\nReason ");
		exit(4);
	}
}


int RemoveDirectory (const char* path) {
	string s = "rm -rf " + string(path);
	const char* command = s.c_str();
	return system(command);
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


void WriteFile (string filepath) {
	ofstream file (filepath, ofstream::binary | ofstream::app);
	string s = "33B, Baker St., London.Winter is coming! -SH\n";
	int slen = s.length();
	char *buffer = new char[slen];
	strcpy(buffer, s.c_str());
	file.write(buffer, slen);
	file.close();
	return;
}


void ReceiveFile(int sockfd, string filepath, int filesize) {
	int bytesRecv = 0;
	int defaultbufferlen = 1024;

	// Method 1	
	ofstream file (filepath.c_str(), ofstream::app);
	while (bytesRecv != filesize) {
		int bufferlen = min(defaultbufferlen, filesize - bytesRecv);
		char* buffer = new char[bufferlen + 1];
		int currentBytesRecv = recv(sockfd, buffer, bufferlen + 1, 0);
		file.write(buffer, bufferlen);	
		bytesRecv += bufferlen;
		delete buffer;
	}
	file.close();

	/* // Method 2
	struct stat filest;
	char *filedest = new char[filepath.length() + 1];
	strcpy(filedest, filepath.c_str());
	char *write = new char[2];
	write[0] = 'w';
	write[1] = 'b';
	FILE* fp = fopen(filedest, write);
	while (bytesRecv <= filesize) {
		char *buffer = new char[bufferlen + 1];
		int currentBytesRecv = recv(sockfd, buffer, bufferlen + 1, 0);
		int currentBytesWritten = fwrite(buffer, sizeof(char) ,bufferlen + 1, fp); 
		cout << "currentBytesRecv : " << currentBytesRecv << endl;
		bytesRecv += currentBytesRecv - 1;
		delete buffer;
	}
	fclose(fp);
	*/
}


int main(int argc, char* argv[]) {
	int numarg = argc - 1;
	NumArguements(numarg);
	string username = argv[2];
	string password = argv[3];
	string filelist = argv[4];
	ParseList(filelist);
	string localfolder = argv[5];

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
	
	// LIST	
	SendMessage(clientfd, "LIST");
	cout << "Server : " << ReceiveMessage(clientfd) << endl;
	
	// RETRV
	int index = 0;
	bool createfolder = true;
	bool requestfile = false;
	for (int i = 0; i < filelist.length(); i++) {
		string fileid;
		if (i == filelist.length() - 1) {
			fileid = filelist.substr(index, i - index + 1);
			requestfile = true;
		}

		if (filelist[i] == ',') {
			fileid = filelist.substr(index, i - index);
			requestfile = true;
			index = i + 2;
		}

		if (requestfile) {
			requestfile = false;
			SendMessage(clientfd, "RETRV " + fileid);
			string filename = ReceiveMessage(clientfd);
			cout << "Server : " << filename << endl;
			string filesizeString = ReceiveMessage(clientfd);
			cout << "Server : " << filesizeString << endl;

			int filesize = atoi(filesizeString.substr(12).c_str());
			string filepath = localfolder + "/" + filename;
			
			if (createfolder) {
				RemoveDirectory(localfolder.c_str());
				CreateDirectory(localfolder.c_str());
				createfolder = false;
			}
			CreateFile(filepath);
			if (filesize != 0) {
				ReceiveFile(clientfd, filepath, filesize);
			}
			cout << "Downloaded message : " << fileid << endl;
		}
	}

	// QUIT
	SendMessage(clientfd, "quit");

	/*
	Exit status :
	1 => Wrong number of arguements
	2 => 
		a) Socket creation failed
		b) Bind failed
	*/

}


