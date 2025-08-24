#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/file.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define MAX_USERS 100
#define MAX_BOOKS 100

// User structure definition
typedef struct {
    char username[50];
    char password[50];
    int admin;
} User;

// Book structure definition
typedef struct {
    char title[100];
    char author[100];
    char isbn[20];
    bool available;
} Book;

// Global arrays to hold users and books
User users[MAX_USERS];
Book books[MAX_BOOKS];
int num_users = 0;
int num_books = 0;
pthread_mutex_t lock;

void *process_request(void *arg);
void load_users();
void load_books();
void save_users();
void save_books();
bool register_user(char *username, char *password, int flag);
bool authenticate_user(char *username, char *password, int* admin_flag);
void add_book(char *title, char *author, char *isbn);
void delete_book(char *title);
void modify_book(char *title, char *author, char *isbn);
void search_book(char *title, char *buffer);
void borrow_book(char *username, char *title, char *buffer);
void return_book(char *username, char *title, char *buffer);

int main() {
    int sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Define server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Bind socket to the address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding to address");
        exit(EXIT_FAILURE);
    }

    // Initialize mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init has failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections (max. 10 concurrent connections allowed)
    if (listen(sockfd, 10) < 0) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    
    // Load users and books data from files
    load_users();
    load_books();

    while (1) {
        // Accept incoming connection
        addr_size = sizeof(client_addr);
        client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sockfd < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the client request
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, process_request, (void *)&client_sockfd) != 0) {
            perror("Error creating thread");
            close(client_sockfd);
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }

    // Destroy mutex lock
    pthread_mutex_destroy(&lock);
    
    close(sockfd);
    return 0;
}

// Function to handle client requests
void *process_request(void *arg) {
    int client_sockfd = *((int *)arg);
    char buffer[BUFFER_SIZE];
    bool login_flag = false;
    int admin_flag = 0;
    char username[50] = {0};
    while (1) {
        // Receive data from the client
        int bytes_received = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            perror("Error Receiving Data From Client");
            close(client_sockfd);
            return NULL;
        }
        buffer[bytes_received] = '\0';

        // Process client request based on the command
        if (buffer[0] == 'Q') break;    // Break command
        else if (buffer[0] == 'R') { // User registration request
            char password[50];
            char flag[1];
            sscanf(buffer + 1, "%s %s %s", username, password, flag);
            int flg = atoi(flag);
            bool registered = register_user(username, password, flg);
            if (registered) {
                strcpy(buffer, "User registered successfully");
            } else {
                strcpy(buffer, "ERR: User already exists");
            }
        } else if (buffer[0] == 'A') { // User access request (login)
            if (login_flag)
                strcpy(buffer, "ERR: Already Logged In");
            else {
                char password[50];
                sscanf(buffer + 1, "%s %s", username, password);
                bool authenticated = authenticate_user(username, password, &admin_flag);
                if (authenticated) {
                    if (admin_flag) {
                        strcpy(buffer, "Admin Logged In Successfully");
                        login_flag = true;
                    } else {
                        strcpy(buffer, "User Logged In Successfully");
                        login_flag = true;
                    }
                } else {
                    strcpy(buffer, "ERR: Incorrect Credentials");
                }
            }
        } else if (buffer[0] == 'C') { // Add book request
            if (!admin_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                char author[100];
                char isbn[20];
                sscanf(buffer + 1, "%s %s %s", title, author, isbn);
                add_book(title, author, isbn);
                strcpy(buffer, "Book Added Successfully");
            }
        } else if (buffer[0] == 'D') { // Delete book request
            if (!admin_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                sscanf(buffer + 1, "%s", title);
                delete_book(title);
                strcpy(buffer, "Book Deleted Successfully");
            }
        } else if (buffer[0] == 'E') { // Modify book request
            if (!admin_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                char author[100];
                char isbn[20];
                sscanf(buffer + 1, "%s %s %s", title, author, isbn);
                modify_book(title, author, isbn);
                strcpy(buffer, "Book Modified Successfully");
            }
        } else if (buffer[0] == 'F') { // Search book request
            if (!login_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                sscanf(buffer + 1, "%s", title);
                search_book(title, buffer);
            }
        } else if (buffer[0] == 'B') { // Borrow book request
            if (!login_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                sscanf(buffer + 1, "%s", title);
                borrow_book(username, title, buffer);
            }
        } else if (buffer[0] == 'U') { // Return book request
            if (!login_flag)
                strcpy(buffer, "ERR: Not Authorised");
            else {
                char title[100];
                sscanf(buffer + 1, "%s", title);
                return_book(username, title, buffer);
            }
        } else
            strcpy(buffer, "ERR: Invalid Request");

        // Send response to client
        if (send(client_sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Error Sending Data To Client");
        }
    }

    close(client_sockfd);
    return NULL;
}

