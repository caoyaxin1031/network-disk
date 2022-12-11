#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>


#define   N  32

#define  R  1   // user - register
#define  L  2   // user - login
#define  Q  3   // user - query
#define  H  4   // user - history
#define  D  5 //down 
#define  U  6 
#define  DATABASE  "my.db"

//============================================================
#define PORT 1000						//定义服务器端口号
#define CILENT_PORT 	1000                   //客户端端口号
#define SERVERIP "192.168.23.128"			//定义服务器的IP地址
#define CILENTIP "192.168.23.128"			//定义客户端的IP地址
#define LENGTH_OF_LISTEN_QUEUE     20  
#define BUFFER_SIZE                4096  
#define FILE_NAME_MAX_SIZE         512  
char recv_buf[4096];        //定义接收缓存区
char send_buf[4096];        //定义发送缓存区
//char *scfile_name;
//============================================================


// 定义通信双方的信息结构体
typedef struct {
	int type;
	char name[N];
	char data[256];
  char file_name[256];//文件名
  char xiaoxi[256];//服务器与客户端之间互传的提示信息
}MSG;



int do_client(int acceptfd, sqlite3 *db);
void do_register(int acceptfd, MSG *msg, sqlite3 *db);
int do_login(int acceptfd, MSG *msg, sqlite3 *db);
int do_query(int acceptfd, MSG *msg, sqlite3 *db);
int do_history(int acceptfd, MSG *msg, sqlite3 *db);
int history_callback(void* arg,int f_num,char** f_value,char** f_name);
int do_searchword(int acceptfd, MSG *msg, char word[]);
int get_date(char *date);

void uploadserver(int acceptfd, MSG *msg, sqlite3 *db);
void downloadserver(int acceptfd, MSG *msg, sqlite3 *db);
// ./server  192.168.3.196  10000
int main(int argc, const char *argv[])
{



	int sockfd;
	struct sockaddr_in  serveraddr;
	int n;
	MSG  msg;
	sqlite3 *db;
	int acceptfd;
	pid_t pid;

	if(argc != 3)
	{
		printf("Usage:%s server ip  port.\n", argv[0]);
		return -1;
	}

	//打开数据库
	if(sqlite3_open(DATABASE, &db) != SQLITE_OK)
	{
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}
	else
	{
		printf("open DATABASE success.\n");
	}

	if((sockfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror("fail to socket.\n");
		return -1;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));

	if(bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to bind.\n");
		return -1;
	}

	// 将套接字设为监听模式
	if(listen(sockfd, 5) < 0)
	{
		printf("fail to listen.\n");
		return -1;
	}

	//处理僵尸进程
	signal(SIGCHLD, SIG_IGN);

	while(1)
	{
		if((acceptfd = accept(sockfd, NULL, NULL)) < 0)
		{
			perror("fail to accept");
			return -1;
		}

		if((pid = fork()) < 0)
		{
			perror("fail to fork");
			return -1;
		}
		else if(pid == 0)  // 儿子进程
		{
			//处理客户端具体的消息
			close(sockfd);
      
			do_client(acceptfd, db);

		}
		else  // 父亲进程,用来接受客户端的请求的
		{
			close(acceptfd);
		}
	}

}

int do_client(int acceptfd, sqlite3 *db)
{
	MSG msg;
	while(recv(acceptfd, &msg, sizeof(msg), 0) > 0)
	{
	  printf("type:%d\n", msg.type);
	   switch(msg.type)
	   {
	  	 case R:
        do_register(acceptfd, &msg, db);//注册
			 break;
		 case L:
			 do_login(acceptfd, &msg, db);//登录
			 break;
		 case Q:
			 do_query(acceptfd, &msg, db);//上传文件
       
			 break;
		 case H:
			 do_history(acceptfd, &msg, db);//网盘文件查看及下载
      
			 break;
     case D:
       downloadserver(acceptfd,&msg, db); 
       break;
    case U:
       uploadserver(acceptfd,&msg, db);
       break;
		 default:
			 printf("Invalid data msg.\n");
	   }
       if(msg.type==775041586)
      { printf("下载文件已执行结束\n");
     // downloadserver();
       break;
       }

	}

	printf("client exit.\n");
	close(acceptfd);
	exit(0);

	return 0;
}

