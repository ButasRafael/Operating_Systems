# Operating_Systems


This repository contains three Operating Systems assignments designed to explore essential OS concepts, including file operations, thread synchronization, and inter-process communication (IPC) through shared memory and pipes.

## Contents

1. [Files Management]
2. [Thread and Process Synchronization]
3. [Inter-Process Communication with Pipes and Shared Memory]

---

### SF Files Management

**Objective**: Implement a program that lists and parses SF (Special File) files, extracting specific sections based on defined parameters.

**Features**:
- **List Files**: Recursively list files in a directory with specified name patterns and permissions.
- **Parse SF Files**: Verify file structure, check for valid "magic numbers" and version constraints, and extract section details.
- **Extract Section Content**: Display content of specified sections based on line numbers and offsets.

### Thread and Process Synchronization
**Objective**: Develop a multi-threaded application simulating synchronized operations between threads using mutexes, condition variables, and semaphores for process communication.

**Features**:
- **Thread Synchronization**: Implement critical sections with pthread_mutex_t and pthread_cond_t.
- **Semaphore Coordination**: Use semaphores to ensure specific threads execute in a designated order.
- **Process Communication**: Fork multiple child processes and synchronize threads within them.
- **Example**: The program demonstrates handling up to 43 threads, each with unique conditions for execution. Shared resources are guarded using mutexes and condition variables, while semaphores coordinate between distinct threads.

### Inter-Process Communication with Pipes and Shared Memory
**Objective**: Implement IPC mechanisms using named pipes and shared memory for a server-client style communication model where commands are issued through pipes.

**Features**:
- **Named Pipes (FIFOs)**: Create REQ_PIPE for client requests and RESP_PIPE for server responses.
- **Shared Memory Creation**: Allocate and map shared memory regions for inter-process data sharing.
- **File Mapping and Reading**: Map external files into memory, read specific sections, and copy data to shared memory.
- **Command Handling**: Commands include creating shared memory, reading from files by offset or section, and writing data.

**Supported Commands**:
* PING: Responds with a confirmation message.
* CREATE_SHM <size>: Creates a shared memory region of the specified size.
* WRITE_TO_SHM <offset> <value>: Writes a value at a specified offset in shared memory.
* MAP_FILE <filename>: Maps a file into memory.
* READ_FROM_FILE_OFFSET <offset> <bytes>: Reads specified bytes from the file offset and copies to shared memory.
* EXIT: Closes connections and cleans up resources.

## Setup and Compilation
### Prerequisites
* GCC for C compilation
* compliant environment (Linux recommended for compatibility)
* Make (optional for easier compilation)
### Compilation
* Clone the repository:
```
git clone https://github.com/ButasRafael/Operating_Systems.git
cd Operating_Systems
```
* Compile all assignments:
```
make
```
* Each assignment can be run separately.

* To remove all compiled binaries:
```
make clean
```



