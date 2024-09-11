// Smain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 9278  // Define the port number for the Smain server
#define BUFFER_SIZE 1024  // Define the buffer size for data transfer
#define SM_MAIN_BASE_DIR "/home/shah5w1/dfs/smain/"  // Base directory for .c files
#define SM_PDF_BASE_DIR "/home/shah5w1/dfs/spdf/"  // Base directory for .pdf files
#define SM_TEXT_BASE_DIR "/home/shah5w1/dfs/stext/"  // Base directory for .txt files
#define SPDF_IP "127.0.0.1"  // IP address for Spdf server
#define SPDF_PORT 9224  // Port number for Spdf server
#define STEXT_IP "127.0.0.1"  // IP address for Stext server
#define STEXT_PORT 9334  // Port number for Stext server

// Recursive directory creation function
int make_directory(const char *dir) {
    struct stat st = {0};
    char tmp[BUFFER_SIZE];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (stat(tmp, &st) == -1) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    perror("Failed to create directory");
                    return -1;
                }
            }
            *p = '/';
        }
    }
    if (stat(tmp, &st) == -1) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            perror("Failed to create directory");
            return -1;
        }
    }
    return 0;
}

// Function to save a file received from the client
void save_file(int new_socket, char *filename, char *destination_path) {
    char *trimmed_filename = basename(filename);
    char *extension = strrchr(trimmed_filename, '.');

    if (!trimmed_filename || strlen(trimmed_filename) == 0) {
        perror("Received empty filename");
        send(new_socket, "ERROR: Received empty filename\n", 32, 0);
        return;
    }

    // Construct the final path based on the file type
    char full_save_path[BUFFER_SIZE];

    if (strcmp(extension, ".c") == 0) {
        snprintf(full_save_path, sizeof(full_save_path), "%s%s", SM_MAIN_BASE_DIR, destination_path);
    } else if (strcmp(extension, ".pdf") == 0) {
        snprintf(full_save_path, sizeof(full_save_path), "%s%s", SM_PDF_BASE_DIR, destination_path);
    } else if (strcmp(extension, ".txt") == 0) {
        snprintf(full_save_path, sizeof(full_save_path), "%s%s", SM_TEXT_BASE_DIR, destination_path);
    } else {
        perror("Unsupported file type");
        send(new_socket, "ERROR: Unsupported file type\n", 29, 0);
        return;
    }

    // Ensure the directory exists
    if (make_directory(full_save_path) == -1) {
        return;
    }

    // Append the filename to the path
    strcat(full_save_path, "/");
    strcat(full_save_path, trimmed_filename);

    // Save the file
    FILE *file = fopen(full_save_path, "wb");
    if (file == NULL) {
        perror("Failed to open file for writing");
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_received, file);
        if (bytes_received < BUFFER_SIZE) break;
    }

    fclose(file);
    printf("File %s received and saved at %s\n", trimmed_filename, full_save_path);

    // Forward to the remote server if needed
    if (extension != NULL) {
        if (strcmp(extension, ".pdf") == 0) {
            send_command_to_remote_server("ufile", full_save_path, trimmed_filename, SPDF_IP, SPDF_PORT);
        } else if (strcmp(extension, ".txt") == 0) {
            send_command_to_remote_server("ufile", full_save_path, trimmed_filename, STEXT_IP, STEXT_PORT);
        } else {
            printf("File %s is kept locally at %s\n", trimmed_filename, full_save_path);
        }
    }

    // Delete the local copy if it was sent to a remote server
    if (strcmp(extension, ".pdf") == 0 || strcmp(extension, ".txt") == 0) {
        remove(full_save_path);
    }
}

// Function to send a command and file to the remote server
void send_command_to_remote_server(const char *command, const char *full_save_path, const char *filename, const char *ip, int port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        printf("Failed to connect to %s:%d\n", ip, port);
        return;
    }

   // Send the command and filename to the remote server
    snprintf(buffer, sizeof(buffer), "%s:%s", command, filename);
    send(sock, buffer, strlen(buffer), 0);
    //usleep(100000); // Slight delay to ensure command is sent

    // Receive the response from the remote server
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Received from %s:%d - %s\n", ip, port, buffer);
    } else {
        perror("Failed to receive response");
    }

    close(sock);
}

// Function to send a file to the client
void send_file(int new_socket, char *filename) {
    char *trimmed_filename = basename(filename);  // Extract the base name of the file
    char *extension = strrchr(trimmed_filename, '.'); // Extract the file extension
    char file_path[BUFFER_SIZE];
    FILE *file = NULL;

    if (extension != NULL) {
        if (strcmp(extension, ".pdf") == 0) {
            snprintf(file_path, sizeof(file_path), "%s%s", SM_PDF_BASE_DIR, trimmed_filename);
        } else if (strcmp(extension, ".txt") == 0) {
            snprintf(file_path, sizeof(file_path), "%s%s", SM_TEXT_BASE_DIR, trimmed_filename);
        } else if (strcmp(extension, ".c") == 0) {
            snprintf(file_path, sizeof(file_path), "%s%s", SM_MAIN_BASE_DIR, trimmed_filename);
        } else {
            perror("Unsupported file type");
            send(new_socket, "ERROR: Unsupported file type\n", 29, 0);
            return;
        }
    } else {
        perror("No file extension found");
        send(new_socket, "ERROR: No file extension found\n", 30, 0);
        return;
    }

    file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("Failed to open file for reading");
        send(new_socket, "ERROR: File not found\n", 23, 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        send(new_socket, buffer, bytes_read, 0);
    }

    fclose(file);
    printf("File %s sent to the client from path %s\n", trimmed_filename, file_path);
}

