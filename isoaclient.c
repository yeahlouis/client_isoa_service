#include "isoaclient.h"
#include <errno.h>


static int my_isspace(int ch) 
{
  if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') return 1;
  return 0;
}

static char *find_not_space(char *src)
{
  char *p;
  int ch;
  if(NULL == src || strlen(src) <= 0) {
	return NULL;
  }
  for (p = src; *p != '\0'; p++) {
	ch = *p;
	if (0 == my_isspace(ch)) return p;
  }
  return src;
}

static int trim(char *src)
{
  int ch;
  size_t size;
  int i;
  if(NULL == src || (size = strlen(src)) <= 0) {
	return 0;
  }
  for (i = size - 1; i >= 0; i--) {
	ch = src[i];
	if (0 == my_isspace(ch)) return 0;
	src[i] = '\0';
  } 
  return 0;
}


static int parse_conf(FILE *f, struct own_entr_content *pcontent)
{
  char sz_file_buf[8192];
  char *sz_pkey, *sz_pval;

  if (NULL == f || NULL == pcontent) {
	return -1;
  }


  while(fgets(sz_file_buf, sizeof(sz_file_buf), f)) {
	//	printf("before.line = [%s]\n",sz_file_buf);
	if('#' == sz_file_buf[0]) {
	  continue;
	}
	sz_pkey = find_not_space(sz_file_buf);
	if (strlen(sz_pkey) <= 0 || '#' == *sz_pkey) {
	  continue;
	}
	//	printf("after..line = [%s]\n",sz_pkey);
	
	sz_pval = strchr(sz_pkey, '=');
	if (NULL == sz_pval) {
	  continue;
	}
	*sz_pval++ = '\0';
	trim(sz_pkey);
	sz_pval = find_not_space(sz_pval);
	trim(sz_pval);
	//	printf("key:val = [%s] : [%s]\n", sz_pkey, sz_pval);
	if (0 == strcmp("ip", sz_pkey)) {
	  snprintf(pcontent->sz_ip, sizeof(pcontent->sz_ip), sz_pval);
	} else if (0 == strcmp("port", sz_pkey)) {
	  snprintf(pcontent->sz_port, sizeof(pcontent->sz_port), sz_pval);
	} else if (0 == strcmp("server", sz_pkey)) {
	  snprintf(pcontent->sz_server, sizeof(pcontent->sz_server), sz_pval);
	} else if (0 == strcmp("contents", sz_pkey)) {
	  snprintf(pcontent->sz_contents, sizeof(pcontent->sz_contents), sz_pval);
	}

  }
  printf("sz_ip:sz_port = [%s:%s]\n", pcontent->sz_ip, pcontent->sz_port);
  printf("sz_server = [%s]\n", pcontent->sz_server);
  printf("sz_contents = [%s]\n", pcontent->sz_contents);
  

  if (strlen(pcontent->sz_ip) <= 0 || strlen(pcontent->sz_port) <= 0 
	  || strlen(pcontent->sz_server) <= 0 || strlen(pcontent->sz_contents) <= 0) {
	return -1;
  } 
  return 0;
}



ssize_t         /* read n bytes from a descriptor */
readn(int fd, void *vptr, size_t n)
{ 
  size_t  nleft;
  ssize_t nread; 
  char *ptr;
    
  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
	if ( (nread = read(fd, ptr, nleft)) < 0) {
	  if (errno == EINTR) nread = 0; /* and call read() again */
	  else return (-1);
	} else if (nread == 0) {
	  break;    /* EOF */
	}
	nleft -= nread;
	ptr += nread;
  } 
  return (n - nleft);  /* return >= 0 */
} 



int tcp_socket_init(int *host_sockfd, char *host_ip, char *host_port)
{
  int ret = 0;
  int sockfd;
#if 0
  int count, nsec;
#endif


  struct sockaddr_in servaddr;
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  fprintf(stderr, "socket error!\n");
	  return SOCK_ERR;
  }


  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(host_port));
  if (inet_pton(AF_INET, host_ip, &servaddr.sin_addr) <= 0) {
	fprintf(stderr, "inet_pton error!\n");
	close(sockfd);
	return SOCK_ERR;
  }

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	fprintf(stderr, "connect error:%m\n");
	close(sockfd);
	return SOCK_ERR;
  }

  *host_sockfd = sockfd;
  return ret;
}


