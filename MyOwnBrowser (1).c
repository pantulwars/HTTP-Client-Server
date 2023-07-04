#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define const1 50
#define const2 20
#define const3 1024
void viewer(char *file_type, char *filename)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0)
    {
        if (strcmp(file_type, "pdf") == 0)
        {
            execlp("evince", "evince", filename, NULL);
            perror("execlp");
        }
        else if (strcmp(file_type, "jpeg") == 0)
        {
            char *args[] = {"eog", filename, NULL};
            execvp("eog", args);
            perror("execvp");
        }
        else if (strcmp(file_type, "html") == 0)
        {
            char *args[] = {"google-chrome", filename, NULL};
            execvp("google-chrome", args);
            perror("execvp");
        }
        else
        {
            char *args[] = {"gedit", filename, NULL};
            execvp("gedit", args);
            perror("execvp");
        }
        exit(1);
    }
    else
    {
        waitpid(pid, &status, 0);
    }
    return;
}

int find_port(char url[], char *ip)
{
    char num[8];
    if (strstr(ip, ":") != NULL)
    {
        char *token;
        token = strtok(ip, ":");
        token = strtok(NULL, "");
        strcpy(num, token);
        return atoi(num);
    }
    else
    {
        for (int i = 0; i < strlen(url); i++)
        {
            if (url[i] == ':' && url[i + 1] == '/')
            {
            }
            else if (url[i] == ':' && url[i + 1] != '/')
            {
                strcpy(num, url + i + 1);
                return atoi(num);
            }
        }
        return -1;
    }
}
int extract_info(char *buffer, char content_type[])
{
    char *content_type_start = strstr(buffer, "Content-Type: ");
    if (content_type_start != NULL)
    {
        content_type_start += strlen("Content-Type: ");
        // char end_str[5];
        char *content_type_end;
        if (strstr(content_type_start, ";") != NULL)
        {
            content_type_end = strstr(content_type_start, ";");
        }
        else
        {
            content_type_end = strstr(content_type_start, "\r\n");
        }
        if (content_type_end != NULL)
        {
            int content_type_len = content_type_end - content_type_start;
            strncpy(content_type, content_type_start, content_type_len);
            content_type[content_type_len] = '\0';
        }
    }
    printf("////%s\n", content_type);
    char *content_length = strstr(buffer, "Content-Length: ");
    if (content_length)
    {
        content_length += strlen("Content-Length: ");
        char *end_of_line = strstr(content_length, "\r\n");
        if (end_of_line)
        {
            *end_of_line = '\0';
            int content_length_value = atoi(content_length);
            printf("Content-Length: %d\n", content_length_value);
            return content_length_value;
        }
    }
}

void recv_func(char *buffer, char *buff, int sock, FILE *fptr, char *file_type, char *filename)
{
    // int content_length;
    // int f_msg_complete;
    // int k;
    memset(buffer, '\0', const3);
    // f_msg_complete = 0, k = 0;
    // char c;
    char *point;
    while (1)
    {
        memset(buff, '\0', const1);
        recv(sock, buff, const1, 0);
        // printf("%s", buff);

        strcat(buffer, buff);
        point = strstr(buffer, "\r\n\r\n");
        int len;
        int flag_end = 0;
        if (point != NULL)
        {
            if (strlen(buffer) >= (point - buffer + 4))
            {
                fprintf(fptr, "%s", point + 4);
                // fwrite(point+4, strlen(point), 1, fptr);
            }
            do
            {
                len = recv(sock, buff, const1, MSG_WAITALL);
                fwrite(buff, sizeof(char), len, fptr);

            } while (len > 0);
            fclose(fptr);
            flag_end = 1;
        }
        if (flag_end == 1)
            break;
    }
    viewer(file_type, filename);
}

