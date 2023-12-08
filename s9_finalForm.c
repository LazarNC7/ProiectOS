#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stddef.h>
#include <sys/wait.h>
#include <libgen.h>

// Are rolul de a retine toate informatiile despre un .bmp file conform tabelului
#pragma pack(push, 1)
typedef struct
{
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
} Header;
#pragma pack(pop)

int isBMPFile(const Header *header)
{
    return (header->signature[0] == 'B' && header->signature[1] == 'M');
}

int isBMPFilePath(const char *file_path)
{
    int inp;
    Header header;

    if ((inp = open(file_path, O_RDONLY)) == -1)
    {
        perror("BMP Error opening input file");
        exit(2);
    }

    if (read(inp, &header, sizeof(Header)) != sizeof(Header) ||
        header.signature[0] != 'B' || header.signature[1] != 'M') // Description-ul este ca 'BM' header-ul fisierului, potrivit tabelului
    {
        close(inp);
        return 0;
    }

    close(inp);
    return 1;
}

int isDirectory(const char *path)
{
    struct stat info;
    if (stat(path, &info) != 0)
    {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISDIR(info.st_mode);
}

int isRegularFile(const char *path)
{
    struct stat info;
    if (lstat(path, &info) != 0)
    {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISREG(info.st_mode) && !S_ISLNK(info.st_mode);
}

int isNonBMPFile(const char *path)
{
    if (isRegularFile(path))
    {
        size_t path_len = strlen(path);
        return (path_len < 4 || (strcmp(path + path_len - 4, ".bmp") != 0));
    }
    return 0;
}

int isSymbolicLink(const char *path)
{
    struct stat info;
    if (lstat(path, &info) != 0)
    {
        perror("Error getting file information");
        exit(2);
    }
    return S_ISLNK(info.st_mode);
}

void statistics(const char *file, const char *output_dir)
{
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(file, O_RDONLY)) == -1)
    {
        perror("Error opening input file");
        exit(2);
    }

    if (fstat(inp, &info) == -1)
    {
        perror("Error getting file information");
        close(inp);
        exit(2);
    }

    Header header;
    if (read(inp, &header, sizeof(Header)) != sizeof(Header) || !isBMPFile(&header))
    {
        fprintf(stderr, "The input file is not a BMP file.\n");
        exit(5);
    }

    int width = *(int *)((char *)&header + 18);
    int height = *(int *)((char *)&header + 22);
    // Se extrag permisiunile pentru read, write si execute pentru user, others si group
    int permissions = info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    char perms[10];
    sprintf(perms, "%o", permissions);

    // Informatiile ce trebuie salvate
    snprintf(statistics, sizeof(statistics),
             "Nume fisier: %s\nInaltime: %d\nLungime: %d\nDimensiune: %d\nIdentificatorul utilizatorului: %d\nTimpul ultimei modificari: %sContorul de legaturi: %ld\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s\n",
             file, height, width, (int)info.st_size, info.st_uid, ctime(&info.st_mtime), info.st_nlink, (perms[0] == '7') ? "RWX" : ((perms[0] == '6') ? "RW-" : ((perms[0] == '5') ? "R-X" : ((perms[0] == '4') ? "R--" : ((perms[0] == '3') ? "-WX" : ((perms[0] == '2') ? "-W-" : ((perms[0] == '1') ? "--X" : "---")))))),
             (perms[1] == '7') ? "RWX" : ((perms[1] == '6') ? "RW-" : ((perms[1] == '5') ? "R-X" : ((perms[1] == '4') ? "R--" : ((perms[1] == '3') ? "-WX" : ((perms[1] == '2') ? "-W-" : ((perms[1] == '1') ? "--X" : "---")))))),
             (perms[2] == '7') ? "RWX" : ((perms[2] == '6') ? "RW-" : ((perms[2] == '5') ? "R-X" : ((perms[2] == '4') ? "R--" : ((perms[2] == '3') ? "-WX" : ((perms[2] == '2') ? "-W-" : ((perms[2] == '1') ? "--X" : "---")))))));

    char output_file[PATH_MAX];
    char aux[100];
    int i = 0;
    // se izoleaza numele fisierului, fara path
    char *last_occurrence = strrchr(file, '/');
    strcpy(aux, last_occurrence + 1);
    for (i = 0; i < strlen(aux) && aux[i] != '.'; i++)
    {
        aux[i] = aux[i];
    }
    aux[i] = '\0';
    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", output_dir, aux);

    // Am ales sa nu il trunchiez, pentru a fi mai relevant apoi la countLines
    out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1)
    {
        perror("Error opening output file");
        close(inp);
        exit(EXIT_FAILURE);
    }

    if (write(out, statistics, strlen(statistics)) == -1)
    {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

// Similar pentru restul fisierelor. Am ales sa fac o functie pentru fiecare tip de statistica, in loc sa fac switch/sa folosesc if-uri
// pentru a adauga/scoate din informatie pentru simplitate
void statisticsDirectory(const char *file, const char *output_dir)
{
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(file, O_RDONLY)) == -1)
    {
        perror("Error opening input file");
        exit(2);
    }

    if (fstat(inp, &info) == -1)
    {
        perror("Error getting file information");
        close(inp);
        exit(2);
    }

    int permissions = info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    char perms[10];
    sprintf(perms, "%o", permissions);

    snprintf(statistics, sizeof(statistics),
             "Nume director: %s\nIdentificatorul utilizatorului: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s\n",
             file, info.st_uid, (perms[0] == '7') ? "RWX" : ((perms[0] == '6') ? "RW-" : ((perms[0] == '5') ? "R-X" : ((perms[0] == '4') ? "R--" : ((perms[0] == '3') ? "-WX" : ((perms[0] == '2') ? "-W-" : ((perms[0] == '1') ? "--X" : "---")))))),
             (perms[1] == '7') ? "RWX" : ((perms[1] == '6') ? "RW-" : ((perms[1] == '5') ? "R-X" : ((perms[1] == '4') ? "R--" : ((perms[1] == '3') ? "-WX" : ((perms[1] == '2') ? "-W-" : ((perms[1] == '1') ? "--X" : "---")))))),
             (perms[2] == '7') ? "RWX" : ((perms[2] == '6') ? "RW-" : ((perms[2] == '5') ? "R-X" : ((perms[2] == '4') ? "R--" : ((perms[2] == '3') ? "-WX" : ((perms[2] == '2') ? "-W-" : ((perms[2] == '1') ? "--X" : "---")))))));

    char output_file[PATH_MAX];
    char aux[100];
    int i = 0;
    char *last_occurrence = strrchr(file, '/');
    strcpy(aux, last_occurrence + 1);
    for (i = 0; i < strlen(aux) && aux[i] != '.'; i++)
    {
        aux[i] = aux[i];
    }
    aux[i] = '\0';
    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", output_dir, aux);

    out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1)
    {
        perror("Error opening output file");
        close(inp);
        exit(EXIT_FAILURE);
    }

    if (write(out, statistics, strlen(statistics)) == -1)
    {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void statisticsRegFile(const char *file, const char *output_dir)
{
    int inp, out;
    struct stat info;
    char statistics[500];

    if ((inp = open(file, O_RDONLY)) == -1)
    {
        perror("Error opening input file");
        exit(2);
    }

    if (fstat(inp, &info) == -1)
    {
        perror("Error getting file information");
        close(inp);
        exit(2);
    }

    int permissions = info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    char perms[10];
    sprintf(perms, "%o", permissions);

    snprintf(statistics, sizeof(statistics),
             "Nume fisier: %s\nDimensiune: %d\nIdentificatorul utilizatorului: %d\nTimpul ultimei modificari: %sContorul de legaturi: %ld\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s\n",
             file, (int)info.st_size, info.st_uid, ctime(&info.st_mtime), info.st_nlink, (perms[0] == '7') ? "RWX" : ((perms[0] == '6') ? "RW-" : ((perms[0] == '5') ? "R-X" : ((perms[0] == '4') ? "R--" : ((perms[0] == '3') ? "-WX" : ((perms[0] == '2') ? "-W-" : ((perms[0] == '1') ? "--X" : "---")))))),
             (perms[1] == '7') ? "RWX" : ((perms[1] == '6') ? "RW-" : ((perms[1] == '5') ? "R-X" : ((perms[1] == '4') ? "R--" : ((perms[1] == '3') ? "-WX" : ((perms[1] == '2') ? "-W-" : ((perms[1] == '1') ? "--X" : "---")))))),
             (perms[2] == '7') ? "RWX" : ((perms[2] == '6') ? "RW-" : ((perms[2] == '5') ? "R-X" : ((perms[2] == '4') ? "R--" : ((perms[2] == '3') ? "-WX" : ((perms[2] == '2') ? "-W-" : ((perms[2] == '1') ? "--X" : "---")))))));

    char output_file[PATH_MAX];
    char aux[100];
    int i = 0;
    char *last_occurrence = strrchr(file, '/');
    strcpy(aux, last_occurrence + 1);
    for (i = 0; i < strlen(aux) && aux[i] != '.'; i++)
    {
        aux[i] = aux[i];
    }
    aux[i] = '\0';
    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", output_dir, aux);

    out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1)
    {
        perror("Error opening output file");
        close(inp);
        exit(EXIT_FAILURE);
    }

    if (write(out, statistics, strlen(statistics)) == -1)
    {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void statisticsSymbolicFile(const char *file, const char *output_dir)
{
    int inp, out;
    struct stat info;
    char statistics[500];

    const char *symbolicLinkPath = file;
    char targetFilePath[100];

    ssize_t targetPathLength = readlink(symbolicLinkPath, targetFilePath, sizeof(targetFilePath) - 1);
    if (targetPathLength == -1)
    {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    targetFilePath[targetPathLength] = '\0';

    if (lstat(targetFilePath, &info) == -1)
    {
        perror("lstat");
        exit(EXIT_FAILURE);
    }

    if ((inp = open(symbolicLinkPath, O_RDONLY)) == -1)
    {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    if (fstat(inp, &info) == -1)
    {
        perror("Error getting file information");
        close(inp);
        exit(EXIT_FAILURE);
    }

    int permissions = info.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    char perms[10];
    sprintf(perms, "%o", permissions);

    snprintf(statistics, sizeof(statistics),
             "Nume legatura: %s\nDimensiune: %d\nDimensiune fisier target: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces altii: %s\n",
             file, (int)info.st_size, (int)info.st_nlink, (perms[0] == '7') ? "RWX" : ((perms[0] == '6') ? "RW-" : ((perms[0] == '5') ? "R-X" : ((perms[0] == '4') ? "R--" : ((perms[0] == '3') ? "-WX" : ((perms[0] == '2') ? "-W-" : ((perms[0] == '1') ? "--X" : "---")))))),
             (perms[1] == '7') ? "RWX" : ((perms[1] == '6') ? "RW-" : ((perms[1] == '5') ? "R-X" : ((perms[1] == '4') ? "R--" : ((perms[1] == '3') ? "-WX" : ((perms[1] == '2') ? "-W-" : ((perms[1] == '1') ? "--X" : "---")))))),
             (perms[2] == '7') ? "RWX" : ((perms[2] == '6') ? "RW-" : ((perms[2] == '5') ? "R-X" : ((perms[2] == '4') ? "R--" : ((perms[2] == '3') ? "-WX" : ((perms[2] == '2') ? "-W-" : ((perms[2] == '1') ? "--X" : "---")))))));

    char output_file[PATH_MAX];
    char aux[100];
    int i = 0;
    char *last_occurrence = strrchr(file, '/');
    strcpy(aux, last_occurrence + 1);
    for (i = 0; i < strlen(aux) && aux[i] != '.'; i++)
    {
        aux[i] = aux[i];
    }
    aux[i] = '\0';
    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", output_dir, aux);

    out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out == -1)
    {
        perror("Error opening output file");
        close(inp);
        exit(EXIT_FAILURE);
    }

    if (write(out, statistics, strlen(statistics)) == -1)
    {
        perror("Error writing to output file");
    }

    close(inp);
    close(out);
}

void convertToGrayscale(const char *file_path)
{
    int inp;
    Header header;

    if ((inp = open(file_path, O_RDWR)) == -1)
    {
        perror("Grayscale Error opening input file");
        exit(2);
    }
    // Se verifica mai intai daca headerul nu e compromis/daca e fisier .bmp
    if (read(inp, &header, sizeof(Header)) != sizeof(Header) || !isBMPFile(&header))
    {
        fprintf(stderr, "The input file is not a BMP file.\n");
        close(inp);
        exit(5);
    }

    int width = header.width;
    int height = header.height;

    unsigned char pixel[3];
    lseek(inp, header.data_offset, SEEK_SET);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Se citesc culorile RGB ale pixelilor. Potrivit Wiki se citesc in ordinea blue, green, red
            read(inp, pixel, sizeof(pixel));

            // Se aplica formula din lab
            unsigned char gray = (unsigned char)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);

            lseek(inp, -3, SEEK_CUR);
            // Pentru gri culorile RGB trebuie sa fie aceleasi
            pixel[0] = pixel[1] = pixel[2] = gray;
            //		printf("%s %d %d %d\n",file_path, pixel[2],pixel[1],pixel[0]);
            if (write(inp, pixel, sizeof(pixel)) != sizeof(pixel))
            {
                perror("Error writing pixel");
                close(inp);
                exit(7);
            }
        }
    }

    close(inp);
}

