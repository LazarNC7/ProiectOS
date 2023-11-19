#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/wait.h>

typedef struct {
    char signature[2];
    int32_t size;
    int32_t reserved;
    int32_t data_offset;
    int32_t header_size;
    int32_t width;
    int32_t height;
    int16_t planes;
    int16_t bcnt;
    int32_t compression;
    int32_t image_size;
    int32_t x_resolution;
    int32_t y_resolution;
    int32_t num_colors;
    int32_t important_colors;
} BMPHeader;

int isBMPFile(const BMPHeader *header) {
    return (header->signature[0] == 'B' && header->signature[1] == 'M');
}

int isBMPFilePath(const char *file_path) {
    int inp;
    BMPHeader header;

    if ((inp = open(file_path, O_RDONLY)) == -1) {
        perror("Error opening input file");
        exit(2);
    }

    if (read(inp, &header, sizeof(BMPHeader)) != sizeof(BMPHeader) || 
        header.signature[0] != 'B' || header.signature[1] != 'M') {
        close(inp);
        return 0; 
    }

    close(inp);
    return 1; 
}

int isDirectory(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISDIR(info.st_mode);
}

int isRegularFile(const char *path) {
    struct stat info;
    if (lstat(path, &info) != 0) {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISREG(info.st_mode) && !S_ISLNK(info.st_mode);
}

int isNonBMPFile(const char *path) {
    if (isRegularFile(path)) {
        size_t path_len = strlen(path);
        return (path_len < 4 || (strcmp(path + path_len - 4, ".bmp") != 0));
    }
    return 0;
}

int isSymbolicLink(const char *path) {
    struct stat info;
    if (lstat(path, &info) != 0) {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISLNK(info.st_mode);
}

void statistics(const char *file) {
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(file, O_RDONLY)) == -1) {
        perror("Error opening input file");
        exit(2);
    }

    if (fstat(inp, &info) == -1) {
        perror("Error getting file information");
        close(inp);
        exit(2);
    }

    BMPHeader header;
    if (read(inp, &header, sizeof(BMPHeader)) != sizeof(BMPHeader) || !isBMPFile(&header)) {
        fprintf(stderr, "The input file is not a BMP file.\n");
        exit(5);
    }

    

    int width = *(int*)((char*)&header + 18);
    int height = *(int*)((char*)&header + 22);

    snprintf(statistics, sizeof(statistics),
        "File name: %s\nHeight: %d\nWidth: %d\nSize: %d\nUser ID: %d\nLast modified time: %sLink count: %ld\nUser access rights: RWX\nGroup access rights: R--\nOthers access rights: ---\n",
        file, height, width, (int)info.st_size, info.st_uid, ctime(&info.st_mtime), info.st_nlink);

    out = open("statistica.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1) {
        perror("Error opening output file");
        close(inp);
        exit(3);
    }

    if (write(out, statistics, strlen(statistics)) == -1) {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void calculateStatistics(const char *input_file, const char *output_dir) {
    
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(input_file, O_RDONLY)) == -1) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    if (fstat(inp, &info) == -1) {
        perror("Error getting file information");
        close(inp);
        exit(EXIT_FAILURE);
    }

    BMPHeader header;
    if (read(inp, &header, sizeof(BMPHeader)) != sizeof(BMPHeader) || !isBMPFile(&header)) {
        fprintf(stderr, "The input file is not a BMP file.\n");
        exit(EXIT_FAILURE);
    }

    int width = header.width;
    int height = header.height;

    snprintf(statistics, sizeof(statistics),
             "File name: %s\nHeight: %d\nWidth: %d\nSize: %d\nUser ID: %d\nLast modified time: %sLink count: %ld\nUser access rights: RWX\nGroup access rights: R--\nOthers access rights: ---\n",
             input_file, height, width, (int)info.st_size, info.st_uid, ctime(&info.st_mtime), info.st_nlink);

    char output_file[PATH_MAX];
    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", output_dir, input_file);

    out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1) {
        perror("Error opening output file");
        close(inp);
        exit(EXIT_FAILURE);
    }

    if (write(out, statistics, strlen(statistics)) == -1) {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void statisticsDirectory(const char *file) {
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(file, O_RDONLY)) == -1) {
        perror("Error opening input file");
        exit(2);
    }

    if (fstat(inp, &info) == -1) {
        perror("Error getting file information");
        close(inp);
        exit(2);
    }

    


    snprintf(statistics, sizeof(statistics),
        "File name: %s\nUser ID: %d\nUser access rights: RWX\nGroup access rights: R--\nOthers access rights: ---\n",
        file,  info.st_uid);

    out = open("statistica.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1) {
        perror("Error opening output file");
        close(inp);
        exit(3);
    }

    if (write(out, statistics, strlen(statistics)) == -1) {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void convertToGrayscale(const char *input_bmp, const char *output_dir) {
   
    FILE *fp_in = fopen(input_bmp, "rb");
    if (!fp_in) {
        perror("Error opening input BMP file");
        exit(EXIT_FAILURE);
    }

    BMPHeader header;
    if (fread(&header, sizeof(BMPHeader), 1, fp_in) != 1 || !isBMPFile(&header)) {
        fprintf(stderr, "The input file is not a BMP file.\n");
        fclose(fp_in);
        exit(EXIT_FAILURE);
    }

    int width = header.width;
    int height = header.height;
    int row_size = width * 3; 

    char output_file[PATH_MAX];
    snprintf(output_file, sizeof(output_file), "%s/%s_grayscale.bmp", output_dir, input_bmp);

    FILE *fp_out = fopen(output_file, "wb");
    if (!fp_out) {
        perror("Error opening output grayscale BMP file");
        fclose(fp_in);
        exit(EXIT_FAILURE);
    }

    fwrite(&header, sizeof(BMPHeader), 1, fp_out); 

    fseek(fp_in, header.data_offset, SEEK_SET); 

    unsigned char pixel[3];
    double P_gri;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            fread(&pixel, sizeof(pixel), 1, fp_in);

            P_gri = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]; 

            unsigned char gray = (unsigned char)round(P_gri); 

            
            fwrite(&gray, sizeof(unsigned char), 1, fp_out);    
            fwrite(&gray, sizeof(unsigned char), 1, fp_out);    
            fwrite(&gray, sizeof(unsigned char), 1, fp_out);   
        }
        if (row_size % 4 != 0) { 
            fseek(fp_in, 4 - (row_size % 4), SEEK_CUR);
        }
    }

    fclose(fp_in);
    fclose(fp_out);
}

void processDirectory(const char *input_dir, const char *output_dir) {
    DIR *dir = opendir(input_dir);
    if (!dir) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", input_dir, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                
                processDirectory(file_path, output_dir);
            }
        } else if (isRegularFile(file_path)) {
            if (isBMPFilePath(file_path)) {
                pid_t pid = fork();
                if (pid == 0) {
                    
                    convertToGrayscale(file_path, output_dir);
                    exit(EXIT_SUCCESS);
                } else if (pid > 0) {
                    
                    calculateStatistics(file_path, output_dir);
                    int status;
                    waitpid(pid, &status, 0);
                    printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
                } else {
                    perror("Failed to fork");
                    exit(EXIT_FAILURE);
                }
            } else {
               
                pid_t pid = fork();
                if (pid == 0) {
                    calculateStatistics(file_path, output_dir);
                    exit(EXIT_SUCCESS);
                } else if (pid < 0) {
                    perror("Failed to fork");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *args[]) {
    if (argc != 3) {
        printf("Usage: %s <input_directory> <output_directory>\n", args[0]);
        exit(EXIT_FAILURE);
    }

    processDirectory(args[1], args[2]);

    return 0;
}
