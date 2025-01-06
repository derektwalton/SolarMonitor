#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>

#include "thingspeak.h"

#define PACKETSIZE 2048

static char pSendBuf[PACKETSIZE+1];
static char pRecvBuf[PACKETSIZE+1];
static char content[PACKETSIZE+1];

#define true 1
#define false 0

/*

curl -v -X POST -d "api_key=MIOWB7F35WQGMDMF&field1=50.0" http://api.thingspeak.com/update.json

> POST /update.json HTTP/1.1
> Host: api.thingspeak.com
> User-Agent: curl/7.46.0
> Accept: *//*
> Content-Length: 36
> Content-Type: application/x-www-form-urlencoded
>
* upload completely sent off: 36 out of 36 bytes
< HTTP/1.1 200 OK
< Content-Type: application/json; charset=utf-8
< Transfer-Encoding: chunked
< X-Cnection: close
< Status: 200 OK
< X-Frame-Options: ALLOWALL
< Access-Control-Allow-Origin: *
< Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS, DELETE, PATCH
< Access-Control-Allow-Headers: origin, content-type, X-Requested-With
< Access-Control-Max-Age: 1800
< ETag: "1e1a2dd12b00ff78284c28abf1d165ce"
< Cache-Control: max-age=0, private, must-revalidate
< Set-Cookie: request_method=POST; path=/
< X-Request-Id: df4b06d1-3e51-441a-b76b-8ae02b22ca7c
< X-Runtime: 0.017742
< X-Powered-By: Phusion Passenger 4.0.57
< Date: Mon, 25 Jan 2016 16:48:06 GMT
< Server: nginx/1.9.3 + Phusion Passenger 4.0.57
<
{"channel_id":80195,"field1":"50.0","field2":null,"field3":null,"field4":null,"field5":null,"field6":null,"field7":null,"field8":null,"created_at":"2016-01-25T16:48:06Z","entry_id":14,"status":null,"l
atitude":null,"longitude":null,"elevation":null}
* Connection #0 to host api.thingspeak.com left intact

*/

int sendPOSTThingspeak
( char *key,
  double *v,
  int *valuePresent,
  int n)
{
  int retval;
  struct sockaddr_in server;
  int  conn_socket;
  int i, contentLength;
  fd_set readmask;
  struct timeval t;
   
  struct hostent *hp;

  // construct content
  content[0] = 0;
  sprintf(content,"api_key=%s",key);
  for(i=0;i<n;i++) {
    if (valuePresent[i]) 
      sprintf(content+strlen(content),"&field%d=%f",i+1,v[i]);
  }
  contentLength = strlen(content);  
  if (!contentLength) {
    return true;
  }
  
  hp = gethostbyname("184.106.153.149");   
  if(!hp) {
    printf("gethostbyname() failed");
    return false;
  }
  
  // Copy the resolved information into the sockaddr_in structure
  memset(&server, 0, sizeof(server));
  memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
  server.sin_family = hp->h_addrtype;
  server.sin_port = htons(80);
  
  conn_socket = socket(AF_INET, SOCK_STREAM ,0); // Open a socket 
  if (conn_socket < 0) {
    printf("socket() failed");
    return false;
  }
  
  //printf("Client connecting to: %s.\n", hp->h_name);  
  if(-1 == connect(conn_socket, (struct sockaddr*)&server, sizeof(server))) {
    printf("connect() failed");
    close(conn_socket);
    return false;
  }
  
  // fill the packet with 'data'
  memset(pSendBuf, 0, PACKETSIZE * sizeof(char));
  sprintf(pSendBuf,"POST /update.json HTTP/1.1\nHost: api.thingspeak.com\nContent-Length: %d\nConnection: close\n\n%s\n", contentLength, content);
  printf("%s\n",pSendBuf);
  
  retval = send(conn_socket, pSendBuf, strlen(pSendBuf), 0);
  if (-1 == retval) {
    printf("send() failed");
    close(conn_socket);
    return false;
  }


  // wait for response with timeout of 20 sec
  FD_ZERO(&readmask);
  FD_SET(conn_socket, &readmask);
  t.tv_sec = 20; // 20 secs
  t.tv_usec = 0; // 0 usecs
  retval = select(conn_socket+1,(fd_set *)&readmask, (fd_set *)0, (fd_set *)0, &t);
  if (retval != 1) {
    printf("select() failed %d",retval);
    close(conn_socket);
    return false;
  }

  retval = recv(conn_socket, pRecvBuf, PACKETSIZE,0 );
  if (-1 == retval) {
    printf("recv() failed");
    close(conn_socket);
    return false;
  }
  pRecvBuf[retval]=0;
  
  close(conn_socket);

  if(strstr(pRecvBuf, "HTTP/1.1 200 OK") == NULL) {
    printf("no HTTP/1.1 200 OK");
    printf("recv() got");
    printf("%s\n",pRecvBuf);
    return false;
  }
    
  printf("Received OK\n");
  printf("  %s\n", pRecvBuf);
  return true;
}

