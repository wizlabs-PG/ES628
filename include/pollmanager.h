#ifndef _POLL_MANAGER_H_
#define _POLL_MANAGER_H_

#define POLL_NO_DESCRIPTOR  -2
#define POLL_BIG_ERR        -1
#define POLL_EVENTED        0
#define POLL_TIME_OUT       1
                              
#define STYP_NONE           0
#define STYP_TCPIP          1
#define STYP_UDPIP          2
#define STYP_UDS            3
#define STYP_RS232          4

//#define POLL_LOOP_TIME		2
#define POLL_LOOP_TIME		1

typedef struct net_obj_t_ net_obj_t;
struct net_obj_t_
{
  int   poll_ndx;
  int   fd;
  int   type;									    // tcp/udp/uds/serial/
  int   tag;
  void  (*on_poll_in )( net_obj_t *);
  void  (*on_poll_out)( net_obj_t *);
  void  (*on_poll_err)( net_obj_t *);
  void  (*on_poll_hup)( net_obj_t *);
};

extern void       poll_init( void);
extern int        poll_register( net_obj_t *);
extern void       poll_unregister( net_obj_t *);
extern int        poll_blank_ndx( void);
extern int        poll_loop( int);
extern int        poll_count( void);
extern net_obj_t *poll_obj( int);
extern int        poll_change_event( int, int);
extern int        poll_get_fd( int);
extern void       poll_set_fd( int, int);

#endif
