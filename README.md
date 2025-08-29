# Peer-to-Peer Messenger (Version 1.0)

## Overview

This project is a **Peer-to-Peer Messenger** built in C++ to explore networking, concurrency, and distributed systems. It was started in **August 2025** and features:

- **Terminal-based UI**
- **Peer-to-peer messaging using Boost.Asio**
- **Persistent message logging**

This is **version 1.0**, with ongoing development for:
- **Version 2.0 (Encryption)** – currently in progress  
- **Version 3.0 (File Transfers)** – planned  

---

## Motivation

The project was designed to provide practical experience in networking, concurrency, and system design. A peer-to-peer messenger allows experimentation with asynchronous communication, message handling, and decentralized network interactions.

---

## Features

- **Peer-to-Peer Networking**: Connect to peers, accept incoming connections, and send/receive messages using TCP sockets with Boost.Asio.  
- **Terminal UI**: Connect to peers, send messages, broadcast to all peers, and view message history.  
- **Message Logging**: Saves messages to  
  - `logs/messages_sent.log`  
  - `logs/messages_received.log`  

---

## Development Stages

The project was built in stages:

1. **P2P Core**: Implemented the networking layer (`NetworkManager`, `Peer`) with asynchronous TCP communication.  
2. **User Interface**: Added a terminal-based interface for peer connections and message management.  
3. **Logging**: Implemented `LogManager` to store messages in files for persistence.  

Challenges included handling asynchronous callbacks, ensuring thread safety with mutexes, and structuring the code modularly.

---

## Technical Highlights

- **Asynchronous Networking** using Boost.Asio  
- **Thread Safety** with mutexes in `NetworkManager` and `LogManager`  
- **Modular Design** with namespaces: `network`, `logging`, `ui`, `message`  
- **Error Handling** for network and file operations  

---

## Lessons Learned

Key technical lessons from the project:

- Using **Boost.Asio** for asynchronous networking  
- Managing memory with **smart pointers** (`std::shared_ptr`)  
- Handling concurrency with **mutexes** and detached threads  
- Structuring a project with **clear separation of concerns**  
- Debugging asynchronous code and linker issues  

---

## Future Plans

- **Version 2.0 (Encryption)** – currently in progress  
- **Version 3.0 (File Transfers)** – planned  

These features will expand the use of cryptography, file handling, and large-scale data transfer.

---

## Project Structure

- `headers/`: Header files for all modules (`network`, `logging`, `ui`, `message`)  
- `source/`: Source files organized by module  
- `build/`: Compiled object files (generated during build)  
- `logs/`: Directory for message log files (created at runtime)  

---

## Building

Requires **C++17**, **Boost.Asio**, **Boost.Thread**, and **pthread**.

    make

This builds the **p2p executable** for the messenger.

---

## Usage

### Run the messenger

    ./p2p [port]

- Default port is `5555` if unspecified  
- Terminal UI allows:  
  - Connecting to peers (`IP:port`)  
  - Listing connected peers  
  - Sending messages or broadcasting to all peers  
  - Viewing and deleting sent/received messages  
  - Exiting cleanly  

---

## Notes

- The local IP is determined using **Google DNS (8.8.8.8)**, falling back to `"unknown:<port>"` if unavailable  
- Messages are limited to **1024 bytes**; larger messages may be truncated  
- Log files are overwritten on updates  
- Some features (e.g., message read status, UI observer) are reserved for future versions  

---

## Limitations

- Peer address matching in the UI is **string-based**  
- Minimal error handling; some errors may be silently ignored  
- The Makefile does not track header dependencies; run `make clean` for header changes

