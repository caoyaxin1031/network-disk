#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include<unistd.h>


//========================================================================
#define PORT 1000						//定义客户端端口号 
#define SERV_PORT 	1000                   //服务器端口
#define SERV_ADDR 	"192.168.23.128"         //服务器ip
#define SERVERIP "192.168.23.128"			//定义服务器的IP地址
#define CILENTIP "192.168.23.128"			//定义客户端的IP地址
#define LENGTH_OF_LISTEN_QUEUE     20  
#define BUFFER_SIZE                4096  
#define FILE_NAME_MAX_SIZE         512  
#define DEAL AF_INET  //IPv4 网络协议的套接字类型
#define SOCK_TYPE SOCK_STREAM

//========================================================================
#define   N  32

#define  R  1   // user - register
#define  L  2   // user - login
#define  Q  3   // user - query
#define  H  4   // user - history
#define  D  5 
#define  U  6 
char recv_buf[4096];        //定义接收缓存区
char send_buf[4096];        //定义发送缓存区
char *scfile_name;//上传文件名
// 定义通信双方的信息结构体
typedef struct {
	int type;
	char name[N];
	char data[256];
  char file_name[256];//文件名
  char xiaoxi[256];//服务器与客户端之间互传的提示信息
}MSG;

void uploadcilent(int sockfd, MSG *msg);
void downloadcilent(int sockfd, MSG *msg);

