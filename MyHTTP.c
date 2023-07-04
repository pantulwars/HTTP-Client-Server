#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 16
#define MESLEN  10000

void fetch_request_header(int newsockfd, char* buf, char* message, char* start_body);
void parse_request_header(char* headers, char** method, char** path, char** ver, char** values);
void handle_request(int sockfd, char *method,char *path, char* buf, char* start_body, char** values);

int main(int argc, char** argv)
{
    int port;
    if(argc==2){
        port = atoi(argv[1]);
    }
    else port = 9000;

    int serv_len, cli_len;
    struct sockaddr_in serv_addr, cli_addr;
    serv_len = sizeof(serv_addr);
    cli_len = sizeof(cli_addr);

    char buf[BUFSIZE];
    char header[MESLEN];
    char start_body[MESLEN];
    char* values[6];

    serv_addr.sin_family = AF_INET;            
    serv_addr.sin_addr.s_addr = INADDR_ANY;    
    serv_addr.sin_port = htons(port);          
        
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        printf("Failed to create socket!\n");
        return 1;
    }
    if(bind(sockfd,(struct sockaddr*)&serv_addr, serv_len)<0){
        printf("Unable to bind!\n");
        return 1;
    }

    listen(sockfd,10);

    char* method, *path, *ver;
    int newsockfd;
    while(1)
    {
        printf("\t\t\t******Waiting for client******\n");
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
        if(newsockfd){
            printf("Connected to Client!\n");
        }
        
        fetch_request_header(newsockfd, buf, header,start_body);

        parse_request_header(header,&method,&path,&ver,values);
        
        handle_request(newsockfd, method, path, buf, start_body, values);

        close(newsockfd);
    }
    return 0;
}

void fetch_request_header(int newsockfd, char* buf, char* message, char* start_body)
{
    int i,l,j=0,k=0,eoh=0;
    int newline_cnt=0;
    while(1){
        l = recv(newsockfd, buf, BUFSIZE-1, 0);

        printf("RECEIVED:\n%s\n", buf);

        if(l==0){
            break;
        }
        if(l<0){
            perror("Unable to Read from Socket\n");
        }
        for(i=0; i<l; i++){
            if(j>0 && buf[i]=='\n' && message[j-1]=='\n') {
                message[j] = '\0';
                eoh=1;
                i++;
                break;
            }
            if(buf[i]=='\r') continue;
            message[j++] = buf[i];
        }
        if(eoh){
            while(i<l){
                // printf("start body --> %c\n", buf[i]);
                start_body[k++]=buf[i++];
            }
            start_body[i]='\0';
        }
        if(buf[l-1]=='\0'){
            break;
        }
    }
    printf("PROCESSED HEADER:\n%s\n", message);    
}


void parse_request_header(char* input_headers, char** method, char** path, char** ver, char** values)
{
    *method = strsep(&input_headers, " ");
    *path = strsep(&input_headers, " ");
    *ver = strsep(&input_headers, " \n");

    char *get_headers[] = {"Host:", "Date:", "Accept:", "Accept-Language:", "If-Modified-Since:", "Connection:"};
    char *put_headers[] = {"Host:", "Date:", "Content-Type:", "Content-Length:", "Connection:"};

    char* line;
    char** all_headers = get_headers; int cnt=6;
    if(strcmp(*method,"PUT")==0) {
        cnt=5;
        all_headers=put_headers;
    }

    while (1){
        line = strsep(&input_headers, "\n");
        if (line == NULL || strlen(line) == 0) break;

        char *key = strsep(&line, " ");
        char *value = strsep(&line, ", ");

        printf("Header : %s, Value : %s\n", key, value);

        for (int i = 0; i < cnt; i++) {
            if (strcasecmp(key, all_headers[i]) == 0) {
                values[i] = value;
                break;
            }
        }
    }
}


void send_file(int sockfd, FILE *fp, char *filename) {    
    char *content_type;
    char *ext = strrchr(filename, '.');
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
        content_type="Content-Type: text/html\r\n";
    else if (strcmp(ext, ".txt") == 0)
        content_type="Content-Type: text/plain\r\n";
    else
        content_type="Content-Type: text/*\r\n";
    send(sockfd, content_type, strlen(content_type), 0);

    // Send content length
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char mess_len[100];
    sprintf(mess_len, "Content-Length: %d\r\n", size);
    strcat(mess_len, "\r\n\r\r\n");
    send(sockfd, mess_len, strlen(mess_len), 0);

    // Send file
    char buf[BUFSIZE];
    int sent_size = 0, read_size, total_read_size = 0;
    while ((read_size = fread(buf, 1, BUFSIZE, fp)) > 0) {
        total_read_size += read_size;
        sent_size += send(sockfd, buf, read_size, 0);
    }

    printf("File sent successfully: %s\n", filename);
    printf("Sent %d bytes\n", sent_size);
    printf("Read %d bytes\n", total_read_size);
}

void send_status(int sockfd, int status) {
    char *firstline;

    if (status == 200) {
        firstline = "HTTP/1.1 200 OK\r\n";
    } else if (status == 404) {
        firstline = "HTTP/1.1 404 Not Found\r\n";
    } else if (status == 501) {
        firstline = "HTTP/1.1 501 Not Implemented\r\n";
    } else {
        firstline = "HTTP/1.1 500 Internal Server Error\r\n";
    }

    // send first line
    send(sockfd, firstline, strlen(firstline), 0);

    char date[50];
    time_t now;
    struct tm *timeinfo;

    // send time
    time(&now);
    timeinfo = localtime(&now);
    strftime(date, 50, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", timeinfo);

    char *server = "Server: MyBrowser/1.0\r\n";
    send(sockfd, server, strlen(server), 0);
}


void get_request_body(int sockfd, FILE *fp, int mess_len, char* start_body) {
    char buf[BUFSIZE];
    int sbl = strlen(start_body);
    fwrite(start_body, 1, sbl, fp);
    mess_len -= sbl;
    sbl = 0;

    int read_size, total_read_size = 0;
    while (mess_len > 0) 
    {
        read_size = recv(sockfd, buf, BUFSIZE-1, 0);
        if(read_size<0){
            printf("breaking!\n");
            break;
        }
        total_read_size += read_size;
        buf[read_size-1]='\0';
        printf("RECEIEVED: %s %d\n", buf, read_size);
        fwrite(buf, 1, read_size, fp);
        mess_len -= read_size;
    }
}

void handle_request(int sockfd, char *method,char *path, char* buf, char* start_body, char** values)
{
    char filename[50];
    if (filename != NULL && filename[0] != '.') {
        filename[0] = '.';
        strcpy(filename + 1, path);
    }
    // handle get request
    if(strcmp(method,"GET")==0){
        // put the path in the right format
        printf("Attempting to open file %s\n", filename);
        FILE *fp = fopen(filename, "r");
        // if file not found
        if (fp == NULL) {
            perror("File not found\n");
            send_status(sockfd,404);
            return;
        }
        send_status(sockfd,200);
        send_file(sockfd, fp, filename);
        fclose(fp);
    }

    //handle put request
    if(strcmp(method,"PUT")==0){
        printf("\n\nReceiving: %s\n\n", filename);
        FILE *fp = fopen(filename, "w");
        if (fp == NULL) {
            perror("File not found\n");
            return;
        }
        // receive file content
        int mess_len = atoi(values[3]);
        get_request_body(sockfd, fp, mess_len, start_body);
        fclose(fp);
        send_status(200, sockfd);
    }
}