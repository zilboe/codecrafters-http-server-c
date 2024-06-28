#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#define HTTP_200_RESPONSE	"HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_404_RESPONSE	"HTTP/1.1 404 Not Found\r\n\r\n"
struct request_t{
	int method;	//1-GET 2-POST
	char *url_path;
	char *http_v;
	char *user_agent;

	char *text;
};

static char buf_recv[128];
static char buf_send[128];
static int send_len;

/**
 * @brief Set the starting and ending strings and extract the middle content
 * @param src source src
 * @param s_str starting strings
 * @param e_str ending strings
 * @return return the middle string
 */
static char *str_skip_get(const char *src, const char *s_str, const char *e_str)
{
	unsigned int i=0, j=0;
	int index=0;
	char *get_str=NULL;
	char *sstr=NULL, *eend=NULL;
	for(index=0; index<strlen(src); index++){
		if(src[index] == *s_str){
			if(strlen(s_str) == 1){
				i=index;
				break;
			} else if(strlen(s_str) > 1){
				if(strncmp(&src[index], s_str, strlen(s_str)) == 0){
					i=index;
					break;
				}else{
					continue;
				}
			}
		}
	}
	if(i==index){
		index+=strlen(s_str);
		i = index;
	} else{
		i = ~(0);
	}
	for(; index<strlen(src); ++index){
		if(src[index] == *e_str){
			if(strlen(e_str) == 1){
				j=index;
				break;
			} else if(strlen(e_str)>1){
				if(strncmp(&src[index], e_str, strlen(e_str)) == 0){
					j=index;
					break;
				}else{
					continue;
				}
			}
		}
	}
	
	if(i != -1 && j != -1 && j > i){
		get_str = (char*)malloc(j-i+1);
		memset(get_str, '\0', j-i+1);
		strncpy(get_str, &src[i], j-i);
	} else if(i==j && i!= -1){
		get_str = (char*)malloc(2);
		*get_str = src[i];
		*(get_str+1) = '\0';
	}
	return get_str;

}

static void connect_handle(int client_fd){
	struct request_t request;
	char *urlpath = NULL;
	read(client_fd, buf_recv, sizeof(buf_recv));
	request.url_path = str_skip_get(buf_recv, "GET ", " ");
	if(request.url_path != NULL){
		request.method = 1;
		if(strlen(request.url_path) == 1){
			send_len = sprintf(buf_send, "%s", HTTP_200_RESPONSE);
		} else{
			//printf("%ld, %s\n",strlen(request.url_path), request.url_path);
			if(strncmp(request.url_path, "/index.html", strlen("/index.html")) == 0){
				send_len = sprintf(buf_send, "%s", HTTP_200_RESPONSE);
			}else{
				send_len = sprintf(buf_send, "%s", HTTP_404_RESPONSE);
			}
		}
	}
	
	write(client_fd, buf_send, send_len);
	close(client_fd);
}
int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage
	
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");
	
	connect_handle(client_fd);

	close(server_fd);

	return 0;
}
