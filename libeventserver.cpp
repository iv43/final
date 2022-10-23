#include<event2/listener.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<string>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<thread>
#include<unistd.h>
#include<algorithm>
#include<fstream>
#include<arpa/inet.h>
std::string directory = "./"; 
static void read_cb(struct bufferevent*bev, void *ctx){
//printf("read_cb\n");
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

//узнать размер данных для чтения
	size_t len = evbuffer_get_length(input);
	char* data{};
	data =(char*) malloc(len);
	//evbuffer_copyout(input, data, len);//прочитать не удаляя из input
	evbuffer_remove(input, data, len);//прочитать и удалить из input
	std::string response(data);
	//char* end_data = &data[len];
	//printf("data: %s\n", data);

 	int start = response.find("GET");
	
	if(start == 0){
		int version = response.find("HTTP/");
	
		std::string file_name = response.substr(4, version-5);
		
		file_name = directory+file_name;	
		//printf("file name:!%s!\n", file_name.c_str());
			FILE* file = fopen(file_name.c_str(), "r");	
			if(file != NULL){
				evbuffer_add(output, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n", 42);
				evbuffer_add_file(output, fileno(file), 0, -1);
sleep(5);
				fclose(file);
			}else{
				evbuffer_add(output, "HTTP/1.0 404 file not found\r\n",28);
				
			}
	}
	else{
		evbuffer_add(output, "HTTP/1.0 404 bad request\r\n", 25);
	}
		

	free(data);
}
static void work_with_client_cb(struct bufferevent*bev, void *ctx){

//printf("work_with_client_cb, bufferevevt: %p\n",bev);
std::thread t(read_cb, std::ref(bev), std::ref(ctx));
t.detach();
}
//callback для событий, таких как закрытие соедининия 
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx){
//printf("echo_event_cb\n");
	if(events & BEV_EVENT_ERROR){
		perror("error");
		bufferevent_free(bev);
	}
	if(events & BEV_EVENT_EOF){
		bufferevent_free(bev);
	}
}
//connection_callback
static void accept_conn_cb(struct evconnlistener *listner,  
				evutil_socket_t fd, struct sockaddr* sockaddr,
				int socklen, 
				void*ctx)
{
//printf("accept_conn_cb\n");
	struct event_base *base =evconnlistener_get_base(listner);
	struct bufferevent *bev = bufferevent_socket_new(base, fd,
					BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, work_with_client_cb, NULL, echo_event_cb, NULL);//регистрация work_with_client_cb и echo_event_cb 
	bufferevent_enable(bev, EV_READ | EV_WRITE);

}


static void accept_error_cb(struct evconnlistener *listner,	void*ctx){

	struct event_base *base = evconnlistener_get_base(listner);
	
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "error = %d = \"%s\"\n", errno,
				   	evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}

int main(int argc, char* argv[]){
daemon(1,1);

const char* opts="h:p:d:";
int a=0;
char*ip;
int port;
char*dir;
while ((a=getopt(argc, argv, opts))!=-1){
	switch(a){
	case 'h':
		ip=optarg;
		break;
	case 'p':
		port = atoi(optarg);
		break;
	case 'd':
		dir=optarg;
		break;
	} 
}
directory = dir;
//создание сновного event_base
	struct event_base *base = event_base_new();

int masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
struct sockaddr_in sockaddr;
sockaddr.sin_family = AF_INET;
sockaddr.sin_port = htons(port);

unsigned addr=0;
inet_pton(AF_INET, ip, &addr);//адрес в число
sockaddr.sin_addr.s_addr = htonl(addr);//htonl меняет порядок следования байт
//bind(masterSocket, (struct sockaddr*)(&sockaddr), sizeof(sockaddr) );//не надо, libevent сам делает


	struct evconnlistener *listner = evconnlistener_new_bind(base,
					accept_conn_cb, NULL, 
					LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
					-1, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

    if (!listner) {
        perror("Couldn't create listener");
        return 1;
    }
//установка error_callback
	evconnlistener_set_error_cb(listner, accept_error_cb);
//event_base_loop с флагом 0 - запуск цикла
	event_base_dispatch(base);




	return 0;
}
