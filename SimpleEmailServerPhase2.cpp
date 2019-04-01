#include <arpa/inet.h>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;


void NumArguements(const int numarg) {
	if (numarg != 3) {
		cout << "Usage : [filename] [PortNumber] [Passwordfile] [FolderPath]" << endl;	
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
	    userdata.insert({username, password});
	}
  }
  return;
}


bool FolderExists (string folderpath) {
	struct stat info;
	char* buffer = new char[folderpath.length() + 1];
	strcpy(buffer, folderpath.c_str());

	if (stat(buffer, &info) != 0) {
	// Check if the directory exists
		return false;
	}
	else if (info.st_mode & S_IFDIR) {
	// Check if the directory is accessible
		return true;
	}
	else {
		return false;
	}
}


int NumFiles (string path) {
	// Returns the number of files and folders present at the specifies path 
	string s = ".";
	string s1 = "..";
	char *ptr = new char[2];
	char *ptr1 = new char[3];
	char *buffer = new char[path.length() + 1];
	strcpy(ptr, s.c_str());
	strcpy(ptr1, s1.c_str());
	strcpy(buffer, path.c_str());
	
	int count = 0;
	DIR *d;
	struct dirent *dir;
	d = opendir(buffer);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strcmp(ptr, dir->d_name) && strcmp(ptr1, dir->d_name)) {
				// Uncomment the below line to display file/directory names
				// cout << dir->d_name << endl;
				count++;
			}
		}
	}
	return count;
}


int CreateSocket() {
	// Create a socket with TCP protocol
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
}


void SendMessage(int sockfd, string msg) {
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


void Validate(int serverfd, int sockfd, string s, map<string, string>& userdata, string& user) {
	string User, Pass;
	string username;
	string password;
	
	istringstream isstring(s);
	if (!(isstring >> User >> username >> Pass >> password)) {
		if (User != "User:" or Pass != "Pass:") {
		cout << "Unknown command." << endl;
		SendMessage(sockfd, "Unknown command.");
		DeleteSocket(sockfd);
		exit(100);
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
					SendMessage(sockfd, "Password incorrect.");
					DeleteSocket(sockfd);
					exit(100);	
				}
			}
			else {
				continue;
			}
		}
		if (!userfound){
			cout << "Invalid User : " << username << endl;
			SendMessage(sockfd, "Invalid user.");
			DeleteSocket(sockfd);
			exit(100);
		}
	}
}


int main(int argc, char*argv[]) {
	int numarg = argc - 1;
	
	NumArguements(numarg);
	string filename = argv[2];
	string folderpath = argv[3];
	map<string, string> userdata;
	
	FileExists(filename,userdata);
	bool dirExists = FolderExists(folderpath);
	if (! dirExists) {
		perror("Unable to access given user database.\nReason ");
		exit(4);
	}
	
	// Create a file descripter
	int serverfd = CreateSocket();
	int portNum = atoi(argv[1]);

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNum);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	BindSocket(serverfd, serverAddr);
	ListenSocket(serverfd, portNum);
	
	while (true) {
		struct sockaddr_in clientAddr;
		int acceptfd = AcceptConnection(serverfd, clientAddr);

		string recvmsg = ReceiveMessage(acceptfd);
		string username;
		Validate(serverfd, acceptfd, recvmsg, userdata, username);
		
		while (true) {
			recvmsg = ReceiveMessage(acceptfd);
			if (recvmsg == "quit") {
				string msg = "Bye, " + username; 
				cout << msg << endl;
				SendMessage(acceptfd, "Bye.");
				break;
			}
			else if (recvmsg == "LIST") {
				if (FolderExists(folderpath + username + "/")) {
				int n = NumFiles(folderpath + username + "/");
				cout << username << " : Number of files : " << n << endl;
				SendMessage(acceptfd, "Number of files : " + to_string(n));
				}
				else {
					cout << username << " : Folder read failed." << endl;
					SendMessage(acceptfd, "Folder read failed.");
					break;
				}
			}
			else {
				cout << "Unknown command." << endl;
				SendMessage(acceptfd, "Unknown command.");
				break;
			}
		}
	}

	/*
	Exit status :
	0 => Success
	1 => Wrong number of arguements
	2 => 
		a) Socket creation failed
		b) Bind failed
		c) Listen failed
		d) Accept failed		
	3 => Password file not present
	4 => Unable to access userdata directory
	100 => Otherwise
	*/

}
