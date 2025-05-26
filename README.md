
# ft_irc

**IRC Server Compatible with irssi**

## Overview

A lightweight, RFC-compliant IRC server written in  **C++17**, designed to work seamlessly with the  **irssi**  client. Supports multi-user channels, private messaging, and operator commands.

----------

## Features

###   **Core IRC Protocol**

-   **Authentication**:  `PASS`,  `NICK`,  `USER`
    
-   **Channels**:  `JOIN`,  `PART`,  `PRIVMSG`,  `LIST`
    
-   **Operator Tools**:  `KICK`,  `INVITE`,  `TOPIC`,  `MODE`  (+i, +t, +k, +o)
    

###   **Key Strengths**

-   **irssi-Compatible**: Fully tested with  `irssi`  for real-world use.
    
-   **Non-Blocking I/O**: Uses  `poll()`  for efficient multi-client handling.
    
-   **Robust Parsing**: Handles fragmented commands (e.g.,  `CTRL+D`  in  `nc`).
    

----------

##   **Technical Implementation**

-   **Language**: Strict C++17 (STL containers, no external libs).
    
-   **Networking**: TCP/IPv4 with non-blocking sockets.
    
-   **Error Handling**: RFC-compliant error replies (e.g.,  `ERR_NONICKNAMEGIVEN`).
    

----------

##   **Quick Start**

1.  **Build**:
    
```bash
make          # Compiles to `ircserv`
```
    
2.  **Run**:
    
```bash    
./ircserv 6667 password  
```   
3.  **Connect with irssi**:
    
```bash
irssi -c 127.0.0.1 -p 6667 -w password  
``` 

----------

##   **Example Workflow**

1.  Set nickname:
```    
/nick Alice  
```
2.  Join a channel:
```    
/join #coders  
```
3.  Send messages:
```
Hello world!  
```    

----------

##   **Notes**

-   Tested on  **Linux**  and  **macOS**.
    
-   For debugging, use  `nc`  to send raw commands:
    
```bash 
nc -C 127.0.0.1 6667
```