void countLines(const char *file_path)
{
    // Creeaza un fisier temporar pentru a stoca rezultatul numararii liniilor
    FILE *tmp_file = tmpfile();

    if (tmp_file == NULL)
    {
        perror("Error creating temporary file");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // Redirectioneaza iesirea comenzii wc -l către fisierul temporar
        dup2(fileno(tmp_file), STDOUT_FILENO); // Utilizeaza fileno pentru a obtine descriptorul de fidier al fisierului temporar

        // Executa comanda wc -l pentru a numara liniile fisierului
        printf("%d\n", execlp("wc", "wc", "-l", file_path, NULL));

        perror("Error executing wc -l");
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0)
            {
                // Repozitioneaza cursorul la inceputul fisierului temporar
                rewind(tmp_file);

                int line_count;
                fscanf(tmp_file, "%d", &line_count);
                printf("Au fost identificate in total %d linii în fișierul %s\n", line_count, file_path);
            }
            else
            {
                printf("S-a încheiat procesul de numărare cu codul %d\n", WEXITSTATUS(status));
            }
        }
    }
    else
    {
        perror("Error creating child process for counting");
    }

    fclose(tmp_file);
}

void countCorrectSentences(char character, int *pipe_fd)
{
    char command[100];
    int count = 0;

    char content[1024]; // Buffer

    // Citeste contentul de la parinte prin pipe
    read(pipe_fd[0], content, sizeof(content));
    printf("Content:%s\n", content);
    char tmp_file[] = "/tmp/tmp_file.txt";
    FILE *fp = fopen(tmp_file, "w");
    if (fp == NULL)
    {
        perror("Error creating temporary file");
        exit(1);
    }
    fprintf(fp, "%s", content);
    fclose(fp);

    // Se construieste comanda 'inspirata' din trimiterea prin linie de comanda
    snprintf(command, sizeof(command), "bash scriptS9.sh %c<%s", character, tmp_file);

    fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("Error executing script");
        exit(1);
    }

    // Buffer
    char output_buffer[100];
    size_t bytes_read = fread(output_buffer, sizeof(char), sizeof(output_buffer), fp);
    // printf("%s", output_buffer);

    if (bytes_read > 0)
    {
        // Script-yl trimite un string ce trebuie convertit in int
        count = atoi(output_buffer);

        // Trimite rezultatul la parinte prin pipe
        write(pipe_fd[1], &count, sizeof(count));
        printf("Count: %d\n", count);
    }
    else
    {
        printf("No output received from the script.\n");
    }

    pclose(fp);

    remove(tmp_file);
}

