# GRASS - GRep AS a Service
> GRASS is buggier on the other side. 
>

This project is a develop, hack and patch challenge for the Software security class @EPFL.

![](pics/grass.jpg)

## Team members

- Delphine Peter 260480
- Tiago Kieliger 258981
- Yann Vonlanthen 258857

## Project structure

```bash
.
├── src			# Source code directory
│   ├── client.cpp	# Main for client
│   ├── server.cpp	# Main for server
│   ├── commands.cpp	# Implementation of commands
│   ├── commands.h	#
│   ├── grass.cpp	# Global constants and hijack flow
│   ├── grass.h		#
│   ├── networking.cpp  # Networking functionality
│   ├── networking.h	#
│   ├── Parser.cpp	# Class to parse user input
│   ├── Parser.h	#
│   ├── User.cpp	# Class modeling user
│   └── User.h		#
├── bin			# Compiled binaries
├── testcases		# Directory containing test cases
├── run_testcases.sh    # Running a client performing all test cases
├── template		# Directory of template files given to us
├── pics		# Contains some pictures for this README and the report
├── Makefile		# Compiles project in 64 bit, no DEP.
├── project-desc.pdf	# Specifications given to us
├── exploits.zip	# Encrypted directory containing all out exploited vulnerabilites
│   ├── exploit1.py	# Buffer Overflow 1
│   ├── exploit2.py	# Buffer Overflow 2
│   ├── exploit3.py	# Format String Attack
│   ├── exploit4.py	# Command injection
│   ├── exploit5.py	# --- Confidential ---
│   ├── backdoor1.py	# --- Confidential ---
│   ├── backdoor2.py	# --- Confidential ---
│   ├── backdoor3.py	# --- Confidential ---
│   ├── project_report.pdf	# PDF detailing the vulnerabilities and exploits.
└── README.md

```

## Introduction

The aim of this project was to write an ssh-like client/server application, allowing functionalities like *mkdir, cd, ls, ping, put, get, grep, etc.*
It is fully written in C++, without external libraries, and is compiled for a 64-bit architecture.

Additionally 5 vulnerabilities had to be hidden, and a proof of concept on how to exploit them was established. (pwntool python script for each) Each exploit poc either needs to open `xcalc` or redirect flow to the `hijack_flow()` function to be accepted as such.

Finally we could also hide additional back-doors, which we have done also.

After this initial phase, other teams are asked to find the vulnerabilities for bonus points, while we will need to patch them in the third and final phase.

## How to run GRASS

After compiling using the `make` command, one can run an instance of the GRASS server from the top level directory by executing:

```bash
$ bin/server # grass.conf must be in current dir
```

Then, in other shells, or on other hosts in the network, one can run a client as follows:

```bash
$ bin/client 127.0.0.1 1337 [infile outfile] # Adapt IP and port according to server .conf file
```

Not that the infile specifies a file for input instead of stdin, and an outfile for output instead of stdout.

Finally, if one wants to run the test cases one can run them as follows:

```bash
$ bash run_testcases.sh # the server must be running
```

## Back-doors added

We have added **3** back-doors. 

## Implementation details

- We have chosen a blocking implementation, where the client waits for a server response, for any command. The only exception to this is for the put and get commands, where of course the file upload/download is done in parallel.
- We chose to stick with a 64-bit compilation. (Important for some exploits)
- The grass.conf file must be placed where where the binaries/scripts are launched from.
- The base dir setting in the conf file is relative to the conf file itself, and the path specified must exist before starting the program.
