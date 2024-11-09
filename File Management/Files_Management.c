#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "FH"
#define MIN_VERSION 54u
#define MAX_VERSION 164u
#define MIN_SECTIONS 2u
#define MAX_SECTIONS 15u
#define VALID_SECTION_TYPE1 39u
#define VALID_SECTION_TYPE2 36u
#define VALID_SECTION_TYPE3 35u
#define VALID_SECTION_TYPE4 57u
#define VALID_SECTION_TYPE5 43u
#define MAX_PATH_LEN 1024

typedef struct
{
    char name[256];
    unsigned int type;
    unsigned int offset;
    unsigned int size;
} section;

void display_variant()
{
    printf("63196\n");
}
void list(const char *path, int recursive, const char *name_starts_with, int has_perm_write, int ok)
{
    DIR *dir;
    struct dirent *entry;
    struct stat filestat;
    char full_path[MAX_PATH_LEN];

    if ((dir = opendir(path)) == NULL)
    {
        printf("ERROR\n");
        printf("invalid directory path\n");
        return;
    }

    if (ok == 1)
    {
        printf("SUCCESS\n");
    }
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (lstat(full_path, &filestat) < 0)
        {
            continue;
        }

        if (name_starts_with && strncmp(entry->d_name, name_starts_with, strlen(name_starts_with)) != 0)
        {
            continue;
        }

        if (has_perm_write && !(filestat.st_mode & S_IWUSR))
        {
            continue;
        }

        printf("%s\n", full_path);

        if (recursive && S_ISDIR(filestat.st_mode))
        {
            list(full_path, recursive, name_starts_with, has_perm_write, 0);
        }
    }

    closedir(dir);
}

void parse(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("ERROR\n");
        printf("Failed to open file\n");
        close(fd);
        return;
    }
    char magic[3];
    unsigned int header_size = 0;
    lseek(fd, -4, SEEK_END);
    read(fd, &header_size, 2);
    read(fd, magic, 2);
    int header_to_int = 0 - (int)header_size;
    lseek(fd, header_to_int, SEEK_END);
    magic[2] = '\0';
    if (strcmp(magic, MAGIC) != 0)
    {
        printf("ERROR\n");
        printf("wrong magic\n");
        close(fd);
        return;
    }
    unsigned int version = 0;
    if (read(fd, &version, 4) != 4)
    {
        printf("ERROR\n");
        printf("Failed to read version\n");
        close(fd);
        return;
    }
    if (version < MIN_VERSION || version > MAX_VERSION)
    {
        printf("ERROR\n");
        printf("wrong version\n");
        close(fd);
        return;
    }
    unsigned int no_of_sections = 0;
    if (read(fd, &no_of_sections, 1) != 1)
    {
        printf("ERROR\n");
        printf("Failed to read number of sections\n");
        close(fd);
        return;
    }
    if (no_of_sections != 2 && (no_of_sections < 7 || no_of_sections > 15))
    {
        printf("ERROR\n");
        printf("wrong sect_nr\n");
        close(fd);
        return;
    }
    section *sect = (section *)malloc(sizeof(section) * no_of_sections);
    for (int i = 0; i < no_of_sections; i++)
    {
        if (read(fd, sect[i].name, 8) != 8 || read(fd, &sect[i].type, 1) != 1 || read(fd, &sect[i].offset, 4) != 4 || read(fd, &sect[i].size, 4) != 4)
        {
            printf("ERROR\n");
            printf("Failed to read information about sections\n");
            free(sect);
            close(fd);
            return;
        }
        else if (sect[i].type != VALID_SECTION_TYPE1 && sect[i].type != VALID_SECTION_TYPE2 && sect[i].type != VALID_SECTION_TYPE3 && sect[i].type != VALID_SECTION_TYPE4 && sect[i].type != VALID_SECTION_TYPE5)
        {
            printf("ERROR\n");
            printf("wrong sect_types\n");
            free(sect);
            close(fd);
            return;
        }
    }

    printf("SUCCESS\n");
    printf("version=%u\n", version);
    printf("nr_sections=%u\n", no_of_sections);
    for (int i = 0; i < no_of_sections; i++)
    {
        printf("section%d: %s %u %u\n", i + 1, sect[i].name, sect[i].type, sect[i].size);
    }

    free(sect);
    close(fd);
}