int tcp_socket_send(int sockfd, char *sz_server, char *sz_contents)
{

  char buf[MAXLINE];
  int len, len_head;
  int32_t n, tmp;
  __pack_head head;
  __pack_request request;

  memset(&head, 0x00, sizeof(head));
  memset(&request, 0x00, sizeof(request));
  memset(buf, 0x00, sizeof(buf));

  
  /* head.osize = head.size = sizeof(head); */
  len_head = L_HEAD;

  snprintf(request.service, sizeof(request.service), sz_server);
  snprintf(request.sz_contents, sizeof(request.sz_contents), sz_contents);
  request.size_contents = strlen(request.sz_contents);
  head.size = sizeof(request.user) + sizeof(request.service) + sizeof(request.size_contents) + request.size_contents;
  head.osize = head.size;
  
  len = 0;
  tmp = htonl(head.flag);
  memcpy(buf + len, &tmp, sizeof(int32_t));
  len += sizeof(int32_t);
  tmp = htonl(head.size);
  memcpy(buf + len, &tmp, sizeof(int32_t));

  len += sizeof(int32_t);
  tmp = htonl(head.osize);
  memcpy(buf + len, &tmp, sizeof(int32_t));

  len = len_head;


  strcpy(buf + len, request.user);
  len += sizeof(request.user);

  strcpy(buf + len, request.service);
  len += sizeof(request.service);
  
  tmp = htonl(request.size_contents);
  memcpy(buf + len, &tmp, sizeof(int32_t));
  len += sizeof(int32_t);

  strcpy(buf + len, request.sz_contents);
  len += request.size_contents;
  
  n = write(sockfd, buf, len);

#if 0
  fptest = fopen("_send.txt", "ab+");                                                                                          
  fwrite(buf, 1, len, fptest);
  fclose(fptest);
#endif

  if (n < 0) {
	fprintf(stderr, "read error!\n");
	return SOCK_ERR;
  }
  return 0;
}



int tcp_socket_recv(int sockfd)
{
  /*  FILE *fprecv; */
  char recvline[MAXLINE];
  char buff[MAXLINE];
  char service[130],service_cost[24]; 
  char *ptmp;
  int n, ntmp;
  int len, olen, flg;
#if 0
  fprecv = fopen("_recv.txt", "ab+");                                                                                          
#endif


  printf("\nreceive...\n");
  if ((n = readn(sockfd, recvline, L_HEAD)) < L_HEAD) {
	return -1;
  } else {

	flg  = *(int32_t *)recvline;
	flg = ntohl(flg);
	len  = *(int32_t *)(recvline + 4); /* real */
	len = ntohl(len);
	olen = *(int32_t *)(recvline + 8); /* original */
	olen = ntohl(olen);
  }

#if 0  
  printf("flg  = %d\n", flg);
  printf("len  = %d\n", len);
  printf("olen = %d\n", olen);
#endif

  printf("flg  \t\t= |%x|\n", flg);


  if (len <= 0 || olen <= 0) {
	return 0;
  }

  if (len < olen) {
	return -1; /* compress */
  }
  if (0 != (bPACK_FAILED & flg) || 0 != (bPACK_CATCH_ERR & flg)) {
	  if ((n = readn(sockfd, recvline, len)) < len) {
		return -1;
	  } else {
		recvline[len] = '\0';
		printf("react_errinfo \t= |%s|\n", recvline);
		return 0;
	  }
  }


  if ((n = readn(sockfd, recvline, len)) < len) {
	return -1;
  } else {
	ptmp = recvline;
	ptmp += 0;
	memcpy(service, ptmp, 128);
	service[128] = '\0';
	trim(service);
	printf("service \t= |%s|\n", service);

	ptmp += 128;
	memcpy(service_cost, ptmp , 20);
	service_cost[20] = '\0';
	trim(service_cost);
	printf("service_cost \t= |%s|\n", service_cost);

	ptmp += 20;
	ntmp  = *(int32_t *)(ptmp);
	ntmp = ntohl(ntmp);
	if (ntmp <= 0) {
	  return -1;
	}
	printf("react_size \t= |%d|\n", ntmp);

	ptmp += 4;
	memcpy(buff, ptmp , ntmp);
	buff[ntmp] = '\0';
	printf("react_contents \t= |%s|\n", buff);


  }

#if 0
  fclose(fprecv);
#endif
  if (n < 0) {
	fprintf(stderr, "read error!\n");
	return SOCK_ERR;
  }
  return 0;

}






static int run(int argc, char *argv[]) 
{
  int ret = 0;
  int sockfd;

  char *confname = "isoaclient.conf";
  char *sz_pfilename;
  FILE *f;

  struct own_entr_content content;


  //  printf("argc = %d\n", argc);
  if (argc >= 2) {
	sz_pfilename = argv[1];
  } else {
	sz_pfilename = confname;
  }



  if (NULL == sz_pfilename || strlen(sz_pfilename) <= 0) {
	printf("input para error!");
	return -1;
  }

  if (NULL == (f = fopen(sz_pfilename, "r+"))) {
	printf("can't find conf file\n");
	return -1;
  }


  memset(&content, 0x00, sizeof(content));
  ret = parse_conf(f, &content);

  fclose(f);

  if (0 != ret) {
	printf("parse conf file failed!!\n");
	return -1;
  }



  ret = tcp_socket_init(&sockfd, content.sz_ip, content.sz_port);
  if (0 != ret) {
	return ret;
  }
  ret = tcp_socket_send(sockfd, content.sz_server, content.sz_contents);
  if (0 != ret) {
	return ret;
  }

  ret = tcp_socket_recv(sockfd);
  if (0 != ret) {
	return ret;
  }

  return ret;
}

#if 1
int main(int argc, char *argv[]) 
{
  int ret;
  printf("=----------------------\n");
  ret = run(argc, argv);
  printf("=----------------------\n");
  exit(ret);
}
#endif

