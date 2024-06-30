#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "zlib.h"
#define HTTP_200_RESPONSE	"HTTP/1.1 200 OK\r\n"
#define HTTP_404_RESPONSE	"HTTP/1.1 404 Not Found\r\n"
#define HTTP_201_RESPONSE	"HTTP/1.1 201 Created\r\n"
#define HTTP_200_CONT		"HTTP/1.1 200 OK"
#define HTTP_CONT_TYPE_TEXT	"Content-Type: text/plain" 
#define HTTP_CONT_TYPE_STREAM	"Content-Type: application/octet-stream"
#define HTTP_CONT_LEN		"Content-Length: "
#define HTTP_USER_AGENT		"User-Agent: "
#define HTTP_ACCEPT_CODEING		"Accept-Encoding: "
#define HTTP_CODING_RESPONSE	"Content-Encoding: "
struct request_t{
	char *url_path;
	char *http_v;
	char *user_agent;
	char *encoding;
	char *text;
};

//============= string process function===================
static char *str_skip_st_to_ed_get(const char *src, const char *s_str, const char *e_str);
static char *str_skip_st(const char *src, const char *s_str);

//========================================================
//============= connect process function==================
static int process_get_request(struct request_t *request);
static int process_post_request(struct request_t *request);
//========================================================

//============= memory process function===================
static void free_struct(struct request_t *r_q);
static void init_struct(struct request_t *r_q);
//========================================================

//============= arg process===============================
static void process_arg(int argc, const char *argv[]);

//========================================================
static char files[256];
static char buf_recv[4096];
static char buf_send[4096];
static char gzip_buff[4096];
static unsigned int gzip_len=4096;
static unsigned int send_len=0;
static char *file_path;
static void connect_handle(int client_fd){
	struct request_t request;
	init_struct(&request);
	request.http_v = NULL;
	memset(buf_recv, '\0', sizeof(buf_recv));
	read(client_fd, buf_recv, sizeof(buf_recv));
	request.url_path = str_skip_st_to_ed_get(buf_recv, "GET ", " ");
	request.encoding = str_skip_st_to_ed_get(buf_recv, HTTP_ACCEPT_CODEING, "\r\n");
	if(request.url_path != NULL){
		process_get_request(&request);
	}
	request.url_path = str_skip_st_to_ed_get(buf_recv, "POST ", " ");
	if(request.url_path != NULL){
		process_post_request(&request);
	}
	write(client_fd, buf_send, send_len);
	// if(strstr(request.encoding, "gzip") != NULL){
	// 	write(client_fd, gzip_buff, gzip_len);
	// }
	free_struct(&request);
	close(client_fd);
}


static int str_get_ch_num(char *str, int len, const char ch)
{
	int ret = 0;
	for(int i=0; i<len; i++){
		if(ch == str[i]){
			ret += 1;
		}
	}
	return ret;
}

int data_compress(char *idata, int ilen, char *odata, int *olen)
{
    z_stream z = {0};

    z.next_in = idata;
    z.avail_in = ilen;
    z.next_out = odata;
    z.avail_out = *olen;
 
    deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8,
               Z_DEFAULT_STRATEGY);

    deflate(&z, Z_FINISH);

    deflateEnd(&z);
 
    *olen = *olen - z.avail_out;

    return 0;
}