void extract(const char *path, int section_num, int line_num)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("ERROR\n");
        printf("Failed to open file\n");
        return;
    }

    char magic[3];
    unsigned int header_size = 0;
    lseek(fd, -4, SEEK_END);
    read(fd, &header_size, 2);
    read(fd, magic, 2);
    int header_to_int = 0 - (int)header_size;
    lseek(fd, header_to_int, SEEK_END);
    magic[2] = '\0';
    if (strcmp(magic, MAGIC) != 0)
    {
        printf("ERROR\n");
        printf("invalid file\n");
        close(fd);
        return;
    }

    unsigned int version = 0;
    if (read(fd, &version, 4) != 4)
    {
        printf("ERROR\n");
        printf("Failed to read version\n");
        close(fd);
        return;
    }

    if (version < MIN_VERSION || version > MAX_VERSION)
    {
        printf("ERROR\n");
        printf("invalid file\n");
        close(fd);
        return;
    }

    unsigned int no_of_sections = 0;
    if (read(fd, &no_of_sections, 1) != 1)
    {
        printf("ERROR\n");
        printf("Failed to read number of sections\n");
        close(fd);
        return;
    }

    if (no_of_sections != 2 && (no_of_sections < 7 || no_of_sections > 15))
    {
        printf("ERROR\n");
        printf("invalid file\n");
        close(fd);
        return;
    }

    section *sect = (section *)malloc(sizeof(section) * no_of_sections);
    if (sect == NULL)
    {
        printf("ERROR\n");
        printf("Memory allocation failed\n");
        close(fd);
        return;
    }

    for (int i = 0; i < no_of_sections; i++)
    {
        if (read(fd, sect[i].name, 8) != 8 || read(fd, &sect[i].type, 1) != 1 || read(fd, &sect[i].offset, 4) != 4 || read(fd, &sect[i].size, 4) != 4)
        {
            printf("ERROR\n");
            printf("Failed to read information about sections\n");
            free(sect);
            close(fd);
            return;
        }
        else if (sect[i].type != VALID_SECTION_TYPE1 && sect[i].type != VALID_SECTION_TYPE2 && sect[i].type != VALID_SECTION_TYPE3 && sect[i].type != VALID_SECTION_TYPE4 && sect[i].type != VALID_SECTION_TYPE5)
        {
            printf("ERROR\n");
            printf("invalid file\n");
            free(sect);
            close(fd);
            return;
        }
    }

    if (section_num < 1 || section_num > no_of_sections)
    {
        printf("ERROR\n");
        printf("invalid section\n");
        free(sect);
        close(fd);
        return;
    }

    unsigned int sect_offset = sect[section_num - 1].offset;
    unsigned int sect_size = sect[section_num - 1].size;
    char *section_content = (char *)malloc(sect_size);
    if (section_content == NULL)
    {
        printf("ERROR\n");
        printf("Memory allocation failed\n");
        free(sect);
        close(fd);
        return;
    }

    lseek(fd, sect_offset, SEEK_SET);
    if (read(fd, section_content, sect_size) != sect_size)
    {
        printf("ERROR\n");
        printf("Failed to read section content\n");
        free(sect);
        free(section_content);
        close(fd);
        return;
    }

    int current_line = 1;
    for (int i = 0; i < sect_size; i++)
    {
        if (section_content[i] == '\n')
        {
            current_line++;
        }
        if (current_line == line_num)
        {
            if (i != 0)
                i++;
            printf("SUCCESS\n");
            for (int j = i; j < sect_size; j++)
            {
                if (section_content[j] != '\r')
                {
                    printf("%c", section_content[j]);
                }
                else
                {
                    printf("\n");
                    free(sect);
                    free(section_content);
                    close(fd);
                    return;
                }
            }
        }
    }
    printf("ERROR\n");
    printf("Invalid line\n");
    free(sect);
    free(section_content);
    close(fd);
}