int  do_register(int sockfd, MSG *msg)
{
	msg->type = R;

	printf("Input name:");
	scanf("%s", msg->name);
	getchar();

	printf("Input passwd:");
	scanf("%s", msg->data);

	if(send(sockfd, msg, sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}

	if(recv(sockfd, msg, sizeof(MSG), 0) < 0)
	{
		printf("Fail to recv.\n");
		return -1;
	}

	// ok !  or  usr alread exist.
	printf("%s\n", msg->data);

	return 0;
}

int do_login(int sockfd, MSG *msg)
{
	msg->type = L;

	printf("Input name:");
	scanf("%s", msg->name);
	getchar();

	printf("Input passwd:");
	scanf("%s", msg->data);

	if(send(sockfd, msg, sizeof(MSG),0) < 0)
	{
		printf("fail to send.\n");
		return -1;
	}
printf("send success.\n");
	if(recv(sockfd, msg, sizeof(MSG), 0) < 0)
	{
		printf("Fail to recv.\n");
		return -1;
	}
	printf("recv success,msg is:%s\n",msg->xiaoxi);
	if(strncmp(msg->xiaoxi, "OK", 3) == 0)
	{
		printf("Login ok!\n");
		return 1;
	}
	else 
	{
		printf("%s\n", msg->data);
	}

	return 0;
}

int do_query(int sockfd, MSG *msg)//记录上传文件名
{
	msg->type = Q;
	puts("--------------");

	while(1)
	{
		printf("Please input your filename:");
   printf("输入#号返回上一级菜单");
		scanf("%s", msg->file_name);
		getchar();

		//客户端，输入#号，返回到上一级菜单
		if(strncmp(msg->file_name, "#", 1) == 0)
			break;

		//将要查询的单词发送给服务器
		if(send(sockfd,msg, sizeof(MSG), 0) < 0)
		{
			printf("Fail to send.\n");
			return -1;
		}

		// 等待接受服务器，传递保存结果是否成功
		if(recv(sockfd, msg,sizeof(MSG), 0) < 0)
		{
			printf("Fail to recv.\n");
			return -1;
		}
		printf("%s\n", msg->xiaoxi);
    scfile_name=msg->file_name;
   // uploadcilent(scfile_name);
	}
		
	return 0;
}

int do_history(int sockfd, MSG *msg) //查询
{

	msg->type = H;

	send(sockfd, msg, sizeof(MSG), 0);
	
	// 接受服务器，传递回来的历史记录信息
 printf("您账户中的文件列表如下：\n");
	while(1)
	{
  
		recv(sockfd, msg, sizeof(MSG), 0);
    
		if(msg->file_name[0] == '\0')
			break;

		//输出文件信息
		printf("%s\n", msg->file_name);
	}
 	close(sockfd);
//downloadcilent();
	return 0;
}

//=============================================================
void downloadcilent(int sockfd, MSG *msg)
{
		int ret = -1;

	FILE *fp = NULL;  //定义文件操作指针
msg->type = D;

	send(sockfd, msg, sizeof(MSG), 0);
	
 printf("欢迎使用网盘文件下载功能\n");
 
	while(1)
	{
		//3.使用send函数发生数据
		printf("请输入要发送给服务器的内容: \n");
   printf("输入#号返回上一级菜单");
		scanf("%s", send_buf);  
   
		//客户端，输入#号，返回到上一级菜单
		if(strncmp(send_buf, "#", 1) == 0)
			break;
	//	if(!strncmp(send_buf,"+++",3))break;	//输入+++客户端断开连接
		fp = fopen(send_buf, "w");  
		if (fp == NULL)  
		{  
			printf("File:\t%s Can Not Open To Write!\n", send_buf);  
			_exit(-1);  
		}  
		printf("File:\t%s Open Success,Waitting To Write...\n", send_buf); 
		
		ret = send(sockfd, send_buf, strlen(send_buf), 0);
		printf("send buffer:%s,sned len:%d\n",send_buf,ret);
		
		//4.使用recv函数接收来自服务端的消息
		ret = recv(sockfd, recv_buf, sizeof(recv_buf), 0);   
		if(ret < 1)
		{
			printf("服务器断开了连接\n");
			break;
		}
		printf("收到服务器发送的数据:recv buffer:\n%s\nrecv len:%d\n",recv_buf,ret);
		printf("将接收到的数据写入文件中:\n");
		//调用fwrite函数将recv_buf缓存中的数据写入文件中
		int write_length = fwrite(recv_buf, sizeof(char), ret, fp);  
        if (write_length < ret)  
        {  
            printf("文件写入失败!\n");  
            break;  
        }  
		printf("Recieve File:\t %s From Server[%s] 接收成功!\n", send_buf, SERV_ADDR);  
		memset(send_buf,0,sizeof(send_buf));   //清空接收缓存区
		memset(recv_buf,0,sizeof(recv_buf));   //清空接收缓存区
		fclose(fp);  
	}
	printf("关闭连接并退出\n");
	close(sockfd);     //关闭socket文件描述符

} 
 


//============================================================================================


void uploadcilent(int sockfd, MSG *msg){
msg->type = U;
//printf("请输入要发送给服务器的内容: \n");
//		scanf("%s", msg->file_name);  
//send(sockfd, msg, sizeof(MSG), 0);	

		FILE *fp = NULL;
    int length;
  
	char buffer[BUFFER_SIZE];  
    while(1)  
    {  
        printf("请输入要发送给服务器的内容: \n");
         printf("输入#号返回上一级菜单");
		scanf("%s", send_buf);  
   
		//客户端，输入#号，返回到上一级菜单
		if(strncmp(send_buf, "#", 1) == 0)
			break;  
        send(sockfd, msg, sizeof(MSG), 0);	
        bzero(buffer, sizeof(buffer));  //bzero:将内存块（字符串）的前n个字节清零
        strncpy(msg->file_name, buffer,strlen(buffer) > FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));  
  
        fp = fopen(msg->file_name, "r");  
        if (fp == NULL)  
        {  
            printf("File:\t%s Not Found!\n", msg->file_name);  
        }  
        else  
        {  
			printf("File:\t%s open success!\n", msg->file_name);  
            bzero(buffer, BUFFER_SIZE);  
            int file_block_length = 0;  
	//循环将文件file_name(fp)中的内容读取到buffer中
			
			
            while((file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)  //
            {  
                printf("读取到的文件长度file_block_length = %d\n", file_block_length);  
  
                // 发送buffer中的字符串到new_server_socket,实际上就是发送给客户端  
                if (send(sockfd, buffer, file_block_length, 0) < 0)  
                {  
                    printf("Send File:\t%s Failed!\n", msg->file_name);  
                    printf("Send :\t%s\tsuccess-----! \n", buffer);
                    break;
                }  
				//清空buffer缓存区
                bzero(buffer, sizeof(buffer));  
            }  
            fclose(fp);  			//关闭文件描述符fp
            printf("File:\t%s Transfer Finished!\n", msg->file_name);  
        } //break;

    }  
    close(sockfd);  			//关闭socket文件描述符		
   
} 



//============================================================================================



//========================================================================================







// ./server  192.168.3.196  10000
int main(int argc, const char *argv[])
{

	int sockfd;
	struct sockaddr_in  serveraddr;
	int n;
	MSG  msg;
//downloadcilent();

	if(argc != 3)
	{
		printf("Usage:%s serverip  port.\n", argv[0]);
		return -1;
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

	if(connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("fail to connect");
		return -1;
	}

	while(1)
	{
		printf("*****************************************************************\n");
		printf("* 1.注册            2.登录                 3.退出               *\n");
		printf("*****************************************************************\n");
		printf("Please choose:");

		scanf("%d", &n);
		getchar();

		switch(n)
		{
		case 1:
   	
  // downloadcilent();
			do_register(sockfd, &msg);
			break;
		case 2:
			if(do_login(sockfd, &msg) == 1)
			{
				goto next;
			}
			break;
		case 3:
			close(sockfd);
			exit(0);
			break;
		default:
			printf("Invalid data cmd.\n");
		}

	}

next:
	while(1)
	{
		printf("*****************************************************\n");
		printf("* 1.网盘文件名更新   2.查看网盘文件名 5.文件下载 6.文件上传  7.退出          *\n");
		printf("*****************************************************\n");
		printf("Please choose:");
		scanf("%d", &n);
		getchar();

		switch(n)
		{
			case 1:
				do_query(sockfd, &msg);
              
        
				break;
			case 2:
				do_history(sockfd, &msg);
        
        
				break;       
			case 7:
				close(sockfd);
				exit(0);
				break;
      case 5:
			  downloadcilent(sockfd, &msg);
		    break;
       case 6:
			  uploadcilent(sockfd, &msg);
		    break;
	default :
			printf("Invalid data cmd.\n");
		}

	}
	
	return 0;
}