/*

curl -v -X POST -d "api_key=FTVY72H3X5MCDUAY&status=Lakehouse reboot alert" http://api.thingspeak.com/apps/thingtweet/1/statuses/update

*   Trying 144.212.80.11...
* Connected to api.thingspeak.com (144.212.80.11) port 80 (#0)
> POST /apps/thingtweet/1/statuses/update HTTP/1.1
> Host: api.thingspeak.com
> User-Agent: curl/7.46.0
> Accept: *//*
> Content-Length: 54
> Content-Type: application/x-www-form-urlencoded
>
* upload completely sent off: 54 out of 54 bytes
< HTTP/1.1 200 OK
< Content-Type: text/html; charset=utf-8
< Content-Length: 1
< X-Cnection: close
< Status: 200 OK
< X-Frame-Options: ALLOWALL
< Access-Control-Allow-Origin: *
< Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS, DELETE, PATCH
< Access-Control-Allow-Headers: origin, content-type, X-Requested-With
< Access-Control-Max-Age: 1800
< ETag: "c4ca4238a0b923820dcc509a6f75849b"
< Cache-Control: max-age=0, private, must-revalidate
< Set-Cookie: request_method=POST; path=/
< X-Request-Id: 7468099c-9e9f-478d-b080-2a7e6a388e30
< X-Runtime: 0.343138
< X-Powered-By: Phusion Passenger 4.0.57
< Date: Mon, 25 Jan 2016 16:46:46 GMT
< Server: nginx/1.9.3 + Phusion Passenger 4.0.57
<
1* Connection #0 to host api.thingspeak.com left intact

*/

