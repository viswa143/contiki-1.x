#include "smtp.h"

#include "smtp-strings.h"
#include "socket.h"

#include <string.h>

struct smtp_state {

  char connected;
  
  struct socket socket;

  char inputbuffer[4];
  
  char *to;
  char *from;
  char *subject;
  char *msg;
  u16_t msglen;  
};

static struct smtp_state s;

static char *localhostname;
static u16_t smtpserver[2];

#define ISO_nl 0x0a
#define ISO_cr 0x0d

#define ISO_period 0x2e

#define ISO_2  0x32
#define ISO_3  0x33
#define ISO_4  0x34
#define ISO_5  0x35


#define SEND_STRING(s, str) SOCKET_SEND(s, str, strlen(str))
/*---------------------------------------------------------------------------*/
static
PT_THREAD(smtp_thread(void))
{
  SOCKET_BEGIN(&s.socket);

  SOCKET_READTO(&s.socket, ISO_nl);
   
  if(strncmp(s.inputbuffer, smtp_220, 3) != 0) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }
  
  SEND_STRING(&s.socket, (char *)smtp_helo);
  SEND_STRING(&s.socket, localhostname);
  SEND_STRING(&s.socket, (char *)smtp_crnl);

  SOCKET_READTO(&s.socket, ISO_nl);
  
  if(s.inputbuffer[0] != ISO_2) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }  

  SEND_STRING(&s.socket, (char *)smtp_mail_from);
  SEND_STRING(&s.socket, s.from);
  SEND_STRING(&s.socket, (char *)smtp_crnl);

  SOCKET_READTO(&s.socket, ISO_nl);
  
  if(s.inputbuffer[0] != ISO_2) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }

  SEND_STRING(&s.socket, (char *)smtp_rcpt_to);
  SEND_STRING(&s.socket, s.from);
  SEND_STRING(&s.socket, (char *)smtp_crnl);

  SOCKET_READTO(&s.socket, ISO_nl);
  
  if(s.inputbuffer[0] != ISO_2) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }
  
  SEND_STRING(&s.socket, (char *)smtp_data);
  
  SOCKET_READTO(&s.socket, ISO_nl);
  
  if(s.inputbuffer[0] != ISO_3) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }

  SEND_STRING(&s.socket, (char *)smtp_to);
  SEND_STRING(&s.socket, s.to);
  SEND_STRING(&s.socket, (char *)smtp_crnl);
  
  SEND_STRING(&s.socket, (char *)smtp_from);
  SEND_STRING(&s.socket, s.from);
  SEND_STRING(&s.socket, (char *)smtp_crnl);
  
  SEND_STRING(&s.socket, (char *)smtp_subject);
  SEND_STRING(&s.socket, s.subject);
  SEND_STRING(&s.socket, (char *)smtp_crnl);

  SOCKET_SEND(&s.socket, s.msg, s.msglen);
  
  SEND_STRING(&s.socket, (char *)smtp_crnlperiodcrnl);

  SOCKET_READTO(&s.socket, ISO_nl);
  if(s.inputbuffer[0] != ISO_2) {
    SOCKET_CLOSE(&s.socket);
    SOCKET_EXIT(&s.socket);
  }

  SEND_STRING(&s.socket, (char *)smtp_quit);

  SOCKET_END(&s.socket);
}
/*---------------------------------------------------------------------------*/
void
smtp_appcall(void *state)
{
  if(uip_closed() || uip_aborted() || uip_timedout()) {
    s.connected = 0;
    return;
  } else {
    smtp_thread();
  }
}
/*---------------------------------------------------------------------------*/
void
smtp_configure(char *lhostname, u16_t *server)
{
  localhostname = lhostname;
  smtpserver[0] = server[0];
  smtpserver[1] = server[1];
}
/*---------------------------------------------------------------------------*/
unsigned char
smtp_send(char *to, char *from, char *subject,
	  char *msg, u16_t msglen)
{
  struct uip_conn *conn;

  conn = tcp_connect(smtpserver, HTONS(25), NULL);
  if(conn == NULL) {
    return 0;
  }
  s.connected = 1;
  s.to = to;
  s.from = from;
  s.subject = subject;
  s.msg = msg;
  s.msglen = msglen;

  SOCKET_INIT(&s.socket, s.inputbuffer, sizeof(s.inputbuffer));
  
  return 1;
}
/*---------------------------------------------------------------------------*/
void
smtp_init(void)
{
  s.connected = 0;
}
/*---------------------------------------------------------------------------*/

