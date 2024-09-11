#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 9334  // Define the port number for the Stext server
#define BUFFER_SIZE 1024  // Define the buffer size for reading and writing data
#define BASE_SAVE_DIR "/home/shah5w1/dfs/stext/"  // Base directory for saving .txt files on the server

// Function to create directories recursively
// It ensures that the full path specified by 'dir' exists by creating any missing directories
int make_directory(const char *dir) {
    struct stat st = {0};  // Structure to get directory status
    if (stat(dir, &st) == -1) {  // Check if the directory exists
        // If the directory doesn't exist, create it with permissions 0755
        if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
            perror("Failed to create directory");  // Print error if directory creation fails
            return -1;
        }
    }
    return 0;  // Return 0 if the directory exists or was successfully created
}

// Function to save a file received from the Smain server
void save_file(int new_socket, char *filename, char *destination_path) {
    char *base_filename = basename(filename);  // Extract the base name of the file

    // Construct the directory path where the file will be saved
    char save_dir[BUFFER_SIZE];
    snprintf(save_dir, sizeof(save_dir), "%s%s", BASE_SAVE_DIR, destination_path);

    // Ensure that the directory exists
    if (make_directory(save_dir) == -1) return;

    // Construct the full path where the file will be saved
    char save_path[BUFFER_SIZE];
    snprintf(save_path, sizeof(save_path), "%s/%s", save_dir, base_filename);

    // Open the file for writing in binary mode
    FILE *file = fopen(save_path, "wb");
    if (file == NULL) {
        perror("Failed to open file");  // Print error if the file couldn't be opened
        return;
    }

    // Buffer to store data received from the socket
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int total_bytes_received = 0;  // Track the total number of bytes received

    // Receive data from the socket and write it to the file
    while ((bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_received, file);
        total_bytes_received += bytes_received;
        if (bytes_received < BUFFER_SIZE) break;  // Stop reading if less than BUFFER_SIZE bytes are received
    }

    // Close the file after writing is complete
    fclose(file);

    // Check if any data was received and saved
    if (total_bytes_received > 0) {
        printf("File received and saved as %s, total bytes received: %d\n", save_path, total_bytes_received);
    } else {
        printf("No data received for file %s\n", save_path);  // Print a message if no data was received
    }
}

// Function to delete a file requested by Smain
void delete_file(char *filename, char *destination_path) {
    char *base_filename = basename(filename);  // Extract the base name of the file

    // Construct the full path to the file that needs to be deleted
    char delete_path[BUFFER_SIZE];
    snprintf(delete_path, sizeof(delete_path), "%s%s/%s", BASE_SAVE_DIR, destination_path, base_filename);

    // Attempt to delete the file
    if (remove(delete_path) == 0) {
        printf("File %s deleted successfully.\n", delete_path);
    } else {
        perror("Failed to delete file");  // Print error if the file couldn't be deleted
    }
}

// Function to create a tar file for .txt files
void create_tar_file(const char *tar_filename) {
    char command[BUFFER_SIZE];
    // Construct the command to find all .txt files and archive them into a tar file
    snprintf(command, sizeof(command), "find %s -type f -name '*.txt' | tar -cvf %s -T -", BASE_SAVE_DIR, tar_filename);
    system(command);  // Execute the command
}

// Function to send the tar file to Smain
void send_tar_file(int new_socket, const char *tar_filename) {
    // Open the tar file for reading in binary mode
    FILE *file = fopen(tar_filename, "rb");
    if (file == NULL) {
        perror("Failed to open tar file for sending");  // Print error if the tar file couldn't be opened
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read from the tar file and send its contents over the socket
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        send(new_socket, buffer, bytes_read, 0);
    }

    // Close the file after sending is complete
    fclose(file);
    printf("Tar file %s sent to Smain.\n", tar_filename);  // Print a message indicating the tar file was sent
}

// Function to list all .txt files and send the list to Smain
void list_files(int new_socket) {
    char command[BUFFER_SIZE];
    char file_list[BUFFER_SIZE] = "";

    // Construct the command to find all .txt files in the base directory
    snprintf(command, sizeof(command), "find %s -type f -name '*.txt'", BASE_SAVE_DIR);

    // Open a pipe to execute the command and read the output
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        perror("Failed to list files");  // Print error if the command couldn't be executed
        return;
    }

    // Read the list of files and send it over the socket
    while (fgets(file_list, sizeof(file_list), pipe) != NULL) {
        send(new_socket, file_list, strlen(file_list), 0);
    }
    pclose(pipe);  // Close the pipe after reading the file list
}

// Main function that sets up the server and handles incoming connections
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};
    char filename[BUFFER_SIZE] = {0};
    char destination_path[BUFFER_SIZE] = {0};

    // Create a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");  // Print error if the socket couldn't be created
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of the address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");  // Print error if setting the socket option failed
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");  // Print error if binding the socket failed
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");  // Print error if listening for connections failed
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept an incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");  // Print error if accepting a connection failed
            exit(EXIT_FAILURE);
        }

        // Read the command sent by Smain
        read(new_socket, buffer, BUFFER_SIZE);
        sscanf(buffer, "%[^:]:%[^:]:%s", command, filename, destination_path);  // Parse the command, filename, and destination path
        printf("Received command: '%s' for file: '%s' with destination: '%s'\n", command, filename, destination_path);

        // Process the command received from Smain
        if (strcmp(command, "ufile") == 0) {
            // Save the file received from Smain
            save_file(new_socket, filename, destination_path);
        } else if (strcmp(command, "rmfile") == 0) {
            // Delete the specified file
            delete_file(filename, destination_path);
        } else if (strcmp(command, "create_tar") == 0) {
            // Create and send the tar file for .txt files
            create_tar_file(filename);
            send_tar_file(new_socket, filename);
        } else if (strcmp(command, "list") == 0) {
            // List all .txt files and send the list to Smain
            list_files(new_socket);
        } else {
            printf("Unknown command received.\n");  // Print a message if an unknown command is received
        }

        // Close the socket after processing the command
        close(new_socket);
    }

    // Close the server socket
    close(server_fd);  

    return 0;  // Return 0 to indicate successful execution
}