static int process_post_request(struct request_t *request)
{
	if(strlen(request->url_path) == 1){

	}else{
		if(strncmp(request->url_path, "/files/", strlen("/files/")) == 0){
			char *urls = str_skip_st(request->url_path, "/files/");
			if(*urls != ' '){
				if(file_path!=NULL){
					printf("url path is %s\n", urls);
					char *url_path = (char*)malloc(strlen(file_path)+strlen(urls)+1);
					memset(url_path, '\0', strlen(file_path)+strlen(urls)+1);
					strcat(url_path, file_path);	/* /tmp/ */
					strcat(url_path, urls);	/* /tmp/file_123 */
					int url_path_len = strlen(url_path);
					int slash_num = str_get_ch_num(urls, strlen(urls), '/');
					char *content_len = str_skip_st_to_ed_get(buf_recv, "Content-Length: ", "\r\n\r\n");
					int file_size = atoi(content_len);
					char *content_text = str_skip_st(buf_recv, "\r\n\r\n");
					printf("slash_num is %d\n", slash_num);
					if(slash_num == 0){
						if(file_size != 0){
							int fd = open(url_path, O_CREAT|O_RDWR);
							if(content_text != NULL){
								if(fd != -1){
									write(fd, content_text, file_size);
								}
							}
							close(fd);
						}
						free(content_len);
					}else{
						int temp_url_len = strlen(url_path);
						if(file_size!=0){
							for(int i=0; i<temp_url_len; i++){
								if(url_path[i] == '/'){
									url_path[i] = '\0';
									mkdir(url_path, 0777);
									url_path[i] = '/';
								}
							}
							int fd = open(url_path, O_CREAT|O_RDWR);
							if(fd!=-1){
								write(fd, content_text, file_size);
							}
							close(fd);
						}
						free(content_len);
					}
					send_len = sprintf(buf_send, "%s\r\n", HTTP_201_RESPONSE);
				}
			}
		}
	}
	
	return 1;
}

static int process_get_request(struct request_t *request)
{
	if(strlen(request->url_path) == 1){
		send_len = sprintf(buf_send, "%s\r\n", HTTP_200_RESPONSE);
	} else{
		//printf("%s\n", request->url_path);
		if(strncmp(request->url_path, "/index.html", strlen("/index.html")) == 0){
			send_len = sprintf(buf_send, "%s\r\n", HTTP_200_RESPONSE);
		}else if(strncmp(request->url_path, "/echo/", strlen("/echo/")) == 0){
			char *echo_str = str_skip_st(request->url_path, "/echo/");
			if(echo_str!=NULL){
				int echo_len = strlen(echo_str);
				if(*echo_str == ' '){
					if(request->encoding!=NULL){
						if(strstr(request->encoding, "gzip") != NULL){
							send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n%s%s\r\n\r\n",HTTP_200_CONT, 
								HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN, HTTP_CODING_RESPONSE, "gzip");
						}else{
							send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n\r\n",HTTP_200_CONT, 
								HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN);
						}
					}else{
						send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n\r\n",HTTP_200_CONT, 
							HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN);
					}
				}else{
					if(request->encoding!=NULL){
						if(strstr(request->encoding, "gzip") != NULL){
							int echo_len = strlen(echo_str);
							data_compress(echo_str, echo_len, gzip_buff, &gzip_len);
							send_len = sprintf(buf_send, "%s\r\n%s\r\n%s%d\r\n%s%s\r\n\r\n",HTTP_200_CONT, 
									HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN, gzip_len, HTTP_CODING_RESPONSE, "gzip");
							for(int i=0;i<gzip_len;i++){
								buf_send[send_len+i] = gzip_buff[i];
							}
							send_len += gzip_len;
							//printf("%s\n", buf_send);
						}else{
							send_len = sprintf(buf_send, "%s\r\n%s\r\n%s%d\r\n\r\n%s",HTTP_200_CONT, 
									HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN, echo_len, echo_str);
						}
					}else{
						send_len = sprintf(buf_send, "%s\r\n%s\r\n%s%d\r\n\r\n%s",HTTP_200_CONT, 
						HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN, echo_len, echo_str);
					}
				}
			}else{
				send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n\r\n",HTTP_200_CONT, 
					HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN);
			}
		}else if(strncmp(request->url_path, "/user-agent", strlen("/user-agent"))==0){
			char *user_agent = str_skip_st_to_ed_get(buf_recv, "User-Agent: ", "\r\n");
			if(user_agent!=NULL){
				if(*user_agent == ' '){
					send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n\r\n",HTTP_200_CONT, 
						HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN);
				}else{
					int user_agent_len = strlen(user_agent);
					send_len = sprintf(buf_send, "%s\r\n%s\r\n%s%d\r\n\r\n%s",HTTP_200_CONT,
								HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN, user_agent_len, user_agent);
				}
			}else{
				send_len = sprintf(buf_send, "%s\r\n%s\r\n%s0\r\n\r\n",HTTP_200_CONT, 
					HTTP_CONT_TYPE_TEXT, HTTP_CONT_LEN);
			}
		}else if(strncmp(request->url_path, "/files/", strlen("/files/"))==0){
			char *file_name = str_skip_st(request->url_path, "/files/");
			//printf("filename = %s\n", file_name);
			if(file_name!=NULL){
				if(*file_name == ' '){
					send_len = sprintf(buf_send, "%s\r\n", HTTP_404_RESPONSE);
				}else{
					int file_len = strlen(file_name);
					if(file_path!=NULL){
						char *file_url_path = (char*)malloc(file_len+strlen(file_path));
						memset(file_url_path, '\0', file_len+strlen(file_path));
						sprintf(file_url_path, "%s%s", file_path, file_name);
						int fd = open(file_url_path, O_RDWR);
						int size = 0;
						if(fd != -1){
							size = lseek(fd, 0, SEEK_END);
							memset(files, '\0', sizeof(files)+1);
							lseek(fd, 0, SEEK_SET);
							read(fd, files, size);
							send_len = sprintf(buf_send, "%s\r\n%s\r\n%s%d\r\n\r\n%s",HTTP_200_CONT,
									HTTP_CONT_TYPE_STREAM,HTTP_CONT_LEN,size,files);
						}else{
							send_len = sprintf(buf_send, "%s\r\n", HTTP_404_RESPONSE);
						}
						free(file_url_path);
						lseek(fd, 0, SEEK_SET);
						close(fd);
					}else{
						send_len = sprintf(buf_send, "%s\r\n", HTTP_404_RESPONSE);
					}
				}
			}else{
				send_len = sprintf(buf_send, "%s\r\n", HTTP_404_RESPONSE);
			}
		}else{
			send_len = sprintf(buf_send, "%s\r\n", HTTP_404_RESPONSE);
		}
	}
}