void get(char *buffer, char *buff, char *ip, int sockfd, char *filename, char *file_type)
{

    strcat(buffer, "Host: ");
    strcat(buffer, ip);
    strcat(buffer, "\r\n");
    strcat(buffer, "Accept: application/pdf, image/jpeg, text/html\r\n\r\n");
    // printf("Sent:\n%s\n", buffer);
    send(sockfd, buffer, strlen(buffer) + 1, 0);
    char temp[const1];
    strcpy(temp, file_type);
    if (strcmp(temp, "pdf") == 0)
    {
        FILE *fptr;
        fptr = fopen(filename, "wb");
        if (fptr == NULL)
        {
            printf("Error opening file!\n");
            exit(0);
        }
        recv_func(buffer, buff, sockfd, fptr, file_type, filename);
    }
    else if (strcmp(temp, "jpeg") == 0)
    {
        FILE *fptr;
        fptr = fopen(filename, "wb");
        if (fptr == NULL)
        {
            printf("Error opening file!\n");
            exit(0);
        }
        recv_func(buffer, buff, sockfd, fptr, file_type, filename);
    }
    else if (strcmp(temp, "html") == 0)
    {
        FILE *fptr;
        fptr = fopen(filename, "wb");
        if (fptr == NULL)
        {
            printf("Error opening file!\n");
            exit(0);
        }
        recv_func(buffer, buff, sockfd, fptr, file_type, filename);
    }
    else
    {
        FILE *fptr;
        fptr = fopen(filename, "wb");
        if (fptr == NULL)
        {
            printf("Error opening file!\n");
            exit(0);
        }
        recv_func(buffer, buff, sockfd, fptr, file_type, filename);
    }
}
void put(char *buffer, char *buff, char *filename, char *file_type, char *ip, int sock)
{
    char array[const1], str[const2];
    // int bytes_read;

    if (strcmp(file_type, "pdf") == 0)
    {
        strcpy(array, "application/pdf");
    }
    else if (strcmp(file_type, "jpeg") == 0)
    {
        strcpy(array, "image/jpeg");
    }
    else if (strcmp(file_type, "html") == 0)
    {
        strcpy(array, "text/html");
    }
    else
    {
        strcpy(array, "text/*");
    }
    // printf("filename: %s\n", filename);
    FILE *file;
    file = fopen(filename, "rb");
    if (!file)
    {
        perror("fopen");
        exit(1);
    }
    /* Get the file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    // printf("filesize: %d\n", file_size);
    /* Build the HTTP request */
    strcat(buffer, "Host: ");
    strcat(buffer, ip);
    strcat(buffer, "\r\n");
    strcat(buffer, "Content-Length: ");
    sprintf(str, "%lu", file_size);
    strcat(buffer, str);
    strcat(buffer, "\r\n");
    strcat(buffer, "Content-Type: ");
    strcat(buffer, array);
    strcat(buffer, "\r\n\r\n");
    // printf("buffer put: %s\n", buffer);
    send(sock, buffer, const3, 0);
    // printf("sent put: %s\n", buffer);
    // Read chunks of the file and send them to the server
    // memset(buffer, '\0', const3);
    size_t bytes_read;
    while ((bytes_read = fread(buff, 1, const1 - 1, file)) > 0)
    {
        buff[bytes_read] = '\0';
        // printf("buffer sent: %s %d\n", buff, bytes_read);
        send(sock, buffer, bytes_read, 0);
        memset(buff, '\0', const1);
    }
    // printf("File sent\n");
    fclose(file);
    close(sock);
}
void parse(char *url, char hostname[], char path[], char filename[], char file_type[], int flag_file)
{
    char *token;
    token = strtok(url, "://");
    // printf("%s\n", token);
    token = strtok(NULL, "/");
    // printf("%s\n", token);
    strcpy(hostname, token);
    token = strtok(NULL, "");
    if (token == NULL)
    {
        strcpy(path, "\0");
    }
    else
    {
        strcpy(path, "/");
        strcat(path, token);
    }

    if (!flag_file)
    {
        token = strrchr(path, '/');
        strcpy(filename, token + 1);
    }
    else
    {
    }
    if (strstr(filename, "pdf"))
    {
        strcpy(file_type, "pdf");
    }
    else if (strstr(filename, "jpg") || strstr(filename, "jpeg"))
    {
        strcpy(file_type, "jpeg");
    }
    else if (strstr(filename, "html"))
    {
        strcpy(file_type, "html");
    }
    else
    {
        strcpy(file_type, "others");
    }
}
int main()
{
    int sockfd, newsockfd, status;
    struct sockaddr_in serv_addr;
    char url[const1], temp[const1], hostname[const1], path[const1], filename[const1], file_type[const1];
    char buffer[const3], buff[const1], cmd[5], ip[const2];
    int port, flag_file, flag_close = 0;
    char *token;

    while (flag_close == 0)
    {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Cannot create socket\n");
            exit(0);
        }

        flag_file = 0;
        printf("\nMyBrowser> ");
        memset(url, '\0', sizeof(url));
        scanf("%s", cmd);
        scanf("%s", url);
        // printf("%s\n",url);

        if (strcmp(cmd, "PUT") == 0)
        {
            scanf("%s", filename);
            flag_file = 1;
        }
        strcpy(temp, url);
        parse(temp, hostname, path, filename, file_type, flag_file);
        strcpy(ip, hostname);
        // printf("filename: %s\n", filename);
        // printf("filetype: %s\n", file_type);
        // printf("hostname: %s\n", hostname);
        // printf("path: %s\n", path);
        port = find_port(url, ip);
        // printf("%s\n", ip);
        if (strcmp(path, "\0") == 0)
        {
        }
        else
        {
            if (port == -1)
            {
                token = strtok(path, "");
            }
            else
            {
                token = strtok(path, ":");
            }
            strcpy(path, token);
            // printf("path: %s\n", path);
        }
        if (port == -1 || port == 0)
        {
            port = 80;
        }
        printf("Connected to IP address: %s and Port: %d\n", ip, port);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);
        newsockfd = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (newsockfd < 0)
        {
            perror("Connection failed\n");
            exit(0);
        }
        if (fork() == 0)
        {
            memset(buffer, '\0', const3);
            strcpy(buffer, cmd);
            strcat(buffer, " ");
            if (strcmp(cmd, "PUT") == 0)
            {
                strcat(buffer, "/");
                strcat(buffer, filename);
                strcat(buffer, " ");
            }
            else
            {

                strcat(buffer, path);
            }
            strcat(buffer, "HTTP/1.1\r\n");
            // printf("\n%s\n", buffer);
            if (strcmp(cmd, "GET") == 0)
            {
                printf("\nGetting file %s from the host\n\n", filename);
                get(buffer, buff, ip, sockfd, filename, file_type);
                close(sockfd);
            }
            else if (strcmp(cmd, "PUT") == 0)
            {
                printf("\nSending file %s to the host\n\n", filename);
                put(buffer, buff, filename, file_type, ip, sockfd);
                close(sockfd);
            }
            else
            {
                flag_close = 1;
                close(sockfd);
            }
        }
        else
        {
            wait(&status);
        }
    }
    close(sockfd);
    return 0;
}