// Load users from file
void load_users() {
    FILE *file = fopen("users.txt", "r");
    if (file == NULL) {
        perror("Error opening users.txt");
        return;
    }
    flock(fileno(file), LOCK_SH); // Shared lock for reading
    num_users = 0;
    while (fscanf(file, "%s %s %d", users[num_users].username, users[num_users].password, &users[num_users].admin) != EOF) {
        num_users++;
    }
    flock(fileno(file), LOCK_UN); // Unlock the file
    fclose(file);
}

// Load books from file
void load_books() {
    FILE *file = fopen("books.txt", "r");
    if (file == NULL) {
        perror("Error opening books.txt");
        return;
    }
    flock(fileno(file), LOCK_SH); // Shared lock for reading
    num_books = 0;
    while (fscanf(file, "%[^,],%[^,],%[^,],%d\n", books[num_books].title, books[num_books].author, books[num_books].isbn, (int *)&books[num_books].available) != EOF) {
        num_books++;
    }
    flock(fileno(file), LOCK_UN); // Unlock the file
    fclose(file);
}

// Save users to file
void save_users() {
    FILE *file = fopen("users.txt", "w");
    if (file == NULL) {
        perror("Error opening users.txt");
        return;
    }
    flock(fileno(file), LOCK_EX); // Exclusive lock for writing
    for (int i = 0; i < num_users; i++) {
        fprintf(file, "%s %s %d\n", users[i].username, users[i].password, users[i].admin);
    }
    flock(fileno(file), LOCK_UN); // Unlock the file
    fclose(file);
}

// Save books to file
void save_books() {
    FILE *file = fopen("books.txt", "w");
    if (file == NULL) {
        perror("Error opening books.txt");
        return;
    }
    flock(fileno(file), LOCK_EX); // Exclusive lock for writing
    for (int i = 0; i < num_books; i++) {
        fprintf(file, "%s,%s,%s,%d\n", books[i].title, books[i].author, books[i].isbn, books[i].available);
    }
    flock(fileno(file), LOCK_UN); // Unlock the file
    fclose(file);
}

// Register a new user
bool register_user(char *username, char *password, int flag) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);
    
    // Check if the username already exists
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&lock);
            return false; // Username already exists
        }
    }
    // Add the new user
    strcpy(users[num_users].username, username);
    strcpy(users[num_users].password, password);
    users[num_users].admin = flag;
    num_users++;
    save_users();
    pthread_mutex_unlock(&lock);
    return true;
}

// Authenticate user credentials
bool authenticate_user(char *username, char *password, int *admin_flag) {
    // Check if the provided username and password match any user's credentials
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            *admin_flag = users[i].admin;
            return true;
        }
    }
    return false;
}

// Add a new book to the library
void add_book(char *title, char *author, char *isbn) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);
    
    strcpy(books[num_books].title, title);
    strcpy(books[num_books].author, author);
    strcpy(books[num_books].isbn, isbn);
    books[num_books].available = true;
    num_books++;
    save_books();
    pthread_mutex_unlock(&lock);
}