void do_register(int acceptfd, MSG *msg, sqlite3 *db)
{
	char * errmsg;
	char sql[128];

sprintf(sql, "create table if not exists usr (name text,pass text);");//创建表usr
	printf("%s\n", sql);
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		strcpy(msg->xiaoxi, "usr table create failed");
	}
	else
	{
		printf("usr table create success\n");
		strcpy(msg->xiaoxi, "OK!");
	}
	sprintf(sql, "insert into usr values('%s','%s');", msg->name, msg->data);
	printf("%s\n", sql);

	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		strcpy(msg->xiaoxi, "usr name already exist.");
	}
	else
	{
		printf("client  register ok!\n");
		strcpy(msg->xiaoxi, "OK!");
	}

	if(send(acceptfd, msg, sizeof(MSG), 0) < 0)
	{
		perror("fail to send");
		return ;
	}

	return ;
}

int do_login(int acceptfd, MSG *msg , sqlite3 *db)
{
	char sql[128] = {};
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;

	sprintf(sql, "select * from usr where name = '%s' and pass ='%s';", msg->name, msg->data);
	printf("%s\n", sql);

	if(sqlite3_get_table(db, sql, &resultp, &nrow, &ncloumn, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	else
	{
		printf("get_table ok!\n");
   printf("search hangshu:%d\n",nrow);
	}

	// 查询成功，数据库中拥有此用户
	if(nrow >= 1)
	{
		strcpy(msg->xiaoxi, "OK");
		send(acceptfd, msg, sizeof(MSG), 0);
		return 0;
	}

	if(nrow == 0) // 密码或者用户名错误
	{
		strcpy(msg->xiaoxi,"usr/passwd wrong.");
		send(acceptfd, msg, sizeof(MSG), 0);
	}

	return 0;
}



int get_date(char *date)
{
	time_t t;
	struct tm *tp;

	time(&t);

	//进行时间格式转换
	tp = localtime(&t);

	sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year + 1900, tp->tm_mon+1, tp->tm_mday, 
			tp->tm_hour, tp->tm_min , tp->tm_sec);
	printf("get date:%s\n", date);

	return 0;
}


int do_query(int acceptfd, MSG *msg , sqlite3 *db)//记录上传文件名在record表中
{
	char filename[64];
	int found = 0;
	char date[128] = {};
	char sql[128] = {};
	char *errmsg;
  int nrow;
	int ncloumn;
  char **resultp;
	//拿出msg结构体中上传的文件名
	strcpy(filename, msg->file_name);
		// 需要获取系统时间
		get_date(date);
   
   sprintf(sql, "create table if not exists record (name text,date text,filename text);");//创建表record(name,text,filename)
	printf("%s\n", sql);
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		strcpy(msg->xiaoxi, "record table create failed");
	}
	else
	{
		printf("record table create success\n");
		strcpy(msg->xiaoxi, "OK!");
	}

  sprintf(sql, "insert into record values('%s', '%s', '%s')", msg->name, date,msg->file_name);//将文件名插入record表里
  printf("%s\n", sql);
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
 	} 
	else
	{
		printf("Insert record success.\n");
    send(acceptfd, msg, sizeof(MSG), 0);
   
   }
  //	 uploadserver(msg->file_name); 
   	
	
  return 0;


}
   
 


// 得到查询结果，并且需要将历史记录发送给客户端
int history_callback(void* arg,int f_num,char** f_value,char** f_name)
{
	// record  , name  , date  , word 
	int acceptfd;
	MSG msg;

	acceptfd = *((int *)arg);

	sprintf(msg.file_name, "%s , %s", f_value[1], f_value[2]);

	send(acceptfd, &msg, sizeof(MSG), 0);

	return 0;
}


int do_history(int acceptfd, MSG *msg, sqlite3 *db)
{
	char sql[128] = {};
	char *errmsg;

	sprintf(sql, "select * from record where name = '%s'", msg->name);

	//查询数据库
	if(sqlite3_exec(db, sql, history_callback,(void *)&acceptfd, &errmsg)!= SQLITE_OK)
	{
		printf("%s\n", errmsg);
	}
	else
	{
		printf("Query record done.\n");
	}

	// 所有的记录查询发送完毕之后，给客户端发出一个结束信息
	msg->data[0] = '\0';

	send(acceptfd, msg, sizeof(MSG), 0);
  puts("已输出完毕");
  close(acceptfd);
 // downloadserver();
	return 0;
 
}

