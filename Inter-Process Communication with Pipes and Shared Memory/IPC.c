#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define RESP_PIPE "RESP_PIPE_63196"
#define REQ_PIPE "REQ_PIPE_63196"
#define SHM_NAME "/6QyjxrMp"

void *addr = NULL;
void *file_addr = NULL;
size_t file_size = 0;

void handle_ping(int resp_fd)
{
    write(resp_fd, "PING$", 5);
    unsigned int number = 63196;
    write(resp_fd, &number, sizeof(unsigned int));
    write(resp_fd, "PONG$", 5);
}

void handle_create_shm(int resp_fd, size_t size)
{
    if (size == 0)
    {
        write(resp_fd, "CREATE_SHM$", 11);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0664);
    if (shm_fd == -1)
    {
        write(resp_fd, "CREATE_SHM$", 11);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (ftruncate(shm_fd, size) == -1)
    {
        write(resp_fd, "CREATE_SHM$", 11);
        write(resp_fd, "ERROR$", 6);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return;
    }

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED)
    {
        write(resp_fd, "CREATE_SHM$", 11);
        write(resp_fd, "ERROR$", 6);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return;
    }

    write(resp_fd, "CREATE_SHM$", 11);
    write(resp_fd, "SUCCESS$", 8);
}

void handle_write_to_shm(int resp_fd, unsigned int offset, unsigned int value)
{
    if (addr == NULL)
    {
        write(resp_fd, "WRITE_TO_SHM$", 13);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (offset + sizeof(unsigned int) > 4952664)
    {
        write(resp_fd, "WRITE_TO_SHM$", 13);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    unsigned int *shm_ptr = (unsigned int *)(addr + offset);
    *shm_ptr = value;

    write(resp_fd, "WRITE_TO_SHM$", 13);
    write(resp_fd, "SUCCESS$", 8);
}

void handle_map_file(int resp_fd, const char *file_name)
{
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        write(resp_fd, "MAP_FILE$", 9);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1)
    {
        write(resp_fd, "MAP_FILE$", 9);
        write(resp_fd, "ERROR$", 6);
        close(fd);
        return;
    }

    file_size = file_stat.st_size;
    file_addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_addr == MAP_FAILED)
    {
        write(resp_fd, "MAP_FILE$", 9);
        write(resp_fd, "ERROR$", 6);
        close(fd);
        return;
    }

    close(fd);

    write(resp_fd, "MAP_FILE$", 9);
    write(resp_fd, "SUCCESS$", 8);
}

