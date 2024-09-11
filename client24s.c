#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libgen.h>

#define PORT 9278 // Port number to connect to the Smain server
#define BUFFER_SIZE 1024 // Buffer size for reading/writing data
#define USERNAME "shah5w1"  // Define the username, used to create the full file path

// Function to validate the file extension
// This function checks if the filename ends with one of the allowed extensions: .c, .pdf, or .txt
int validate_file_extension(char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return 0; // No extension found or filename starts with a dot
    }
    // Check if the extension matches .c, .pdf, or .txt
    if (strcmp(dot, ".c") == 0 || strcmp(dot, ".pdf") == 0 || strcmp(dot, ".txt") == 0) {
        return 1;
    }
    return 0; // Extension is not valid
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char command[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    char destination_path[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    char full_file_path[BUFFER_SIZE];

    while (1) {
        // Prompt the user to enter a command
        printf("Enter a command: ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline character from the input

        // Handle the "ufile" command (upload file to Smain server)
        if (strncmp(command, "ufile", 5) == 0) {
            // Parse the command to extract filename and destination path
            if (sscanf(command, "ufile %s %s", filename, destination_path) != 2) {
                printf("Invalid command format. Use: ufile <filename> <destination_path>\n");
                continue;
            }

            // Prepend the path to the user's home directory to the filename
            snprintf(full_file_path, sizeof(full_file_path), "/home/%s/%s", USERNAME, filename);

            // Validate the file extension before proceeding
            if (!validate_file_extension(full_file_path)) {
                printf("Invalid file type. Only .c, .pdf, and .txt files are allowed.\n");
                continue;
            }

            // Check if the file exists
            if (access(full_file_path, F_OK) != 0) {
                perror("File does not exist at the specified path");
                continue;
            }

            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation error");
                return -1;
            }

            // Set up the server address
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("Invalid address/ Address not supported\n");
                return -1;
            }

            // Connect to the Smain server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }

            // Send the command along with the filename and destination path to the server
            snprintf(buffer, sizeof(buffer), "ufile:%s:%s", full_file_path, destination_path);
            send(sock, buffer, strlen(buffer), 0);
            usleep(100000); // Slight delay to ensure command is sent before file content

            // Open the file to be sent
            FILE *file = fopen(full_file_path, "rb");
            if (file == NULL) {
                perror("Failed to open file");
                close(sock);
                continue;
            }

            // Read from the file and send its contents to the server
            int bytes_read;
            while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
                if (send(sock, buffer, bytes_read, 0) != bytes_read) {
                    perror("Failed to send file data");
                    break;
                }
            }

            // Close the file and socket after sending the file
            fclose(file);
            close(sock);
            printf("File %s sent successfully.\n", filename);
        
        // Handle the "dfile" command (download file from Smain server)
        } else if (strncmp(command, "dfile", 5) == 0) {
            // Parse the command to extract the filename
            if (sscanf(command, "dfile %s", filename) != 1) {
                printf("Invalid command format. Use: dfile <filename>\n");
                continue;
            }

            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation error");
                return -1;
            }

            // Set up the server address
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("Invalid address/ Address not supported\n");
                return -1;
            }

            // Connect to the Smain server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }

            // Send the command and filename to the server
            snprintf(buffer, sizeof(buffer), "dfile:%s", filename);
            send(sock, buffer, strlen(buffer), 0);

            // Receive the file data from the server
            FILE *file = fopen(basename(filename), "wb");
            if (file == NULL) {
                perror("Failed to open file for writing");
                close(sock);
                continue;
            }

            // Read the data from the server and write it to the local file
            int bytes_received;
            while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
                fwrite(buffer, sizeof(char), bytes_received, file);
                if (bytes_received < BUFFER_SIZE) break; // End of file
            }

            // Close the file and socket after downloading the file
            fclose(file);
            close(sock);
            printf("File %s downloaded successfully.\n", basename(filename));
        
        // Handle the "rmfile" command (remove file from Smain server)
        } else if (strncmp(command, "rmfile", 6) == 0) {
            // Parse the command to extract the filename
            if (sscanf(command, "rmfile %s", filename) != 1) {
                printf("Invalid command format. Use: rmfile <filename>\n");
                continue;
            }

            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation error");
                return -1;
            }

            // Set up the server address
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("Invalid address/ Address not supported\n");
                return -1;
            }

            // Connect to the Smain server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }

            // Send the command and filename to the server
            snprintf(buffer, sizeof(buffer), "rmfile:%s", filename);
            send(sock, buffer, strlen(buffer), 0);

            // Close the socket after sending the request
            close(sock);
            printf("Delete request for file %s sent successfully.\n", filename);
        
        // Handle the "dtar" command (download tar file from Smain server)
        } else if (strncmp(command, "dtar", 4) == 0) {
            // Parse the command to extract filetype
            if (sscanf(command, "dtar %s", filename) != 1) {
                printf("Invalid command format. Use: dtar <filetype>\n");
                continue;
            }

            // Validate the file type for the tar command
            if (strcmp(filename, ".c") == 0 || strcmp(filename, ".pdf") == 0 || strcmp(filename, ".txt") == 0) {
                // Valid file type
            } else {
                printf("Unsupported file type for tar command: %s\n", filename);
                continue;
            }

            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation error");
                return -1;
            }

            // Set up the server address
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("Invalid address/ Address not supported\n");
                return -1;
            }

            // Connect to the Smain server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }

            // Send the command to the server
            snprintf(buffer, sizeof(buffer), "dtar:%s", filename);
            send(sock, buffer, strlen(buffer), 0);

            // Receive the tar file from the server
            FILE *file = fopen(filename, "wb");  // Save with the correct tar filename
            if (file == NULL) {
                perror("Failed to open file for writing");
                close(sock);
                continue;
            }

            // Read the tar file data from the server and write it to the local file
            int bytes_received;
            while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
                fwrite(buffer, sizeof(char), bytes_received, file);
                if (bytes_received < BUFFER_SIZE) break; // End of file
            }

            // Close the file and socket after downloading the tar file
            fclose(file);
            close(sock);
            printf("Tar file %s downloaded successfully.\n", filename);
        
        // Handle the "display" command (display file list from Smain, Spdf, and Stext)
        } else if (strncmp(command, "display", 7) == 0) {
            // Parse the command to extract the pathname
            if (sscanf(command, "display %s", filename) != 1) {
                printf("Invalid command format. Use: display <pathname>\n");
                continue;
            }

            // Create socket
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation error");
                return -1;
            }

            // Set up the server address
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("Invalid address/ Address not supported\n");
                return -1;
            }

            // Connect to the Smain server
            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("Connection Failed");
                return -1;
            }

            // Send the display command to the server
            snprintf(buffer, sizeof(buffer), "display:%s", filename);
            send(sock, buffer, strlen(buffer), 0);

            // Receive the file list from the server and display it
            printf("List of files:\n");
            int bytes_received;
            while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
                fwrite(buffer, sizeof(char), bytes_received, stdout);
                if (bytes_received < BUFFER_SIZE) break; // End of file list
            }

            // Close the socket after receiving the file list
            close(sock);
        
        // Handle unknown commands
        } else {
            printf("Unknown command. Available commands: ufile, dfile, rmfile, dtar, display\n");
        }
    }

    return 0;
}
