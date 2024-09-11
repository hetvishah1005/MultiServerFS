
# Distributed File System

## Project Overview
This project implements a **Distributed File System** using **C** and **socket programming**. The system consists of three servers: **Smain**, **Spdf**, and **Stext**. It supports multiple client connections, allowing clients to upload, download, and manage files transparently. The **Smain** server handles communication and distributes specific file types to **Spdf** and **Stext** in the background.

## Features
- **Upload**: `.c` files (stored in Smain), `.txt` files (stored in Stext), and `.pdf` files (stored in Spdf).
- **Download**: Retrieve any file via Smain, with Smain fetching files from Stext/Spdf in the background.
- **Delete**: Remove files from the respective servers.
- **File Listing**: View available files in specified directories.
- **Archive Creation**: Generate tar files for `.c`, `.txt`, or `.pdf` files.

## Project Components
1. **Smain.c**: The main server that handles client interactions.
2. **Spdf.c**: A server that manages `.pdf` files.
3. **Stext.c**: A server that manages `.txt` files.
4. **client24s.c**: Client-side program for interacting with the servers.

## Prerequisites
- Basic knowledge of **socket programming** in C.
- Linux environment.

## Installation

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/hetvishah1005/MultiServerFS.git
   cd MultiServerFS
   ```

## Running the Project
1. **Compile** the `.c` files on different machines or terminals.
   ```bash
   gcc Smain.c -o Smain
   gcc Spdf.c -o Spdf
   gcc Stext.c -o Stext
   gcc client24s.c -o client24s
   ```

2. **Run** the servers and the client on different terminals:
   ```bash
   ./Smain
   ./Spdf
   ./Stext
   ./client24s
   ```

   The client can now send commands to **Smain** and interact with the system.

## Usage

### Commands

#### `ufile filename destination_path`
Upload a file to the server.
- **Description:** Uploads the file specified by `filename` to the `destination_path` on the server.
- **Example:**
  ```bash
  client24s$ ufile sample.c ~smain/folder
  ```

#### `dfile filename`
Download a file from the server.
- **Description:** Downloads the file specified by `filename` from the server to your local machine.
- **Example:**
  ```bash
  client24s$ dfile ~smain/folder/sample.txt
  ```

#### `rmfile filename`
Delete a file from the server.
- **Description:** Deletes the file specified by `filename` from the server.
- **Example:**
  ```bash
  client24s$ rmfile ~smain/folder/sample.pdf
  ```

#### `dtar filetype`
Create a tar archive of files of a specific type.
- **Description:** Creates a tar archive of all files matching the specified `filetype` (e.g., `.c`, `.txt`, `.pdf`) on the server.
- **Example:**
  ```bash
  client24s$ dtar .c
  ```

#### `display pathname`
List files in a specified directory.
- **Description:** Lists all files and directories under the specified `pathname` on the server.
- **Example:**
  ```bash
  client24s$ display ~smain/folder
  ```

This will list `.c`, `.pdf`, and `.txt` files in the specified directory.

## Error Handling
- The system provides appropriate error messages for invalid commands, incorrect paths, network issues, or failed file operations.
- Error messages guide users to correct their actions.

## Authors
- Hetvi Shah (Team Member 1)
- Naiya Jani (Team Member 2)

## License
Distributed under the MIT License.
