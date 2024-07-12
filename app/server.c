#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

struct Request {
    int32_t method;
    int32_t version;
    char* path;
    uint64_t path_len;
    char* headers;
    uint64_t headers_len;
    uint64_t headers_capacity;
};

int streqp(char* a, int al, char* b, int bl) {
    if (al != bl) {
        return 0;
    }
    for (int i = 0; i < al; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

int strprefix(char* a, int al, char* b, int bl) {
    if (al < bl) {
        return 0;
    }
    for (int i = 0; i < bl; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

char* format_uint(char* buf, int i) {
    if (i == 0) {
        *buf = '0';
        return buf++;
    }
    while (i > 0) {
        *buf = (i % 10) + '0';
        i = i / 10;
        buf++;
    }
    return buf;
}

char* strdupp(char* to, char* from, int from_len) {
    while (from_len > 0) {
        *to = *from;
        to++;
        from++;
        from_len--;
    }
    return to;
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

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

	int client = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client < 0) {
        printf("Client failed\n");
        return 1;
    }

    char rbuf[1024];
    int size = read(client, rbuf, 1024);
    char* buf = rbuf;
    struct Request req;
    char* end = buf+size;
    char GET[] = "GET";
    char POST[] = "POST";
    char* start = buf;
    while (1) {
        if (buf == end) {
            goto bad_request;
        }
        buf++;
        if (*(buf-1) == ' ') {
            break;
        }
    }
    if (streqp(start, buf-start-1, GET, sizeof(GET)-1)) {
        req.method = 1;
    } else if (streqp(start, buf-start-1, POST, sizeof(POST)-1)) {
        req.method = 1;
    } else {
        // Unknown method
        req.method = -1;
    }

    // path
    start = buf;
    while (1) {
        if (buf == end) {
             goto bad_request;
        }
        buf++;
        if (*(buf-1) == ' ') {
            break;
        }
    }
    if (buf-1 == start) {
        goto bad_request;
    }
    req.path_len = buf-1-start;
    req.path = (char*) malloc(buf-1-start);
    strdupp(req.path, start, req.path_len);

    // version
    start = buf;
    while (1) {
        if (buf == end) {
             goto bad_request;
        }
        buf++;
        if (*(buf-1) == '\r') {
            break;
        }
    }

    char VERSION[] = "HTTP/1.1";
    if (streqp(start, buf-1-start, VERSION, sizeof(VERSION)-1)) {
        req.version = 1;
    } else {
        req.version = -1;
    }

    if (buf == end || *buf != '\n') {
        goto bad_request;
    }
    buf++;

    // TODO: headers

    char msg200[] = "HTTP/1.1 200 OK\r\n\r\n";
    char msg404[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    char* msg = msg404;
    int msg_len = sizeof(msg404)-1;
    if (req.path_len == 1 && *req.path == '/') {
        msg = msg200;
        msg_len = sizeof(msg200) - 1;
    } else if (strprefix(req.path, req.path_len, "/echo/", 6)) {
        char m[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";
        char* tmp = (char*) malloc(sizeof(m)-1+20+4+req.path_len-6);
        msg = tmp;
        tmp = strdupp(tmp, m, sizeof(m) - 1);
        tmp = format_uint(tmp, req.path_len-6);
        *tmp = '\r';
        *(tmp+1) = '\n';
        *(tmp+2) = '\r';
        *(tmp+3) = '\n';
        tmp += 4;
        tmp = strdupp(tmp, req.path+6, req.path_len-6);
        msg_len = tmp-msg;
    }
    printf("%d\n", msg_len);
    write(client, msg, msg_len);
    close(client);

	close(server_fd);

	return 0;

bad_request:
    printf("Bad request\n");
    return 1;
}