int sendTweetThingspeak
( void )
{
  int retval;
  struct sockaddr_in server;
  int  conn_socket;
  int i, contentLength;
  fd_set readmask;
  struct timeval t;
   
  struct hostent *hp;

  // construct content
  content[0] = 0;
  sprintf(content,"api_key=FTVY72H3X5MCDUAY");
  sprintf(content+strlen(content),"&status=Lakehouse power on alert");
  contentLength = strlen(content);  
  if (!contentLength) {
    return true;
  }
  
  hp = gethostbyname("184.106.153.149");   
  if(!hp) {
    printf("gethostbyname() failed");
    return false;
  }
  
  // Copy the resolved information into the sockaddr_in structure
  memset(&server, 0, sizeof(server));
  memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
  server.sin_family = hp->h_addrtype;
  server.sin_port = htons(80);
  
  conn_socket = socket(AF_INET, SOCK_STREAM ,0); // Open a socket 
  if (conn_socket < 0) {
    printf("socket() failed");
    return false;
  }
  
  //printf("Client connecting to: %s.\n", hp->h_name);  
  if(-1 == connect(conn_socket, (struct sockaddr*)&server, sizeof(server))) {
    printf("connect() failed");
    close(conn_socket);
    return false;
  }
  
  // fill the packet with 'data'
  memset(pSendBuf, 0, PACKETSIZE * sizeof(char));
  sprintf(pSendBuf,"POST /apps/thingtweet/1/statuses/update HTTP/1.1\nHost: api.thingspeak.com\nContent-Length: %d\nConnection: close\n\n%s\n", contentLength, content);
  printf("%s\n",pSendBuf);
  
  retval = send(conn_socket, pSendBuf, strlen(pSendBuf), 0);
  if (-1 == retval) {
    printf("send() failed");
    close(conn_socket);
    return false;
  }


  // wait for response with timeout of 20 sec
  FD_ZERO(&readmask);
  FD_SET(conn_socket, &readmask);
  t.tv_sec = 20; // 20 secs
  t.tv_usec = 0; // 0 usecs
  retval = select(conn_socket+1,(fd_set *)&readmask, (fd_set *)0, (fd_set *)0, &t);
  if (retval != 1) {
    printf("select() failed %d",retval);
    close(conn_socket);
    return false;
  }

  retval = recv(conn_socket, pRecvBuf, PACKETSIZE,0 );
  if (-1 == retval) {
    printf("recv() failed");
    close(conn_socket);
    return false;
  }
  pRecvBuf[retval]=0;
  
  close(conn_socket);
  
  if(strstr(pRecvBuf, "HTTP/1.1 200 OK") == NULL) {
    printf("no HTTP/1.1 200 OK\n");
    printf("recv() got\n");
    printf("%s\n",pRecvBuf);
    return false;
  }
    
  printf("Received OK\n");
  printf("  %s\n", pRecvBuf);
  return true;
}

/*
POST https://api.thingspeak.com/apps/thingtweet/1/statuses/update
    api_key=FTVY72H3X5MCDUAY
    status=I just posted this from my thing!
*/

void fetchValue(char *s, double *v, int *valuePresent);

void thingspeak(void)
{
  static int first = 1;
  int i;
  double value[16];
  int valuePresent[16];

  // LakeHouse (up to 8 channels)
  i=0;
  fetchValue("T1st",&value[i],&valuePresent[i]); i++;
  fetchValue("T2nd",&value[i],&valuePresent[i]); i++;
  fetchValue("Tbas",&value[i],&valuePresent[i]); i++;
  fetchValue("Tout",&value[i],&valuePresent[i]); i++;
  fetchValue("Tcoll",&value[i],&valuePresent[i]); i++;
  fetchValue("Tstor",&value[i],&valuePresent[i]); i++;
  fetchValue("Troot",&value[i],&valuePresent[i]); i++;
  fetchValue("PVacPower",&value[i],&valuePresent[i]); i++;
  sendPOSTThingspeak("MIOWB7F35WQGMDMF",value,valuePresent,i);

  // SeedlingRoom
  i=0;
  fetchValue("grow_ambient_T",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_ambient_humidity",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_soil_T",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_light",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_room_heat",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_soil_heat",&value[i],&valuePresent[i]); i++;
  fetchValue("grow_fan",&value[i],&valuePresent[i]); i++;
  sendPOSTThingspeak("E8WYOQIGHXEQMQCF",value,valuePresent,i);

  // Greenhouse
  i=0;
  fetchValue("greenhouse_ambient_T",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_ambient_humidity",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_inner_T",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_gnd3_T",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_pool_T",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_panel_T",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_pump",&value[i],&valuePresent[i]); i++;
  fetchValue("greenhouse_fan",&value[i],&valuePresent[i]); i++;
  sendPOSTThingspeak("YVTJHMEYDXPEMJGX",value,valuePresent,i);
  
  if (first) {
    if (sendTweetThingspeak())
      first = 0;
  }
}