// Functie ajutatoare pt convertToGrayscale--verifica daca fisierul e BMP pentru a nu se face nou proces daca nu e necesar
void convertToGrayScaleBMP(const char *entry, const char *inputDir)
{
    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", inputDir, entry);
    if (isBMPFilePath(file_path))
    {
        char file_path[PATH_MAX];

        snprintf(file_path, sizeof(file_path), "%s/%s", inputDir, entry);

        convertToGrayscale(file_path);
    }
}

void processEntry(const char *entry, const char *inputDir, const char *outputDir, char character, int *pipe_fd)
{
    char file_path[PATH_MAX];
    // Se construieste file path-ul pentru fisierul ce va fi folosit pentru statistica
    snprintf(file_path, sizeof(file_path), "%s/%s", inputDir, entry);

    int pipe_fd1[2];
    if (pipe(pipe_fd1) == -1)
    {
        perror("Error creating pipe");
        exit(1);
    }

    // if pentru procesare in functie de tipul de fisier
    if (isDirectory(file_path))
    {
        if (strcmp(entry, ".") != 0 && strcmp(entry, "..") != 0)
        {

            statisticsDirectory(file_path, outputDir);
        }
    }
    else if (isRegularFile(file_path))
    {

        if (isBMPFilePath(file_path))
        {

            statistics(file_path, outputDir);
        }
        else if (isNonBMPFile(file_path))
        {

            statisticsRegFile(file_path, outputDir);

            char content[1024];
            FILE *fp = fopen(file_path, "r");
            if (fp == NULL)
            {
                perror("The file cannot be opened");
                exit(EXIT_FAILURE);
            }
            fread(content, sizeof(char), sizeof(content), fp);
            fclose(fp);
            // printf("%s\n", content);

            // Trimite continutul prin pipe catre functia countCorrectSentences
            write(pipe_fd[1], content, strlen(content));
        }
    }
    else if (isSymbolicLink(file_path))
    {

        statisticsSymbolicFile(file_path, outputDir);
    }
}