static void init_struct(struct request_t *r_q){
	r_q->http_v = NULL;
	r_q->text = NULL;
	r_q->url_path = NULL;
	r_q->user_agent = NULL;
	r_q->encoding = NULL;
}

static void free_struct(struct request_t *r_q){
	if(r_q->url_path!=NULL){
		free(r_q->url_path);
	}
	if(r_q->http_v!=NULL){
		free(r_q->http_v);
	}
	if(r_q->text!=NULL){
		free(r_q->text);
	}
	if(r_q->user_agent!=NULL){
		free(r_q->user_agent);
	}
	if(r_q->encoding!=NULL){
		free(r_q->encoding);
	}
}

static void process_arg(int argc, const char *argv[])
{
	char *temp_method = NULL;
	if(argv[argc][1] == '-'){
		temp_method = (char*)&argv[argc][2];
	}else{
		temp_method = (char*)&argv[argc][1];
	}
	if(strcmp(temp_method,"directory") == 0){
		file_path = (char*)argv[argc+1];
	}

}

int main(int argc, const char *argv[]) {
	file_path = NULL;
	for(int i=1; i<argc; i++){
		if(argv[i][0] == '-'){
			process_arg(i, argv);
		}
	}
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
	
	while(1){
		int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		connect_handle(client_fd);
	}

	close(server_fd);

	return 0;
}

/**
 * @brief Set the starting and ending strings and extract the middle content
 * @param src source src
 * @param s_str starting strings
 * @param e_str ending strings
 * @return return the middle string
 */
static char *str_skip_st_to_ed_get(const char *src, const char *s_str, const char *e_str)
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

/**
 * @brief Extract and skip fixed fields from a string(not malloc new memory)
 * @param src Source string
 * @param s_str string skip
 * @return success fetch or fail null
 */
static char *str_skip_st(const char *src, const char *s_str)
{
	char *get_str=NULL;
	int index = 0;
	for(index=0; index<strlen(src); index++){
		if(src[index] == *s_str){
			if(strlen(s_str) == 1){
				break;
			} else if(strlen(s_str) > 1){
				if(strncmp(&src[index], s_str, strlen(s_str)) == 0){
					break;
				}else{
					continue;
				}
			}
		}
	}
	if(index != strlen(src)){
		return (char*)&src[index+strlen(s_str)];
	}else{
		return NULL;
	}
}