void handle_read_from_file_offset(int resp_fd, unsigned int offset, unsigned int no_of_bytes)
{
    if (addr == NULL || file_addr == NULL)
    {
        write(resp_fd, "READ_FROM_FILE_OFFSET$", 22);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (offset + no_of_bytes > file_size)
    {
        write(resp_fd, "READ_FROM_FILE_OFFSET$", 22);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    memcpy(addr, file_addr + offset, no_of_bytes);

    write(resp_fd, "READ_FROM_FILE_OFFSET$", 22);
    write(resp_fd, "SUCCESS$", 8);
}

void handle_read_from_file_section(int resp_fd, unsigned int section_no, unsigned int offset, unsigned int no_of_bytes)
{
    if (addr == NULL || file_addr == NULL)
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (memcmp(file_addr + file_size - 2, "FH", 2) != 0)
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    unsigned short header_size;
    memcpy(&header_size, file_addr + file_size - 4, sizeof(unsigned short));

    unsigned int version;
    memcpy(&version, file_addr + file_size - header_size, sizeof(unsigned int));

    if (version < 54 || version > 164)
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    unsigned char no_of_sections;
    memcpy(&no_of_sections, file_addr + file_size - header_size + 4, sizeof(unsigned char));

    if (no_of_sections != 2 && (no_of_sections < 7 || no_of_sections > 15))
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (section_no < 1 || section_no > no_of_sections)
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    void *section_headers_addr = file_addr + file_size - header_size + 5;
    unsigned int section_header_offset = (section_no - 1) * (8 + 1 + 4 + 4);
    unsigned int section_offset;
    unsigned int section_size;
    memcpy(&section_offset, section_headers_addr + section_header_offset + 8 + 1, sizeof(unsigned int));
    memcpy(&section_size, section_headers_addr + section_header_offset + 8 + 1 + 4, sizeof(unsigned int));
    if (offset + no_of_bytes > section_size)
    {
        write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    memcpy(addr, file_addr + section_offset + offset, no_of_bytes);

    write(resp_fd, "READ_FROM_FILE_SECTION$", 23);
    write(resp_fd, "SUCCESS$", 8);
}

void handle_read_from_logical_space_offset(int resp_fd, unsigned int logical_offset, unsigned int no_of_bytes)
{
    if (addr == NULL || file_addr == NULL)
    {
        write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    if (memcmp(file_addr + file_size - 2, "FH", 2) != 0)
    {
        write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    unsigned short header_size;
    memcpy(&header_size, file_addr + file_size - 4, sizeof(unsigned short));

    unsigned int version;
    memcpy(&version, file_addr + file_size - header_size, sizeof(unsigned int));

    if (version < 54 || version > 164)
    {
        write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    unsigned char no_of_sections;
    memcpy(&no_of_sections, file_addr + file_size - header_size + 4, sizeof(unsigned char));

    if (no_of_sections != 2 && (no_of_sections < 7 || no_of_sections > 15))
    {
        write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
        write(resp_fd, "ERROR$", 6);
        return;
    }

    void *section_headers_addr = file_addr + file_size - header_size + 5;

    unsigned int logical_base = 0;
    for (unsigned char i = 0; i < no_of_sections; i++)
    {
        unsigned int section_offset;
        unsigned int section_size;

        memcpy(&section_offset, section_headers_addr + i * (8 + 1 + 4 + 4) + 8 + 1, sizeof(unsigned int));
        memcpy(&section_size, section_headers_addr + i * (8 + 1 + 4 + 4) + 8 + 1 + 4, sizeof(unsigned int));

        if (logical_offset >= logical_base && logical_offset < logical_base + section_size)
        {
            unsigned int file_offset = section_offset + (logical_offset - logical_base);

            if (file_offset + no_of_bytes > file_size)
            {
                write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
                write(resp_fd, "ERROR$", 6);
                return;
            }

            memcpy(addr, file_addr + file_offset, no_of_bytes);

            write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
            write(resp_fd, "SUCCESS$", 8);
            return;
        }

        logical_base = (logical_base + section_size + 2047) & ~2047;
    }

    write(resp_fd, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31);
    write(resp_fd, "ERROR$", 6);
}

unsigned int read_unsigned_int(int fd)
{
    unsigned int value;
    read(fd, &value, sizeof(unsigned int));
    return value;
}

void read_string_field(int fd, char *buffer, size_t max_len)
{
    char ch;
    size_t i = 0;
    while (i < max_len - 1)
    {
        read(fd, &ch, 1);
        if (ch == '$')
        {
            break;
        }
        buffer[i++] = ch;
    }
    buffer[i] = '\0';
}

int main()
{
    if (mkfifo(RESP_PIPE, 0666) == -1)
    {
        printf("ERROR\n");
        printf("cannot create the response pipe\n");
        return 1;
    }

    int req_fd = open(REQ_PIPE, O_RDONLY);
    if (req_fd == -1)
    {
        printf("ERROR\n");
        printf("cannot open the request pipe\n");
        unlink(RESP_PIPE);
        return 1;
    }

    int resp_fd = open(RESP_PIPE, O_WRONLY);
    if (resp_fd == -1)
    {
        printf("ERROR\n");
        printf("cannot open the response pipe\n");
        close(req_fd);
        unlink(RESP_PIPE);
        return 1;
    }

    if (write(resp_fd, "CONNECT$", 8) == -1)
    {
        printf("ERROR\n");
        printf("cannot write to the response pipe\n");
        close(req_fd);
        close(resp_fd);
        unlink(RESP_PIPE);
        return 1;
    }
    printf("SUCCESS\n");
    char command[256];
    while (1)
    {
        read_string_field(req_fd, command, sizeof(command));
        if (strcmp(command, "PING") == 0)
        {
            handle_ping(resp_fd);
        }
        else if (strcmp(command, "CREATE_SHM") == 0)
        {
            unsigned int size = read_unsigned_int(req_fd);
            handle_create_shm(resp_fd, size);
        }
        else if (strcmp(command, "WRITE_TO_SHM") == 0)
        {
            unsigned int offset = read_unsigned_int(req_fd);
            unsigned int value = read_unsigned_int(req_fd);
            handle_write_to_shm(resp_fd, offset, value);
        }
        else if (strcmp(command, "MAP_FILE") == 0)
        {
            char file_name[256];
            read_string_field(req_fd, file_name, sizeof(file_name));
            handle_map_file(resp_fd, file_name);
        }
        else if (strcmp(command, "READ_FROM_FILE_OFFSET") == 0)
        {
            unsigned int offset = read_unsigned_int(req_fd);
            unsigned int no_of_bytes = read_unsigned_int(req_fd);
            handle_read_from_file_offset(resp_fd, offset, no_of_bytes);
        }
        else if (strcmp(command, "READ_FROM_FILE_SECTION") == 0)
        {
            unsigned int section_no = read_unsigned_int(req_fd);
            unsigned int offset = read_unsigned_int(req_fd);
            unsigned int no_of_bytes = read_unsigned_int(req_fd);
            handle_read_from_file_section(resp_fd, section_no, offset, no_of_bytes);
        }
        else if (strcmp(command, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
        {
            unsigned int logical_offset = read_unsigned_int(req_fd);
            unsigned int no_of_bytes = read_unsigned_int(req_fd);
            handle_read_from_logical_space_offset(resp_fd, logical_offset, no_of_bytes);
        }
        else if (strcmp(command, "EXIT") == 0)
        {
            break;
        }
    }

    if (addr != NULL)
    {
        munmap(addr, 4952664);
    }

    if (file_addr != NULL)
    {
        munmap(file_addr, file_size);
    }

    close(req_fd);
    close(resp_fd);
    unlink(RESP_PIPE);

    return 0;
}