//=================================================================================================
void downloadserver(int acceptfd, MSG *msg, sqlite3 *db)
{
			FILE *fp = NULL;
      int length;
    // 服务器端一直运行用以持续为客户端提供服务   
	char buffer[BUFFER_SIZE];  
    while(1)  
    {  
        bzero(buffer, sizeof(buffer));  //bzero:将内存块（字符串）的前n个字节清零
		printf("等待客户端发送过来的文件名：\n");
        length = recv(acceptfd, buffer, BUFFER_SIZE, 0);  
    		if(length > 0)
    		{
    			printf("接收client消息成功:'%s'，共%d个字节的数据\n", buffer, length);
    		}
        else
        {  
            printf("客户端断开了连接，退出！！！\n");  
            break;  
        }  
        char file_name[FILE_NAME_MAX_SIZE + 1];  
        bzero(file_name, sizeof(file_name));  
        strncpy(file_name, buffer,strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));  
  
        fp = fopen(file_name, "r");  
        if (fp == NULL)  
        {  
            printf("File:\t%s Not Found!\n", file_name);  
        }  
        else  
        {  
			printf("File:\t%s open success!\n", file_name);  
            bzero(buffer, BUFFER_SIZE);  
            int file_block_length = 0;  
			//循环将文件file_name(fp)中的内容读取到buffer中
			
			
            while((file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)  //
            {  
                printf("读取到的文件长度file_block_length = %d\n", file_block_length);  
  
                // 发送buffer中的字符串到new_server_socket,实际上就是发送给客户端  
                if (send(acceptfd, buffer, file_block_length, 0) < 0)  
                {  
                    printf("Send File:\t%s Failed!\n", file_name);  
                    break;  
                }  
				//清空buffer缓存区
                bzero(buffer, sizeof(buffer));  
            }  
            fclose(fp);  			//关闭文件描述符fp
            printf("File:\t%s Transfer Finished!\n", file_name);  
          } 
       length = recv(acceptfd, buffer, BUFFER_SIZE, 0);  
      		if(length > 0)
      		{
      			printf("接收client消息成功:'%s'，共%d个字节的数据\n", buffer, length);
      		}
          else
          {  
              printf("客户端断开了连接，退出！！！\n");  
              break;  
          } 
           
    }  
	

  
     close(acceptfd);  		//关闭accept文件描述符
}


//=================================================================================================
void uploadserver(int acceptfd, MSG *msg, sqlite3 *db)
{
		int ret = -1;

	FILE *fp = NULL;  //定义文件操作指针
	

 char *scfile_name;  
    scfile_name=msg->file_name; 
	//下面客户端和服务器互相收发
	while(1)
	{
		//3.使用send函数发生数据
		printf("文件:%s\t将被服务器接收",scfile_name);
	
		fp = fopen(msg->file_name, "w");  
    		if (fp == NULL)  
    		{  
    			printf("File:\t%s Can Not Open To Write!\n", scfile_name);  
    			_exit(-1);  
    		}  
		printf("File:\t%s Open Success,Waitting To Write...\n", scfile_name); 
		

		
		//4.使用recv函数接收来自服务端的消息
		ret = recv(acceptfd, recv_buf, sizeof(recv_buf), 0);   
    		if(ret < 1)
    		{
    			printf("服务器断开了连接\n");
    			break;
    		}
    		printf("收到服务器发送的数据:recv buffer:\n%s\nrecv len:%d\n",recv_buf,ret);
    	//	printf("将接收到的数据写入文件中:\n");
    		//调用fwrite函数将recv_buf缓存中的数据写入文件中
    		int write_length = fwrite(recv_buf, sizeof(char), ret, fp);  
        if (write_length < ret)  
        {  
            printf("文件写入失败!\n");  
            break;  
        }  
		printf("Recieve File:\t %s From Server[%s] 接收成功!\n", scfile_name, CILENTIP);  
	memset(send_buf,0,sizeof(send_buf));   //清空接收缓存区
		memset(recv_buf,0,sizeof(recv_buf));   //清空接收缓存区
		fclose(fp); 
  	}
 
	printf("关闭连接并退出\n");
	close(acceptfd);     //关闭socket文件描述符
}

 

