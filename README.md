# This is a peer to peer system for downloading documents with a central index server. More details can be found in the project document

# P2P Central Index

IMPLEMENTATION:
This project has been implemented in c++ (c++11)

REQUIREMENTS:
You must have g++ compiler as well as c++11 support.

MAKE:
There is a MAKEFILE that has been included with the project.
To run the project:
make clean; make

This will generate the p2p executable


EXECUTION INSTRUCTIONS:
Run it using ./p2p

There will be 2 choices:
0 for Bootstrap server
1 for peer (client)

Server:
Listens on the well known port 7734
It is multithreaded. 
For each incoming client request a new server thread is spawned to handle it.

Client:
1. When you start a client there will be prompts to configure:
2. Bootstrap server IP.
3. Upload server port (peer server on which other peers can ask this client for RFCs)
4. Client IP (this clients IP address).

After this configuration you can enter ADD, LOOKUP, LIST or GET commands.
Format of these commands is as described in the project document.
Sample commands can be found in the file sample_add_cmd.txt

To separate two consecutive commands please enter the string "EOCEOCEOC".
This is a requirement for correct functioning as I am using this as a delimiter after
a command.


RFCs:
RFCs are maintained in the RFC/ directory of the project.
The rfc text files in RFC/ directory have the name format rfcnumber.txt.
eg. 2547.txt
(Please look at the RFC directory for clarification)

A GET request will ask a client (Peer server) to look at it's RFC/ directory and transfer the 
corresponding rfcnumber.txt from it's RFC/ directory if it exists.

When a client gets a GET response from the server it stores the received RFC in it's 
RFC/ directory.
You may want to issue an ADD command after this if you wish to inform the Bootstrap server
that you are willing to serve this RFC in the future.

TESTING:
This has been tested on AWS with 3 VMs (1 Server and 2 clients).
Make sure your ports have been opened and if you're using privileged port use root.
