# Online-Library-Management-System
An Online Library Management System built in C using socket programming and system calls. Features include secure user authentication, book/member management, concurrent client access, and reliable file handling with locks for data consistency. A robust solution for library operations.

## How to Run

### Prerequisites
- Linux system with gcc and make
- POSIX threads and sockets (standard on Linux)

### Build
In the project directory, run:

```bash
make
```

This will build both the server and client executables.

### Start the Server
In one terminal:

```bash
./server
```

The server will listen on 127.0.0.1:8080.

### Start the Client
In another terminal:

```bash
./client
```

Follow the on-screen menu to register, login, and manage books.

### Notes
- You can run multiple clients at once.
- Data is stored in `users.txt`, `books.txt`, and `book_user.txt`.
- To clean up builds, run:

```bash
make clean
```