// Function to delete file from smain or forward the request to spdf/stext
void delete_file(char *filename) {
    char *trimmed_filename = basename(filename); // Extract the base name of the file
    char *extension = strrchr(trimmed_filename, '.'); // Extract the file extension
    char full_path[BUFFER_SIZE];

    if (!extension || strlen(extension) == 0) {
        perror("Invalid file extension");
        return;
    }

    if (strcmp(extension, ".c") == 0) {
        snprintf(full_path, sizeof(full_path), "%s%s", SM_MAIN_BASE_DIR, filename);
        if (remove(full_path) == 0) {
            printf("File %s deleted locally.\n", full_path);
        } else {
            perror("Failed to delete file locally");
        }
    } else if (strcmp(extension, ".pdf") == 0) {
        send_command_to_remote_server("rmfile", filename, filename, SPDF_IP, SPDF_PORT);
    } else if (strcmp(extension, ".txt") == 0) {
        send_command_to_remote_server("rmfile", filename, filename, STEXT_IP, STEXT_PORT);
    } else {
        perror("Unsupported file type");
    }
}

// Function to create a tar file for a specific file type
void create_tar_file(const char *filetype, const char *tar_filename, const char *base_dir) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "find %s -type f -name '*%s' | tar -cvf %s -T -", base_dir, filetype, tar_filename);
    system(command);
}

// Function to send a tar file to the client
void send_tar_file(int new_socket, const char *tar_filename) {
    FILE *file = fopen(tar_filename, "rb");
    if (file == NULL) {
        perror("Failed to open tar file for sending");
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        send(new_socket, buffer, bytes_read, 0);
    }

    fclose(file);
    printf("Tar file %s sent to the client.\n", tar_filename);
}

// Function to handle dtar command
void handle_dtar(int new_socket, const char *filetype) {
    char tar_filename[BUFFER_SIZE];
    
    if (strcmp(filetype, ".c") == 0) {
        // Create tar file for .c files in Smain
        snprintf(tar_filename, sizeof(tar_filename), "cfiles.tar");
        create_tar_file(".c", tar_filename, SM_MAIN_BASE_DIR);
        send_tar_file(new_socket, tar_filename);
    } else if (strcmp(filetype, ".pdf") == 0) {
         // Request tar file for .pdf files from Spdf
        snprintf(tar_filename, sizeof(tar_filename), "pdffiles.tar");
        send_command_to_remote_server("create_tar", tar_filename, tar_filename, SPDF_IP, SPDF_PORT);
          // Receive the tar file back from Spdf and send it to the client
        send_tar_file(new_socket, tar_filename);
    } else if (strcmp(filetype, ".txt") == 0) {
         // Request tar file for .txt files from Stext
        snprintf(tar_filename, sizeof(tar_filename), "textfiles.tar");
        send_command_to_remote_server("create_tar", tar_filename, tar_filename, STEXT_IP, STEXT_PORT);
          // Receive the tar file back from Stext and send it to the client
        send_tar_file(new_socket, tar_filename);
    } else {
        perror("Unsupported file type for dtar");
        send(new_socket, "ERROR: Unsupported file type\n", 29, 0);
    }
}


// Function to handle display command
void handle_display(int new_socket, const char *pathname) {
    char command[BUFFER_SIZE];
    char file_list[BUFFER_SIZE];
    int bytes_received;
    // List .c files from Smain
    snprintf(command, sizeof(command), "find %s -type f -name '*.c'", pathname);
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        perror("Failed to get .c files");
        return;
    }

    while (fgets(file_list, sizeof(file_list), pipe) != NULL) {
        send(new_socket, file_list, strlen(file_list), 0);
    }
    pclose(pipe);

    // Fetch list of .pdf files from Spdf
    send_command_to_remote_server("list", pathname, pathname, SPDF_IP, SPDF_PORT);
     bytes_received = recv(new_socket, file_list, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        send(new_socket, file_list, bytes_received, 0);
    }
     // Fetch list of .txt files from Stext
    send_command_to_remote_server("list", pathname, pathname, STEXT_IP, STEXT_PORT);
    bytes_received = recv(new_socket, file_list, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        send(new_socket, file_list, bytes_received, 0);
    }
    }

// Function to handle client requests
void prcclient(int new_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};
    char filename[BUFFER_SIZE] = {0};
    char destination_path[BUFFER_SIZE] = {0};

    read(new_socket, buffer, BUFFER_SIZE);
    sscanf(buffer, "%[^:]:%[^:]:%s", command, filename, destination_path);

    if (strcmp(command, "ufile") == 0) {
        printf("Receiving file: '%s' with destination: '%s'\n", filename, destination_path);
        save_file(new_socket, filename, destination_path);
    } else if (strcmp(command, "dfile") == 0) {
        printf("Sending file: '%s'\n", filename);
        send_file(new_socket, filename);
    } else if (strcmp(command, "rmfile") == 0) {
        printf("Deleting file: '%s'\n", filename);
        delete_file(filename);
    } else if (strcmp(command, "dtar") == 0) {
        printf("Creating and sending tar for filetype: '%s'\n", filename);
        handle_dtar(new_socket, filename);
    } else if (strcmp(command, "display") == 0) {
        printf("Displaying file list for pathname: '%s'\n", filename);
        handle_display(new_socket, filename);
    } else {
        printf("Unknown command received.\n");
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

     // Create the socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified port and address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Create a new process to handle the client's request
        pid_t pid = fork();
        if (pid == 0) {  // Child process
            close(server_fd);  // Child doesn't need the listening socket
            prcclient(new_socket);  // Handle the client request
            close(new_socket);  // Close the connection socket in the child process
            exit(0);  // Exit the child process
        } else if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }

        close(new_socket);  // Parent doesn't need this socket
    }

    return 0;
}
