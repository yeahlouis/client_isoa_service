#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>



#define MAXLINE 4096
#define ONE_SEC 1000000 /* the one second */



#define SOCK_ERR -1


#define _TRC(pstr) printf(pstr)

#define _ERR(pstr) fprintf(stderr, pstr)


enum {
      bPACK_FAILED            = 0x0000000001,         //调用的应用函数自身出错
      bPACK_CATCH_ERR         = bPACK_FAILED << 1,    //系统扑获到了异常
      bPACK_ZIP               = bPACK_FAILED << 2,    //1压缩, 0未压缩,
      bPACK_CRYPTO            = bPACK_FAILED << 3,    //1加密, 0未加密

      bPACK_MAC               = bPACK_FAILED << 4,    //1是否作MAC验证, 验证必须在加密压缩后进行，mac不能压缩
                                                      //是对<head>.size的验证而不是osize
                                                      //头:{flag/size/osize/mac[130]}
      bPACK_NEXTREQ           = bPACK_FAILED << 5,    //1是否有下一个请求也在此连接发送,
      bPACK_NEXT              = bPACK_FAILED << 6,    //1是否有下一个数据buffer,
      bPACK_SPORT             = bPACK_FAILED << 7,    //1是否使用了ISOA.sport传输请求或应答数据文件
      bPACK_DETECTOR          = bPACK_FAILED << 8,    //探测消息, 只有头无附加数据，反馈flag＝0即表示状态ok
      bPACK_ASYNC             = bPACK_FAILED << 9,    //异步通讯
      bPACK_SYNC              = bPACK_FAILED << 10,   //同步通讯
      bPACK_REACT             = bPACK_FAILED << 11,   //获取应答请求, 连接中介时使用，请求服务和获取应答均在一个端口完成所以要区分。
                                                      //如果中介也采取分开模式则无须此位，但还是用一个守护进程吧，将来再说。
                                                      //为保持一致，直接连接ISOA获取应答时也要设置此位，否则视为错误。
                                                      //目的是使得客户端无论通过中介还是直连ISOA采取一致的方式。
      bPACK_BUSY              = bPACK_FAILED << 12,   //系统忙，拒绝服务
      bPACK_MAXBIT            = bPACK_FAILED << 20,   //  
};



enum {
  L_MAC                 = 130,
  L_USER                 = 64,
  L_SERVICE              = 128,
  L_IP                    = 40, 
  L_PORT                  = 128,
  L_HOST                  = 64,       //主机名
  L_HEAD                  = sizeof(int32_t) * 3 + L_MAC,
  L_VERSION             = 12
};


// ISOA_socket_recv、send().flag:
enum {
  bCHECK_MAC     = 0x01,
  bZIP     = 0x02,
};





typedef struct{
  int32_t     flag;
  int32_t     size;
  int32_t     osize;
  char        mac[L_MAC];
}__pack_head;

typedef struct{
  char        user[L_USER];
  char        service[L_SERVICE];
  int32_t     size_contents;
  char        sz_contents[MAXLINE];
}__pack_request;


struct own_entr_content
{
  char sz_port[16];
  char sz_ip[256];
  char sz_server[L_SERVICE];
  char sz_contents[MAXLINE];

};





int tcp_socket_init(int *host_sockfd, char *host_ip, char *host_port);
