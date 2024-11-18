#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to the server");
        exit(EXIT_FAILURE);
    }

    // Communicate with the server (e.g., send and receive data)
    while (1) {
        // Display options to the user
        printf("\nOptions:\n");
        printf("0. Register\n");
        printf("1. Login\n");
        printf("2. Add book\n");
        printf("3. Delete book\n");
        printf("4. Modify book\n");
        printf("5. Search book\n");
        printf("6. Borrow book\n");
        printf("7. Return book\n");
        printf("8. Quit\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        switch (choice) {
            case 0: {
                // User registration
                char username[50];
                char password[50];
                char flag[1];
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                printf("Enter 1 for admin and 0 for user: ");
                scanf("%s", flag);
                sprintf(buffer, "R%s %s %s", username, password, flag);
                break;
            }
            case 1: {
                // User login
                char username[50];
                char password[50];
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                sprintf(buffer, "A%s %s", username, password);
                break;
            }
            case 2: {
                // Add a new book
                char title[100];
                char author[100];
                char isbn[20];
                printf("Enter book title: ");
                scanf("%s", title);
                printf("Enter book author: ");
                scanf("%s", author);
                printf("Enter book ISBN: ");
                scanf("%s", isbn);
                sprintf(buffer, "C%s %s %s", title, author, isbn);
                break;
            }
            case 3: {
                // Delete a book
                char title[100];
                printf("Enter title of book to delete: ");
                scanf("%s", title);
                sprintf(buffer, "D%s", title);
                break;
            }
            case 4: {
                // Modify a book
                char title[100];
                char author[100];
                char isbn[20];
                printf("Enter title of book to modify: ");
                scanf("%s", title);
                printf("Enter new author: ");
                scanf("%s", author);
                printf("Enter new ISBN: ");
                scanf("%s", isbn);
                sprintf(buffer, "E%s %s %s", title, author, isbn);
                break;
            }
            case 5: {
                // Search for a book
                char title[100];
                printf("Enter title of book to search: ");
                scanf("%s", title);
                sprintf(buffer, "F%s", title);
                break;
            }
            case 6: {
                // Borrow book
                char title[100];
                printf("Enter title of book to borrow: ");
                scanf("%s", title);
                sprintf(buffer, "B%s", title);
                break;
            }
            case 7: {
                // Return book
                char title[100];
                printf("Enter title of book to return: ");
                scanf("%s", title);
                sprintf(buffer, "U%s", title);
                break;
            }
            case 8:
                // Quit the program
                sprintf(buffer, "QUIT");
                if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
                    perror("Error sending data to server");
                    break;
                }
                close(sockfd);
                return 0;
            default:
                // Handle invalid choices
                printf("Invalid choice. Please try again.\n");
                continue;
        }

        // Send the request to the server
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Error sending data to server");
            break;
        }

        // Receive the response from the server
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("Error receiving data from server");
            break;
        }
        buffer[bytes_received] = '\0';

        // Print the server's response
        printf("\nServer response:\n%s\n", buffer);
    }

    // Close the socket
    close(sockfd);

    return 0;
}