void processDirectory(const char *inputDir, const char *outputDir, char c)
{
    DIR *dir = opendir(inputDir);
    if (!dir)
    {
        perror("Error opening directory");
        exit(4);
    }

    // Se creeaza pipe pentru countCorrectSentences
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Error creating pipe");
        exit(1);
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            //Procesul specific pentru statistica
            pid_t pid_stat = fork();
            if (pid_stat == 0)
            {
                // Procesul fiu pentru numararea propozitiilor

                processEntry(entry->d_name, inputDir, outputDir, c, pipe_fd);
            }
            else if (pid_stat > 0)
            {
                // In interiorul procesului parinte
                int status;
                waitpid(pid_stat, &status, 0);
                if (WIFEXITED(status))
                {
                    char file_path[PATH_MAX];
                    //file path ul pentru fisierul de intrare
                    snprintf(file_path, sizeof(file_path), "%s/%s", inputDir, entry->d_name);
                    char *base_name = basename(strdup(file_path));
                    char *dot_position = strrchr(base_name, '.');//scapa de terminatie pentru a nu intra in compozitia numelui

                    if (dot_position != NULL)
                    {
                        *dot_position = '\0';
                    }

                    char output_file[PATH_MAX];
                    // La fel, doar la directory-ul de output pt fisierul cu statistica
                    snprintf(output_file, sizeof(output_file), "%s/%s_statistica.txt", outputDir, base_name);
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                        countLines(output_file);// determina nr de linii in urma procesarii statisticii

                        //cel de al doilea proces al parintelui (S9). Se creeaza numai cand e un fisier obisnuit, fara .bmp
                    if (isRegularFile(file_path) && isNonBMPFile(file_path))
                    {
                        pid_t count_pid = fork();
                        if (count_pid == 0)
                        {
                            countCorrectSentences(c, pipe_fd);
                        }
                        else if (count_pid > 0)
                        {

                            int status;
                            waitpid(count_pid, &status, 0);

                            if (WIFEXITED(status))
                            {
                                // In interiorul procesului parinte
                               
                                
                                close(pipe_fd[1]); // Inchide partea de scriere a pipe-ului
                                int count;
                                read(pipe_fd[0], &count, sizeof(count));
                                printf("!!!!!!!!!Au fost identificate in total %d propozitii corecte care contin caracterul %c!!!!!!!!!\n", count, c);
                                close(pipe_fd[0]); // Inchide partea de citire a pipe-ului
                                printf("S-a incheiat procesul cu pid-ul %d si codul %d\n", (int)count_pid, WEXITSTATUS(status));
                            }
                        }
                        else
                        {
                            perror("Error creating child process for counting");
                        }
                    }
                    printf("S-a incheiat procesul cu pid-ul %d si codul %d\n", (int)pid_stat, WEXITSTATUS(status));
                }
            }

            else
            {
                perror("Error creating child process for entry processing");
            }

            //Procesul coresspunzator pentru conversia in grayscale
            pid_t pid_gs = fork();
            if (pid_gs == 0)
                convertToGrayScaleBMP(entry->d_name, inputDir);
            else if (pid_gs > 0)
            {
                // In interiorul procesului parinte
                int status;
                waitpid(pid_gs, &status, 0);
                if (WIFEXITED(status))
                {

                    printf("S-a incheiat procesul cu pid-ul %d si codul %d\n", (int)pid_gs, WEXITSTATUS(status));
                }
            }
            else
            {
                perror("Error creating child process for entry processing");
            }
            exit(0);
        }
        else if (pid < 0)
        {
            perror("Error creating child process for entry processing");
        }
    }
    //Structura permite procesului parinte sa monitorizeze procesele sale copil, asteptand pana cand fiecare proces copil isi 
    //schimba starea si verificand daca acestea s-au terminat in mod normal. 
    int status;
    pid_t wpid;
    while ((wpid = waitpid(-1, &status, 0)) > 0)
    {
        if (WIFEXITED(status))
        {
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", (int)wpid, WEXITSTATUS(status));
        }
    }

    closedir(dir);
}

int main(int argc, char *args[])
{
    if (argc != 4)
    {
        // Se va introduce full path-ul
        printf("Usage: %s <input_directory> <output_directory <character>\n", args[0]);
        exit(1);
    }

    char c = args[3][0];
    processDirectory(args[1], args[2], c);

    return 0;
}