void findall(const char *path, int ok)
{
    DIR *dir;
    struct dirent *entry;
    struct stat filestat;
    char full_path[MAX_PATH_LEN];
    if ((dir = opendir(path)) == NULL)
    {
        printf("ERROR\n");
        printf("invalid directory path\n");
        return;
    }

    if (ok == 1)
    {
        printf("SUCCESS\n");
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (lstat(full_path, &filestat) < 0)
        {
            continue;
        }
        if (S_ISDIR(filestat.st_mode))
        {
            findall(full_path, 0);
        }
        else if (S_ISREG(filestat.st_mode))
        {
            int fd = open(full_path, O_RDONLY);
            if (fd == -1)
            {
                close(fd);
                continue;
            }
            char magic[3];
            unsigned int header_size = 0;
            lseek(fd, -4, SEEK_END);
            read(fd, &header_size, 2);
            read(fd, magic, 2);
            int header_to_int = 0 - (int)header_size;
            lseek(fd, header_to_int, SEEK_END);
            magic[2] = '\0';
            if (strcmp(magic, MAGIC) != 0)
            {
                close(fd);
                continue;
            }
            unsigned int version = 0;
            if (read(fd, &version, 4) != 4)
            {
                close(fd);
                continue;
            }
            if (version < MIN_VERSION || version > MAX_VERSION)
            {
                close(fd);
                continue;
            }
            unsigned int no_of_sections = 0;
            if (read(fd, &no_of_sections, 1) != 1)
            {
                close(fd);
                continue;
            }
            if (no_of_sections != 2 && (no_of_sections < 7 || no_of_sections > 15))
            {
                close(fd);
                continue;
            }
            section *sect = (section *)malloc(sizeof(section) * no_of_sections);
            int count = 0;
            for (int i = 0; i < no_of_sections; i++)
            {
                if (read(fd, sect[i].name, 8) != 8 || read(fd, &sect[i].type, 1) != 1 || read(fd, &sect[i].offset, 4) != 4 || read(fd, &sect[i].size, 4) != 4)
                {
                    free(sect);
                    close(fd);
                    continue;
                }
                else if (sect[i].type != VALID_SECTION_TYPE1 && sect[i].type != VALID_SECTION_TYPE2 && sect[i].type != VALID_SECTION_TYPE3 && sect[i].type != VALID_SECTION_TYPE4 && sect[i].type != VALID_SECTION_TYPE5)
                {
                    free(sect);
                    close(fd);
                    continue;
                }
                if (sect[i].type == VALID_SECTION_TYPE4)
                {
                    count++;
                }
            }
            if (count >= 2)
            {
                printf("%s\n", full_path);
            }
            free(sect);
            close(fd);
        }
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            display_variant();
        }
        else if (strcmp(argv[1], "list") == 0)
        {
            const char *path = NULL;
            int recursive = 0;
            const char *name_starts_with = NULL;
            int has_perm_write = 0;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    path = argv[i] + 5;
                }
                else if (strcmp(argv[i], "recursive") == 0)
                {
                    recursive = 1;
                }
                else if (strncmp(argv[i], "name_starts_with=", 17) == 0)
                {
                    name_starts_with = argv[i] + 17;
                }
                else if (strcmp(argv[i], "has_perm_write") == 0)
                {
                    has_perm_write = 1;
                }
            }

            if (!path)
            {
                printf("ERROR\n");
                printf("Path argument is missing\n");
                return 1;
            }
            list(path, recursive, name_starts_with, has_perm_write, 1);
        }
        else if (strcmp(argv[1], "parse") == 0)
        {
            const char *path = NULL;
            if (strncmp(argv[2], "path=", 5) == 0)
            {
                path = argv[2] + 5;
            }
            if (!path)
            {
                printf("ERROR\n");
                printf("Path argument is missing\n");
                return 1;
            }
            parse(path);
        }
        else if (strcmp(argv[1], "extract") == 0)
        {
            const char *path = NULL;
            int section_num = 0;
            int line_num = 0;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    path = argv[i] + 5;
                }
                else if (strncmp(argv[i], "section=", 8) == 0)
                {
                    section_num = atoi(argv[i] + 8);
                }
                else if (strncmp(argv[i], "line=", 5) == 0)
                {
                    line_num = atoi(argv[i] + 5);
                }
            }
            if (!path || section_num <= 0 || line_num <= 0)
            {
                printf("ERROR\n");
                printf("Path argument is missing or Invalid section|line\n");
                return 1;
            }
            extract(path, section_num, line_num);
        }
        else if (strcmp(argv[1], "findall") == 0)
        {
            const char *path = NULL;
            if (strncmp(argv[2], "path=", 5) == 0)
            {
                path = argv[2] + 5;
            }
            if (!path)
            {
                printf("ERROR\n");
                printf("Path argument is missing\n");
                return 1;
            }
            findall(path, 1);
        }
    }
    return 0;
}