// Delete a book from the library
void delete_book(char *title) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);

    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].title, title) == 0) {
            // Shift books after the deleted book one position to the left
            for (int j = i; j < num_books - 1; j++) {
                strcpy(books[j].title, books[j + 1].title);
                strcpy(books[j].author, books[j + 1].author);
                strcpy(books[j].isbn, books[j + 1].isbn);
                books[j].available = books[j + 1].available;
            }
            num_books--;
            break;
        }
    }
    save_books();
    pthread_mutex_unlock(&lock);
}

// Modify details of an existing book
void modify_book(char *title, char *author, char *isbn) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);

    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].title, title) == 0) {
            strcpy(books[i].author, author);
            strcpy(books[i].isbn, isbn);
            break;
        }
    }
    save_books();
    pthread_mutex_unlock(&lock);
}

// Search for a book in the library
void search_book(char *title, char *buffer) {
    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].title, title) == 0) {
            sprintf(buffer, "Book found:\nTitle: %s\nAuthor: %s\nISBN: %s\n", books[i].title, books[i].author, books[i].isbn);
            return;
        }
    }
    strcpy(buffer, "Book not found.\n");
}

// Borrow book
void borrow_book(char *username, char *title, char *buffer) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);

    FILE *file = fopen("book_user.txt", "a+");
    if (file == NULL) {
        perror("Error opening book_user.txt");
        pthread_mutex_unlock(&lock);
        strcpy(buffer, "ERR: Could not borrow book");
        return;
    }
    flock(fileno(file), LOCK_EX);

    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].title, title) == 0) {
            if (!books[i].available) {
                strcpy(buffer, "ERR: Book not available");
                flock(fileno(file), LOCK_UN);
                fclose(file);
                pthread_mutex_unlock(&lock);
                return;
            }
            books[i].available = false;
            fprintf(file, "%s,%s\n", title, username);
            save_books();
            strcpy(buffer, "Book borrowed successfully");
            flock(fileno(file), LOCK_UN);
            fclose(file);
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    flock(fileno(file), LOCK_UN);
    fclose(file);
    pthread_mutex_unlock(&lock);
    strcpy(buffer, "ERR: Book not found");
}

//Return Book
void return_book(char *username, char *title, char *buffer) {
    pthread_mutex_lock(&lock);

    //delay for concurrency check
    //sleep(5);

    FILE *file = fopen("book_user.txt", "r+");
    if (file == NULL) {
        perror("Error opening book_user.txt");
        pthread_mutex_unlock(&lock);
        strcpy(buffer, "ERR: Could not return book");
        return;
    }
    flock(fileno(file), LOCK_EX);

    char temp[100][150]; // To store the remaining entries after deletion
    int temp_count = 0;
    char file_title[100], file_username[50];
    bool book_found = false;
    
    while (fscanf(file, "%[^,],%s\n", file_title, file_username) != EOF) {
        if (strcmp(file_title, title) == 0 && strcmp(file_username, username) == 0) {
            book_found = true;
        } else {
            sprintf(temp[temp_count++], "%s,%s", file_title, file_username);
        }
    }

    if (!book_found) {
        strcpy(buffer, "ERR: Book not found or not borrowed by user");
        flock(fileno(file), LOCK_UN);
        fclose(file);
        pthread_mutex_unlock(&lock);
        return;
    }

    freopen("book_user.txt", "w", file);
    for (int i = 0; i < temp_count; i++) {
        fprintf(file, "%s\n", temp[i]);
    }

    for (int i = 0; i < num_books; i++) {
        if (strcmp(books[i].title, title) == 0) {
            books[i].available = true;
            break;
        }
    }
    save_books();

    flock(fileno(file), LOCK_UN);
    fclose(file);
    pthread_mutex_unlock(&lock);
    strcpy(buffer, "Book returned successfully");
}
