/*
 * main.c
 *
 *  Created on: Sep 12, 2017
 *      Author: root
 */

#include <global.h>
#include <string.h>
#include <msg_comu.h>
#include <rcb.h>
#include <pwr_control.h>
#include <group_data.h>
#include <netinc.h>
#include <pollmanager.h>
#include <gpio.h>
#include <i2c_gpio.h>
#include <i2c_lcd.h>
#include <nvgstplayer.h>
#include <usb_storage.h>
#include <pattern_control.h>
#include <i2c_sensing.h>
#include <file_manager.h>
#include <rcb_485.h>
#include <i2c.h>
#include <gxttf.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <model_data.h>
#include <group_data.h>
#include <pwr_tcpclient.h>
#include <time.h>

#ifdef USE_SHARED_MEM
shared_status_t *status_shm;
buffer_status_t status_buf = {0,};
int status_shm_pid;
#endif





static void print_title(void)
{
	time_t seconds = time(NULL);
	printf("\n");
	printf("======================================================\n");
	printf("Name		: %s\n", PG_NAME);
	printf("Version 	: R%d.%d %s\n", FW_VERSION/10, FW_VERSION%10, __DATE__);
	printf("Current Time:  %s\n",__TIME__);
	printf("Author		: seo chi hong\n");
	printf("Copyright 	: copyright (c) 2019 ensis.co.,Ltd.\n");
	printf("======================================================\n");
	printf("\n");
}

static int ip_check(void)
{
	net_config_t 	net_info = {0,};
	char 			data[MAX_PATH], eth_name[MAX_PATH];
	uint8_t			dipsw_ip, last_ip, group_id;
	int 			retval=0;

	gp.network_connect=0;

	ERRCHECK(system("killall NetworkManager"));
	ERRCHECK(system("killall dhclient"));
	ERRCHECK(system("killall wpa_supplicant"));

	memset(eth_name, 0, MAX_PATH);
	get_eth_name(eth_name);

	get_ip_set(&net_info);

	printf("ETHERNET : %s\n", eth_name);
	printf("IP : %d.%d.%d.%d\n", net_info.myip[0], net_info.myip[1], net_info.myip[2], net_info.myip[3]);
	printf("SUBNET : %d.%d.%d.%d\n", net_info.netmask[0], net_info.netmask[1], net_info.netmask[2], net_info.netmask[3]);

	dipsw_ip 	= i2c_gpio_get_ipsw();
	last_ip		= net_info.myip[3];
	printf("DIP S/W	: %d, last_ip : %d\n", dipsw_ip, last_ip);

	group_id = get_group_id();

//ERRCHECK(system("iw reg set VN"));//SDV:VN SDC:00 SSM:CN
#if 1
	if(dipsw_ip != last_ip)
	{
		FILE *fp;
		fp = fopen(NET_INTERFACE_FILE, "wb");
		if(fp!=NULL)
		{
			memset(data, 0, sizeof(data));
			sprintf(data, "# interfaces(5) file used by ifup(8) and ifdown(8)\n");
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data, "source-directory /etc/network/interfaces.d\n");
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data, "auto %s\n", eth_name);
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data,"iface %s inet static\n", eth_name);
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data,"address %s%d.%d\n", DEF_PREFIX_IP, group_id, dipsw_ip);
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data,"netmask %s\n", DEF_SUBNET_MASK);
			fwrite(data, strlen(data), 1, fp);

			memset(data, 0, sizeof(data));
			sprintf(data,"gateway %s%d.1\n", DEF_PREFIX_IP, group_id);
			fwrite(data, strlen(data), 1, fp);

			fclose(fp);

			memset(data, 0, sizeof(data));
			sprintf(data,"%s%d.%d", DEF_PREFIX_IP, group_id, dipsw_ip);
			rcb_write(rcb_fd, RCB_LINE1, "IP CHANGED!");
			rcb_write(rcb_fd, RCB_LINE2, data);
			sleep(5);
			retval=1;
			//return 1;
		}
	}
	//wifi
	char line[1024];
	char ssid[MAX_WIFI_SSID];
	char identity[MAX_WIFI_IDENTITY];
	char password[MAX_WIFI_PASSWORD];
	char ip[MAX_IP_ADDR];
	char netmask[MAX_IP_ADDR];
	char gateway[MAX_IP_ADDR];
	unsigned char wifi_select=0;

	ERRCHECK(system("ifconfig wlan0 up"));
	get_ip_wlan0(&net_info);
	printf("WIFI MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);

	memcpy(gp.wifi_mac,net_info.mac,sizeof(net_info.mac));
	memset(ssid,0,sizeof(ssid));
	memset(identity,0,sizeof(identity));
	memset(password,0,sizeof(password));
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));

	wifi_select=get_wifi_info(ssid,ip,netmask,gateway,identity,password);
	if(wifi_select>0)
	{
		if(wifi_select<=2)
		{
			memset(line, 0, sizeof(line));
			sprintf(line, "ifconfig %s down",eth_name);
			ERRCHECK(system(line));
		}
		printf("WIFI SSID : %s\n", ssid);
		printf("WIFI IP : %s\n", ip);

		memset(line, 0, sizeof(line));
		sprintf(line, "wpa_passphrase ""%s"" ""%s"" > %s",ssid,password,WPA_CONF_TEMP_FILE);

		if(system(line)>=0)
		{
			FILE *fp;
			FILE *fp1;

			fp = fopen(WPA_CONF_TEMP_FILE, "r");
			fp1 = fopen(WPA_CONF_FILE, "wb");
			if((fp!=NULL)&&(fp1!=NULL))
			{
#if 0
				memset(line, 0, sizeof(line));
				sprintf(line, "ctrl_interface=/var/run/wpa_supplicant\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "eapol_version=1\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "ap_scan=1\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "fast_reauth=1\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fgets(line,1024,fp);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fgets(line,1024,fp);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fgets(line,1024,fp);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fgets(line,1024,fp);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fclose(fp);

				memset(line, 0, sizeof(line));
				sprintf(line, "\tproto=RSN\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tkey_mgmt=WPA-PSK\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tpairwise=CCMP TKIP\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tgroup=CCMP\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "blob-base64-exampleblob={\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "SGVsbG8gV29ybGQhCg==\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "network={\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "key_mgmt=NONE\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fclose(fp1);
				sync();
#else
				//PEAP
				memset(line, 0, sizeof(line));
				sprintf(line, "ctrl_interface=/var/run/wpa_supplicant\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "eapol_version=1\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "ap_scan=1\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "fast_reauth=0\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				{
					memset(line, 0, sizeof(line));
					sprintf(line, "network={\n");
					ERRCHECK(fwrite(line, strlen(line), 1, fp1));

					memset(line, 0, sizeof(line));
					sprintf(line, "\tssid=\"%s\"\n",ssid);
					ERRCHECK(fwrite(line, strlen(line), 1, fp1));
				}
				fclose(fp);

				memset(line, 0, sizeof(line));
				//sprintf(line, "\tpriority=15\n");
				sprintf(line, "\tpriority=10\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tkey_mgmt=WPA-EAP\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				//sprintf(line, "\teap=PEAP MSCHAPV2\n");
				sprintf(line, "\teap=PEAP\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tidentity=\"%s\"\n",identity);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tpassword=\"%s\"\n",password);
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "\tphase2=\"auth=MSCHAPV2\"\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "blob-base64-exampleblob={\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "SGVsbG8gV29ybGQhCg==\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "network={\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "key_mgmt=NONE\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				memset(line, 0, sizeof(line));
				sprintf(line, "}\n");
				ERRCHECK(fwrite(line, strlen(line), 1, fp1));

				fclose(fp1);
				sync();
#endif

				memset(line, 0, sizeof(line));
				sprintf(line, "wpa_supplicant -i wlan0 -c %s&",WPA_CONF_FILE);
				ERRCHECK(system(line));

				sprintf(data,"SSID=%s",ssid);
				rcb_write(rcb_fd, RCB_LINE1, "WIFI CHANGED!");
				rcb_write(rcb_fd, RCB_LINE2, data);
				sleep(10);

				if(wifi_select==2)
				{
					ERRCHECK(system("dhclient -v wlan0 &")); //dhcp
				}
				else
				{
					memset(line, 0, sizeof(line));
					sprintf(line, "ifconfig wlan0 %s netmask %s up",ip,netmask);
					ERRCHECK(system(line));

					//geteway
					memset(line, 0, sizeof(line));
					sprintf(line, "route add default gw %s wlan0",gateway);
					ERRCHECK(system(line));
				}
				remove(WPA_CONF_TEMP_FILE);
			}
		}
	}
	else
	{
		ERRCHECK(system("ifconfig wlan0 down"));
	}

//	ERRCHECK(system("sudo pm-powersave false"));//wifi power save off
#endif
	return retval;
}

static void pwr_sensing(void)
{
	if( get_onoff_flag()==ENUM_OFF ) return;
	if( (get_opmode()!=AUTO_RUN) && (get_opmode()!=MANU_RUN) ) return;

	uint64_t 	elapse_time;
	elapse_time = get_detect_elapse_time();
	if(elapse_time >= PWR_DETECT_TIME)
	{
		if(bd_idx<1 || bd_idx>ENSIS_PWR_CH) bd_idx = 1;

		pwr_detect_get(bd_idx++);
		set_detect_start_time();
	}
}

static void exit_handler(int sig_no)
{
	if(sig_no == SIGINT)	// Ctrl+C
	{
		_intr_handler(sig_no);	// destroy (nvgstplayer.c)
		exit_flag = 1;
		exit_load = 1;
	}
}

static void sig_setup(void)
{
	struct sigaction action;

	memset (&action, 0, sizeof (action));
	action.sa_handler = exit_handler;

	sigaction (SIGINT, &action, NULL);
}

static void variables_init(void)
{
	set_reboot(0);
	set_opmode(INIT);
	set_pattern_index(0);
	set_schedule_index(0);

	model_var_init();
}
// for removing HDMI noise
void *load_cpu(void *arg)
{
	while(!exit_load){}
	return NULL;
}

#define PACKETSIZE	64
struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};
struct protoent *proto=NULL;

unsigned short checksum(void *b, int len)
{	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

// Define the Packet Constants
// ping packet size
#define PING_PKT_S 64

// Automatic port number
#define PORT_NO 0

// Automatic port number
#define PING_SLEEP_RATE 1000000

// Gives the timeout delay for receiving packets
// in seconds
#define RECV_TIMEOUT 2

// ping packet structure
struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

// add 19-07-18
int send_ping(char *gateway)
{
	int ret=0;
    int i, addr_len;
    struct ping_pkt pckt;
    struct sockaddr_in r_addr;
    int ping_sockfd;
	struct sockaddr_in ping_addr;
	int ttl_val=64;
    struct timespec tfs;

    //socket()
    ping_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(ping_sockfd<0)
    {
        printf("\nSocket file descriptor not received!!\n");
        return 0;
    }


	memset(&ping_addr,0, sizeof(ping_addr));
	ping_addr.sin_family 		= PF_INET;
	ping_addr.sin_port = htons (PORT_NO);
	ping_addr.sin_addr.s_addr = inet_addr(gateway);


    struct timeval tv_out;
    tv_out.tv_sec = RECV_TIMEOUT;
    tv_out.tv_usec = 0;

    clock_gettime(CLOCK_MONOTONIC, &tfs);

	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
	{
		printf("\nSetting socket options to TTL failed!\n");
	}
	// setting timeout of recv setting
	setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

	// flag is whether packet was sent or not

	//filling packet
	bzero(&pckt, sizeof(pckt));

	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = getpid();

	for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
		pckt.msg[i] = i+'0';

	pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = 0;
	pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));


	//send packet
	if ( sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*) &ping_addr, sizeof(ping_addr)) <= 0)
	{
		printf("\nPacket Sending Failed!\n");
	}

	//receive packet
	addr_len=sizeof(r_addr);

	if ( recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, (socklen_t *)&addr_len) <= 0)
	{
		ret=0;
	}
	else
	{
		if(!(pckt.hdr.type ==69 && pckt.hdr.code==0))
		{
			printf("Error..Packet received with ICMP type %d code %d\n", pckt.hdr.type, pckt.hdr.code);
			ret=0;
		}
		else ret=1;
	}

	close(ping_sockfd);
    return ret;
}

int ping(char *ip)
{
	const int val=255;
	int i, sd, cnt=1;
	int ret=0;
	struct packet pckt;
	struct sockaddr_in r_addr;
	struct sockaddr_in s_addr;

	proto = getprotobyname("ICMP");

	memset(&s_addr,0, sizeof(s_addr));
	s_addr.sin_family 		= PF_INET;
	s_addr.sin_port = 0;
	s_addr.sin_addr.s_addr = inet_addr(ip);

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 )
	{
		printf("PING:socket\n");
		return 0;
	}

	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
	{
		printf("PING:Set TTL option\n");
		close(sd);
		return 0;
	}

	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
	{
		printf("PING:Request nonblocking I/O\n");
		close(sd);
		return 0;
	}

	{

		memset(&pckt,0, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = 1;
		for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
			pckt.msg[i] = i+'0';
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&s_addr, sizeof(s_addr)) <= 0 )
		{
			printf("PING:sendto\n");
			close(sd);
			return 0;
		}

		sleep(1);

		int len=sizeof(r_addr);

		if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr,(socklen_t *) &len) > 0 ) ret=1;

	}
	close(sd);

	return ret;
}

//seochihong 20190218
int get_wifi_ping()
{
	int sockfd;
	struct iw_statistics stats;
	struct iwreq req;

	memset(&stats, 0, sizeof(stats));
	memset(&req, 0, sizeof(req));
	sprintf(req.ifr_name, "wlan0");
	req.u.data.pointer = &stats;
	req.u.data.length = sizeof(stats);


	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Could not create simple datagram socket");
		return 0;
	}

	/* Perform the ioctl */
	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1) {
		close(sockfd);
		return 0;
	}

	close(sockfd);
	return 1;
}

// add 19-07-18
void *link_check_thread(void *arg)
{
	int ping_check=0;
    int reconnect=1;
	char line[1024];
	char ssid[MAX_WIFI_SSID];
	char identity[MAX_WIFI_IDENTITY];
	char password[MAX_WIFI_PASSWORD];
	char serverip[MAX_IP_ADDR];
	char ip[MAX_IP_ADDR];
	char netmask[MAX_IP_ADDR];
	char gateway[MAX_IP_ADDR];
	unsigned char wifi_select=0;

	memset(ssid,0,sizeof(ssid));
	memset(identity,0,sizeof(identity));
	memset(password,0,sizeof(password));
	memset(serverip,0,sizeof(serverip));
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));

	get_server_ip(serverip);
	wifi_select=get_wifi_info(ssid,ip,netmask,gateway,identity,password);


	while(1)
	{
		if(wifi_select>0)
		{
			/*
			if(send_ping(gateway)<=0)
			{
				ping_check++;
			}*/
			ping_check=send_ping(gateway);
			if(ping_check<=0)
			{
				sleep(1);
				ping_check=send_ping(gateway);
				if(ping_check<=0)
				{
					sleep(1);
					ping_check=send_ping(gateway);
				}
			}


			if(ping_check<=0)
			{
				ping_check=0;
				gp.network_connect=1; //wifi reconnting
				ERRCHECK(system("killall NetworkManager"));
				ERRCHECK(system("killall wpa_supplicant"));

				ERRCHECK(system("ifconfig wlan0 up"));
				printf("--WIFI RECONNECT--\n");
				printf("WIFI SSID : %s\n", ssid);
				printf("WIFI IP : %s\n", ip);

				memset(line, 0, sizeof(line));
				sprintf(line, "wpa_supplicant -i wlan0 -c %s&",WPA_CONF_FILE);
				ERRCHECK(system(line));

				sleep(15);

				if((wifi_select==1)||(wifi_select==3))
				{
					memset(line, 0, sizeof(line));
					sprintf(line, "ifconfig wlan0 %s netmask %s up",ip,netmask);
					ERRCHECK(system(line));

					//geteway
					memset(line, 0, sizeof(line));
					sprintf(line, "route add default gw %s wlan0",gateway);
					ERRCHECK(system(line));
				}
				else if(wifi_select==2)
				{
					ERRCHECK(system("killall dhclient"));
					sleep(1);
					ERRCHECK(system("dhclient -v wlan0 &")); //dhcp
				}
				sleep(20);
				printf("--WIFI RECONNECT END--\n");
//				ERRCHECK(system("sudo pm-powersave false"));//wifi power save off
				/*
				if(ping(serverip)<=0)
				{
					gp.network_connect=1;//reconnect
					printf("--WIFI RECONNECT FALSE--\n");
				}
				else
				{
					gp.network_connect=2;//connect
					printf("--WIFI RECONNECT COMPLETE--\n");
				}*/
				reconnect=1;
			}
			else gp.network_connect=2;//connect
		}
		else gp.network_connect=0; //wifi disconnect

//		printf("gp.network_connect=%d wifi_select=%d\n",gp.network_connect,wifi_select);

		//ERRCHECK(system("sudo pm-powersave false"));//wifi power save off
		sleep(5);
		if(reconnect>0)
		{
//			ERRCHECK(system("sudo pm-powersave false"));//wifi power save off
			reconnect++;
			if(reconnect>5) reconnect=0;
		}
	}
	return NULL;
}

static void video_player_check()
{
	char cmd[128];
	char buffer[128];
	int	ret=0;
	FILE *fp;

	if( get_onoff_flag()==ENUM_OFF ) return;
	if( (get_opmode()!=AUTO_RUN) && (get_opmode()!=MANU_RUN) ) return;
	if (video_start==0)	return;
	if ( (fpga_time.h_active>=3840) && (fpga_time.v_active>=2160) ) return;


	memset(cmd, 0, sizeof(cmd));
	memset(buffer, 0, sizeof(buffer));

	sprintf(cmd, "pgrep %s", DEF_VIDEO_PLAYER);
	fp = popen(cmd, "r");
	ret = fread(buffer, sizeof(char), sizeof(buffer), fp);
	pclose(fp);

	if(ret<0) printf("Failed checking fhd video player process\n");

	if( (buffer[0]==0) && (buffer[1]==0) && (buffer[2]==0) )
	{
//		printf("gst-launch is not running\n");
		ERRCHECK(system(under_video_path));
		usleep(600000);
	}
	else
	{
//		printf("checking fhd video player process : %s\t%d %d %d\n", buffer,buffer[0],buffer[1],buffer[2]);
	}
}

#ifdef USE_SHARED_MEM
void *status_check_thread(void *arg)
{
	uint32_t	i;
	int			ret;

	memset(status_shm, 0, sizeof(shared_status_t));
	memset(&status_buf, 0, sizeof(buffer_status_t));

	printf("\nshm size = %ld, buf size = %ld\n", sizeof(shared_status_t), sizeof(status_buf));

	while(1)
	{

		sprintf((char*)status_buf.fw_version, "%d", FW_VERSION);

		if(get_opmode()==AUTO_RUN)
			status_buf.state = 2;
		else if( ((get_opmode()>=MANU_RUN) && (get_opmode()<=PWM_CHANGE)) || (get_opmode()==CURSOR_COLOR) )
			status_buf.state = 3;
		else
			status_buf.state = 1;

//		if(get_model_init_status()==NACK)

		if(model_selection_end==ACK)
			sprintf((char*)status_buf.cur_model_name, "%s", model_name);
		else
			sprintf((char*)status_buf.cur_model_name, "%s", ERR_STRING);

		if(group_selection_end==ACK)
		{
			sprintf((char*)status_buf.cur_group_name, "%s", group_name);
			sprintf((char*)status_buf.cur_pat_name, "%s", get_pattern_name());
		}
		else
		{
			sprintf((char*)status_buf.cur_group_name, "%s", ERR_STRING);
			sprintf((char*)status_buf.cur_pat_name, "%s", ERR_STRING);
		}

		if(get_pwr_vendor()==0)		//EU1150
		{
			status_buf.vdd = htons(rsp_detect_data[0].det_val.vdd);
			status_buf.vbl = htons(rsp_detect_data[0].det_val.vbl);

			for(i=0; i<ENSIS_PWR_CH; i++)
			{
				status_buf.idd += htons(rsp_detect_data[i].det_val.idd);
				status_buf.ibl += htons(rsp_detect_data[i].det_val.ibl);
			}
//			printf("vdd:%d, vbl:%d, idd:%d, ibl:%d\n", status_shm.vdd, status_shm.vbl, status_shm.idd, status_shm.ibl);

			for(i=0; i<ENSIS_PWR_CH; i++)
			{
				if(rsp_detect_data[i].error!=0)
				{
					// 1st check(power)
					status_buf.limit 	= rsp_detect_data[i].error;
					status_buf.vdd	= htons(rsp_detect_data[i].det_val.vdd);
					status_buf.vbl	= htons(rsp_detect_data[i].det_val.vbl);
					status_buf.ocp_pid	= (uint8_t)(i+1);	// power board id(1~4)
//					printf("limit(power_ch%d) : 0x%02X \n", i, status_shm.limit);
					break;
				}
			}
		}
		else	//ES620P, OSUNG
		{
			status_buf.vdd = (rsp_detect_osung_data.det_val[0].vdd);
			status_buf.vbl = (rsp_detect_osung_data.det_val[0].vbl);
			status_buf.idd = (rsp_detect_osung_data.det_val[0].idd);
			status_buf.ibl = (rsp_detect_osung_data.det_val[0].ibl);

//			printf("vdd:%d, vbl:%d, idd:%d, ibl:%d\n", status_shm.vdd, status_shm.vbl, status_shm.idd, status_shm.ibl);

			if(rsp_detect_osung_data.error[0]!=0)
			{
				status_buf.limit		= (rsp_detect_osung_data.error[0]);
				status_buf.vdd			= (rsp_detect_osung_data.det_val[0].vdd);
				status_buf.vbl			= (rsp_detect_osung_data.det_val[0].vbl);
				status_buf.ocp_pid 	= 1;
//				printf("limit(power) : 0x%02X \n", status_shm.limit);
			}
		}


		ret = memcmp(&status_buf.state, &status_shm->state, sizeof(buffer_status_t)-4);
		if(ret==0){
//			printf("Status is not changed\n");
		}
		else {
//			printf("Status is changed\n");
			memcpy(status_shm, &status_buf, sizeof(buffer_status_t));
			status_shm->rw = 1;
		}
		status_shm->res = 1;

		usleep(500000);
	}

	return NULL;
}

void read_shmem(void)
{
	printf("v=%s st=%d m=%s g=%s p=%s vdd=%d vbl=%d idd=%d ibl=%d limit=0x%x ocp=%d progress=%d\n",		\
			status_shm->fw_version, status_shm->state,													\
			status_shm->cur_model_name, status_shm->cur_group_name, status_shm->cur_pat_name,			\
			status_shm->vdd, status_shm->vbl, status_shm->idd, status_shm->ibl,							\
			status_shm->limit, status_shm->ocp_pid, status_shm->progress);
}

void set_progress_count(int count)
{
	status_buf.progress = (unsigned short)count;
}

#endif


uint16_t	pseq_sig_on, pseq_sig_tgl;
uint16_t	pseq_sfirst, pseq_pwr1_on, pseq_pwr2_on;
uint32_t	pseq_scnt;
uint16_t	pseq_acdet_on;
uint16_t	pseq_i2c_1_on;
uint16_t	pseq_ocmd_1_on;
uint16_t	pseq_i2c_2_on;
uint16_t	pseq_ocmd_2_on;
uint16_t	pseq_i2c_on;
uint16_t	pseq_ocmd_on;
uint8_t		shutdown_p, shutdown_ac;
uint16_t	cycle_end;

void std_pseq_forced_shut_down(void)
{
	std_power_seq_run=2;

//	shutdown_p=1;
	printf("pseq forced stop\n");
	set_std_pseq_power_start_time(); //2023.10.20 power sequence first cycle issue
	cycle_end=0; //2023.10.20 power sequence first cycle issue


	FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);

#ifdef EXTERNAL_POWER_CTRL
	pwr_ctrl_onoff(ID_PWR2, 4, 0);
	pwr_ctrl_onoff(ID_PWR1, 4, 0);
	printf("Force_shutdown: PWR1 OFF\n");
	printf("Force_shutdown: PWR2 OFF\n");
#else
	std_power_onoff_set(1, 4, 0);	//power2, all channel, off
	std_power_onoff_set(0, 4, 0);	//power1, all channel, off
#endif


	pseq_sig_on=0;			//sig on init
	pseq_sig_tgl=1;			//sig toggle init
	pseq_pwr2_on=0;			//pwr2 on init
	pseq_pwr1_on=0;			//pwr1 on init
	pseq_scnt=0;			//seq cnt init
	pseq_rpcnt=0;			//rpt cnt init 2023.11.01 ksk don't reset scnt, rpcnt for saving status of pseq
	pseq_sfirst=1;			//first seq flag init

	pause_time=0;
	old_pause_time=0;
	sig_pause_time=0;
	old_sig_pause_time=0;

//	shutdown_ac=1;
//	printf("pseq acdet i2c forced off\n");
	pseq_acdet_on=0;
	pseq_i2c_1_on=0;
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0000);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcd,0x0000);
	ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V

	std_power_seq_run=0;	//pseq run flag off

	set_onoff_flag(ENUM_OFF);
	rcb_ready_screen(ACK);
	set_opmode(READY);
}

#ifdef EXTERNAL_POWER_CTRL

int				link_terminated_pwr1=0, link_terminated_pwr2=0;
struct timeval 	connect_start_time;
struct timeval 	connect_start_time_pwr1;
struct timeval 	connect_start_time_pwr2;
struct timeval 	non_res_start_time;

static void set_connect_start_time(uint8_t id)
{
	switch(id)
	{
		case ID_PWR1 :	gettimeofday(&connect_start_time_pwr1, NULL); break;
		case ID_PWR2 :	gettimeofday(&connect_start_time_pwr2, NULL); break;
		default :		gettimeofday(&connect_start_time, NULL); break;
	}
}

uint64_t get_connect_elapse_time(uint8_t id)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);

	switch(id)
	{
		case ID_PWR1 :	timersub(&end_time, &connect_start_time_pwr1, &result_time); break;
		case ID_PWR2 :	timersub(&end_time, &connect_start_time_pwr2, &result_time); break;
		default :		timersub(&end_time, &connect_start_time, &result_time); break;
	}

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}


//static void get_power_ip(char *ip1, char *ip2)
//{
//	FILE	*fp;
//	char 	path[MAX_PATH], str[MAX_PATH];
//
//	memset(path, 0, MAX_PATH);
//	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, POWER_IP_FILE);
//	if(0==access(path, F_OK))
//	{
//		fp = fopen(path, "r");
//		memset(str, 0, MAX_PATH);
//		if(0==fgets(str, MAX_IP_ADDR, fp)) {}
//		strncpy(ip1, str, strlen(str)-2);
//
//		memset(str, 0, MAX_PATH);
//		if(0==fgets(str, MAX_IP_ADDR, fp)) {}
//		strncpy(ip2, str, strlen(str)-2);
//		fclose(fp);
//	}
//	else
//	{
//		fprintf(stderr, "%s file not exist. creating file...\n", POWER_IP_FILE);
//		sprintf(ip1, "%s\r\n", DEF_POWER1_IP);
//		sprintf(ip2, "%s\r\n", DEF_POWER2_IP);
//		fp = fopen(path, "w");
//		fputs(ip1, fp);
//		fputs(ip2, fp);
//		fclose(fp);
//		sync();
//	}
//}
//
//void set_power_ip(char *ip1, char *ip2)
//{
//	FILE	*fp;
//	char 	path[MAX_PATH], str[MAX_PATH];
//
//	memset(path, 0, MAX_PATH);
//	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, POWER_IP_FILE);
//	fp = fopen(path, "w");
//	if(fp)
//	{
//		memset(str, 0, MAX_PATH);
//		sprintf(str, "%s\r\n", ip1);
//		fputs(str, fp);
//
//		memset(str, 0, MAX_PATH);
//		sprintf(str, "%s\r\n", ip2);
//		fputs(str, fp);
//
//		fclose(fp);
//		sync();
//	}
//	else
//	{
//		printf("[%s] %s failed!\n", path, POWER_IP_FILE);
//	}
//}


void *External_power_link_check_thread(void *arg)
{
	while(1)
	{
		if(pwr_ip_changed){
			pwr_ip_changed=0;
			if(NULL != tcp_pwr1_fd) {
				tcpclient_close(tcp_pwr1_fd);
				tcp_pwr1_fd = NULL;
			}
			if(NULL != tcp_pwr2_fd) {
				tcpclient_close(tcp_pwr2_fd);
				tcp_pwr2_fd = NULL;
			}
			pwr1_ch_chk_done=0;
			pwr2_ch_chk_done=0;
			memset(power1_ip_info, 0, MAX_IP_ADDR);
			memset(power2_ip_info, 0, MAX_IP_ADDR);
			get_power_ip(power1_ip_info, power2_ip_info);
		}


//		elapse_time_pwr1 = get_connect_elapse_time(ID_PWR1);
//		if(elapse_time_pwr1 >= 1000)
		{

			if( NULL == tcp_pwr1_fd )
			{
//				printf("trying to connect pwr1 %s ...\n", power1_ip_info);
				tcp_pwr1_fd = tcpclient_open(power1_ip_info, DEF_TCP_PORT, 1);	// Try to connect every 2 second
				if( NULL == tcp_pwr1_fd )
				{
//					fprintf(stderr, "tcpclient_open() pwr1 failed!(%d)\n", neterr_no);
					pwr1_ch_chk_done=0;
				}
				else
				{
					printf("tcp pwr1 %s connected!\n", power1_ip_info);
					tcp_pwr1_fd->on_read 	= on_tcp_recv_pwr1;
					tcp_pwr1_fd->on_error 	= on_tcp_error_pwr1;

					msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.
					check_output_channel_count(ID_PWR1);
				}
			}
			else
			{
				if(link_terminated_pwr1==1)
				{
					printf("pwr1 disconnected!~~~~~~~~~~~~~\n");
					tcpclient_close(tcp_pwr1_fd);
					tcp_pwr1_fd = NULL;

//					pthread_mutex_lock(&mutex_lock);
					link_terminated_pwr1 = 0;
//					pthread_mutex_unlock(&mutex_lock);
				}
			}
//			set_connect_start_time(ID_PWR1);
		}
		// power1 section ---------------------------------------------------

		// power2 section ---------------------------------------------------
//		elapse_time_pwr2 = get_connect_elapse_time(ID_PWR2);
//		if(elapse_time_pwr2 >= 1000)
		{

			if( NULL == tcp_pwr2_fd )
			{
//				printf("trying to connect pwr2 %s ...\n", power2_ip_info);
				tcp_pwr2_fd = tcpclient_open(power2_ip_info, DEF_TCP_PORT, 1);	// Try to connect every 2 second
				if( NULL == tcp_pwr2_fd )
				{
//					fprintf(stderr, "tcpclient_open() pwr2 failed!(%d)\n", neterr_no);
					pwr2_ch_chk_done=0;
				}
				else
				{
					printf("tcp pwr2 %s connected!\n", power2_ip_info);
					tcp_pwr2_fd->on_read 	= on_tcp_recv_pwr2;
					tcp_pwr2_fd->on_error 	= on_tcp_error_pwr2;

					msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.
					check_output_channel_count(ID_PWR2);
				}
			}
			else
			{
				if(link_terminated_pwr2==1)
				{
					printf("pwr2 disconnected!~~~~~~~~~~~~~\n");
					tcpclient_close(tcp_pwr2_fd);
					tcp_pwr2_fd = NULL;

//					pthread_mutex_lock(&mutex_lock);
					link_terminated_pwr2 = 0;
//					pthread_mutex_unlock(&mutex_lock);
				}
			}

//			set_connect_start_time(ID_PWR2);
		}

		sleep(1);
	}

	return NULL;

}
#endif

void *std_pseq_power_thread(void *arg)
{
	uint32_t	slew1=0, slew2=0;
	uint64_t 	sig_elapse_time;
//	uint64_t 	elapse_time;

//	req_std_pwr_seq_test_t i2c_data;
//	req_std_ocmd_pwr_seq_test_t ocmd_data;

	pseq_scnt=0;
	shutdown_p=0;
	pseq_sig_on=0;
	pseq_sig_tgl=1;
	pseq_sfirst=1;
	pseq_pwr1_on=0;
	pseq_pwr2_on=0;
//	old_pause_time=0;
	sig_pause_time=0;
	old_sig_pause_time=0;
//	pause_time=0;
//	if(std_power_seq_run==1) pseq_rpcnt=i2c_data.rpcnt;
//	else if(std_power_seq_run==3) pseq_rpcnt=ocmd_data.rpcnt;
	pseq_rpcnt=0;
	printf("Repeat Count = %d\n",pseq_rpcnt);
	printf("pseq power thread start\n");

	while(1)
	{
//		if( (std_power_seq_run==1)&&(shutdown_p==0)&&(shutdown_ac==0) )			//state 0: off(seq time end), 1:on, 2:forced off(user end)
		if((std_power_seq_run==1) || (std_power_seq_run==3)) 			//state 0: off(seq time end), 1:on, 2:forced off(user end), 3: on for ocmd
		{
			if(pseq_sfirst==1)
			{
				if(pwr_sequence_vx1_edp_flag == 0)
				{
					//set slew : to power thread
					slew1 = (10000*power1_voltage)/pseq_buf[pseq_scnt].t1;
					slew2 = (10000*power2_voltage)/pseq_buf[pseq_scnt].tb1;

#ifdef EXTERNAL_POWER_CTRL
					pwr_ctrl_slew(ID_PWR1, 4, slew1);
					pwr_ctrl_slew(ID_PWR2, 4, slew2);
#else
					std_power_slew_set(0, 4, slew1);			// power1, all channel, slew max
					std_power_slew_set(1, 4, slew2);			// power1, all channel, slew max
#endif


	//				printf("%d first\n", pseq_scnt);
					printf("Pseq start\n");

					pseq_sfirst=0;
					pseq_pwr1_on=1;

					//test
	//				set_w_time();
	//				time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
	//				printf("[%ld] 628 - pwr1 on 1 start\n", time_d);

					pwr_seq_aux_state = 0;

					set_std_pseq_power_start_time();


#ifdef EXTERNAL_POWER_CTRL
					pwr_ctrl_onoff(ID_PWR1, 4, 1);
					printf("Pseq_power_thread: PWR1_ON, %ld\n",elapse_time);
#else
					std_power_onoff_set(0, 4, 1);	//power1, all channel, on
#endif

					//test
	//				set_w_time();
	//				time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
	//				printf("[%ld] 628 - pwr1 on 1 end\n", time_d);

					pattern_change(0);
					set_onoff_flag(ENUM_ON);
				}
				else if(pwr_sequence_vx1_edp_flag == 1)
				{
					// reset pwr_seq_aux_addr_data.reg_data
					memset(&pwr_seq_aux_addr_data.reg_data[0], 0, sizeof(pwr_seq_aux_addr_data.reg_data));

					//set slew : to power thread
					slew1 = (10000*power1_voltage)/pseq_buf[pseq_scnt].t1;
//					slew2 = (10000*power2_voltage)/pseq_buf[pseq_scnt].tb1;

#ifdef EXTERNAL_POWER_CTRL
					pwr_ctrl_slew(ID_PWR1, 4, slew1);
//					pwr_ctrl_slew(ID_PWR2, 4, slew2);
#else
					std_power_slew_set(0, 4, slew1);			// power1, all channel, slew max
//					std_power_slew_set(1, 4, slew2);			// power1, all channel, slew max
#endif


	//				printf("%d first\n", pseq_scnt);
					printf("Pseq start\n");

					pseq_sfirst=0;
					pseq_pwr1_on=1;

					//test
	//				set_w_time();
	//				time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
	//				printf("[%ld] 628 - pwr1 on 1 start\n", time_d);

					set_std_pseq_power_start_time();


#ifdef EXTERNAL_POWER_CTRL
					pwr_ctrl_onoff(ID_PWR1, 4, 1);
					printf("Pseq_power_thread: PWR1_ON, %ld\n",elapse_time);
#else
					std_power_onoff_set(0, 4, 1);	//power1, all channel, on
#endif

					//test
	//				set_w_time();
	//				time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
	//				printf("[%ld] 628 - pwr1 on 1 end\n", time_d);

					pattern_change(0);
					set_onoff_flag(ENUM_ON);
				}
			}
			else
			{
				if(pwr_sequence_vx1_edp_flag == 0)
				{
//					elapse_time=get_std_pseq_power_elapse_time();
//					elapse_time=get_std_pseq_power_elapse_time()-elapse_pause_time;
					if(std_power_paused==1)
					{
//						set_std_pseq_power_pause_start_time(); //set standard time of pause
						pause_time=get_std_pseq_power_pause_elapse_time()+old_pause_time;
						elapse_time=get_std_pseq_power_elapse_time()-pause_time;
//						printf("## Pause time: %ld\n", pause_time);
					}
					else
					{
						elapse_time=get_std_pseq_power_elapse_time()-pause_time;
						old_pause_time=pause_time;
					}


					if(pseq_pwr1_on==1)
					{
						// power1
						if(elapse_time>=pseq_buf[pseq_scnt].pwr1_off_time)
						{
							pseq_pwr1_on=0;
//							printf("%d 2 %ld\n", pseq_scnt, elapse_time);

							// test
//							set_w_time();
//							time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//							printf("[%ld] 628 - pwr1 off 1 start\n", time_d);

#ifdef EXTERNAL_POWER_CTRL
							pwr_ctrl_onoff(ID_PWR1, 4, 0);
							printf("Pseq_power_thread: PWR1_OFF, %ld\n",elapse_time);
#else
							std_power_onoff_set(0, 4, 0);	//power1, all channel, off
#endif
//							printf("stop time = %ld\n", get_w_time());

							// test
//							set_w_time();
//							time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//							printf("[%ld] 628 - pwr1 off 1 end\n", time_d);

						}

						// signal
						if(pseq_sig_on==1)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].signal_off_time)
							{
								pseq_sig_on=0;
								pseq_sig_tgl=1;

//								printf("%d 7 %ld\n", pseq_scnt, elapse_time);

								FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
								printf("Pseq_power_thread: Signal_off TP1, %ld\n",elapse_time);
							}
							else
							{
								if(std_power_paused==1)
								{
									sig_pause_time=get_std_pseq_signal_elapse_time()+old_sig_pause_time;
									sig_elapse_time=get_std_pseq_signal_elapse_time()-sig_pause_time;
								}
								else
								{
									sig_elapse_time=get_std_pseq_signal_elapse_time()-sig_pause_time;
									old_sig_pause_time=sig_pause_time;
								}
//								sig_elapse_time=get_std_pseq_signal_elapse_time();

								if(pseq_sig_tgl==1)
								{
									if(sig_elapse_time>=pseq_buf[pseq_scnt].tlds)
									{
										pseq_sig_tgl=0;

//										printf("%d 5 %ld\n", pseq_scnt, elapse_time);

										FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
										printf("Pseq_power_thread: Signal_off TP2, %ld\n",elapse_time);
									}

//									if(pseq_buf[pseq_scnt].tlds==0)
//									{
//
//									}
								}
								else
								{
									if(sig_elapse_time>=(pseq_buf[pseq_scnt].tldo + pseq_buf[pseq_scnt].tlds))
									{
										pseq_sig_tgl=1;

//										printf("%d 6 %ld\n", pseq_scnt, elapse_time);

										FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
										printf("Pseq_power_thread: Signal_on TP1, %ld\n", elapse_time);
										set_std_pseq_signal_start_time();
									}
								}
							}

							// power2
							if(pseq_pwr2_on==1)
							{
								if(elapse_time>=pseq_buf[pseq_scnt].pwr2_off_time)
								{

//									printf("%d 9 %ld\n", pseq_scnt, elapse_time);

									pseq_pwr2_on=0;

									// test
//									set_w_time();
//									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//									printf("[%ld] 628 - pwr2 off 1 start\n", time_d);
#ifdef EXTERNAL_POWER_CTRL
									pwr_ctrl_onoff(ID_PWR2, 4, 0);
									printf("Pseq_power_thread: PWR2_OFF, %ld\n", elapse_time);
#else
									std_power_onoff_set(1, 4, 0);	//power2, all channel, off
#endif
									// test
//									set_w_time();
//									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//									printf("[%ld] 628 - pwr2 off 1 end\n", time_d);
								}
							}
							else
							{
								if( (elapse_time>=pseq_buf[pseq_scnt].pwr2_on_time) && (elapse_time<pseq_buf[pseq_scnt].pwr2_off_time))
								{

//									printf("%d 8 %ld\n", pseq_scnt, elapse_time);

									pseq_pwr2_on=1;

									// test
//									set_w_time();
//									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//									printf("[%ld] 628 - pwr2 on 1 start\n", time_d);
	#ifdef EXTERNAL_POWER_CTRL
									pwr_ctrl_onoff(ID_PWR2, 4, 1);
									printf("Pseq_power_thread: PWR2_ON %ld\n",elapse_time);
	#else
									std_power_onoff_set(1, 4, 1);	//power2, all channel, on
	#endif
									// test
//									set_w_time();
//									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//									printf("[%ld] 628 - pwr2 on 1 end\n", time_d);
								}
							}
						}
						else
						{
							if( (elapse_time>=pseq_buf[pseq_scnt].signal_on_time) && (elapse_time<pseq_buf[pseq_scnt].signal_off_time))
							{
								pseq_sig_on=1;
								pseq_sig_tgl=1;

//								printf("%d 4 %ld\n", pseq_scnt, elapse_time);

								set_std_pseq_signal_start_time();
								FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
								printf("Pseq_power_thread: Signal_on, %ld\n",elapse_time);
							}
						}
					}
					else
					{
						// 2023.11.27 unblock "else"
						if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
						{
							pseq_sig_on=0;
							pseq_sig_tgl=1;
							FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);

//							printf("%d 3 %ld\n", pseq_scnt, elapse_time);

//							pseq_pwr2_on=0;
//							std_power_onoff_set(1, 4, 0);		//power2, all channel, off

							if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
							{
								pseq_rpcnt++;
								pseq_pwr1_on=1;

//								printf("%d 1 %ld\n", pseq_scnt, elapse_time);

								// test
//								set_w_time();
//								time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//								printf("[%ld] 628 - pwr1 on 2 start\n", time_d);
								set_std_pseq_power_start_time();
								elapse_time=0;
								pause_time=0;
								old_pause_time=0;
								sig_pause_time=0;
								old_sig_pause_time=0;
								cycle_end=0;
#if (I2C_TEST == 1)
								printf("## Repeat %d, %ld\n", pseq_rpcnt, elapse_time);
								printf("# Time set by Pseq 0\n");
#endif
#ifdef EXTERNAL_POWER_CTRL
								pwr_ctrl_onoff(ID_PWR1, 4, 1);
								printf("Pseq_power_thread: PWR1_ON, %ld\n", elapse_time);
#else
								std_power_onoff_set(0, 4, 1);	//power1, all channel, on
#endif
								// test
//								set_w_time();
//								time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//								printf("[%ld] 628 - pwr1 on 2 end\n", time_d);
							}
							else
							{
								if(pseq_scnt<(pseq_set_scnt-1))
								{
									pseq_scnt++;
									scenario_cnt=pseq_scnt+1;
									printf("## Scenario %d, %ld\n", scenario_cnt, elapse_time);
								}

								else
								{
									pseq_scnt=0;
									std_power_seq_run=0;		// power_sequence end
									set_onoff_flag(ENUM_OFF);
									rcb_ready_screen(ACK);
									set_opmode(READY);
									cycle_end=0;
									printf("Pseq stop\n");
								}

								pseq_rpcnt=0;
								pseq_sfirst=1;
							}
						}	// pseq sig missing 23.11.22
					}
				}
				else if(pwr_sequence_vx1_edp_flag == 1)
				{
//					elapse_time=get_std_pseq_power_elapse_time();
//					elapse_time=get_std_pseq_power_elapse_time()-elapse_pause_time;
					if(std_power_paused==1)
					{
//						set_std_pseq_power_pause_start_time(); //set standard time of pause
						pause_time=get_std_pseq_power_pause_elapse_time()+old_pause_time;
						elapse_time=get_std_pseq_power_elapse_time()-pause_time;
//						printf("## Pause time: %ld\n", pause_time);
					}
					else
					{
						elapse_time=get_std_pseq_power_elapse_time()-pause_time;
						old_pause_time=pause_time;
					}

					if(pseq_pwr1_on==1)
					{
						// power1
						if(elapse_time>=pseq_buf[pseq_scnt].pwr1_off_time)
						{
							pseq_pwr1_on=0;
//							printf("%d 2 %ld\n", pseq_scnt, elapse_time);

							// test
//							set_w_time();
//							time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//							printf("[%ld] 628 - pwr1 off 1 start\n", time_d);

#ifdef EXTERNAL_POWER_CTRL
							pwr_ctrl_onoff(ID_PWR1, 4, 0);
							printf("Pseq_power_thread: PWR1_OFF, %ld\n",elapse_time);
#else
							std_power_onoff_set(0, 4, 0);	//power1, all channel, off
#endif
//							printf("stop time = %ld\n", get_w_time());

							// test
//							set_w_time();
//							time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//							printf("[%ld] 628 - pwr1 off 1 end\n", time_d);

						}

						// signal
						if(pseq_sig_on==1)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].signal_off_time)
							{
								pseq_sig_on=0;
//								pseq_sig_tgl=1;

//								printf("%d 7 %ld\n", pseq_scnt, elapse_time);

								FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
								printf("Pseq_power_thread: Signal_off TP1, %ld\n",elapse_time);
							}
							else
							{
								if(std_power_paused==1)
								{
									sig_pause_time=get_std_pseq_signal_elapse_time()+old_sig_pause_time;
									sig_elapse_time=get_std_pseq_signal_elapse_time()-sig_pause_time;
								}
								else
								{
									sig_elapse_time=get_std_pseq_signal_elapse_time()-sig_pause_time;
									old_sig_pause_time=sig_pause_time;
								}
//								sig_elapse_time=get_std_pseq_signal_elapse_time();

//								if(pseq_sig_tgl==1)
//								{
//									if(sig_elapse_time>=pseq_buf[pseq_scnt].tlds)
//									{
//										pseq_sig_tgl=0;
//
////										printf("%d 5 %ld\n", pseq_scnt, elapse_time);
//
//										FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
//										printf("Pseq_power_thread: Signal_off TP2, %ld\n",elapse_time);
//									}
//
////									if(pseq_buf[pseq_scnt].tlds==0)
////									{
////
////									}
//								}
//								else
//								{
//									if(sig_elapse_time>=(pseq_buf[pseq_scnt].tldo + pseq_buf[pseq_scnt].tlds))
//									{
//										pseq_sig_tgl=1;
//
////										printf("%d 6 %ld\n", pseq_scnt, elapse_time);
//
//										FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
//										printf("Pseq_power_thread: Signal_on TP1, %ld\n", elapse_time);
//										set_std_pseq_signal_start_time();
//									}
//								}
							}

//							// power2
//							if(pseq_pwr2_on==1)
//							{
//								if(elapse_time>=pseq_buf[pseq_scnt].pwr2_off_time)
//								{
//
////									printf("%d 9 %ld\n", pseq_scnt, elapse_time);
//
//									pseq_pwr2_on=0;
//
//									// test
////									set_w_time();
////									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
////									printf("[%ld] 628 - pwr2 off 1 start\n", time_d);
//#ifdef EXTERNAL_POWER_CTRL
//									pwr_ctrl_onoff(ID_PWR2, 4, 0);
//									printf("Pseq_power_thread: PWR2_OFF, %ld\n", elapse_time);
//#else
//									std_power_onoff_set(1, 4, 0);	//power2, all channel, off
//#endif
//									// test
////									set_w_time();
////									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
////									printf("[%ld] 628 - pwr2 off 1 end\n", time_d);
//								}
//							}
//							else
//							{
//								if( (elapse_time>=pseq_buf[pseq_scnt].pwr2_on_time) && (elapse_time<pseq_buf[pseq_scnt].pwr2_off_time))
//								{
//
////									printf("%d 8 %ld\n", pseq_scnt, elapse_time);
//
//									pseq_pwr2_on=1;
//
//									// test
////									set_w_time();
////									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
////									printf("[%ld] 628 - pwr2 on 1 start\n", time_d);
//	#ifdef EXTERNAL_POWER_CTRL
//									pwr_ctrl_onoff(ID_PWR2, 4, 1);
//									printf("Pseq_power_thread: PWR2_ON %ld\n",elapse_time);
//	#else
//									std_power_onoff_set(1, 4, 1);	//power2, all channel, on
//	#endif
//									// test
////									set_w_time();
////									time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
////									printf("[%ld] 628 - pwr2 on 1 end\n", time_d);
//								}
//							}
						}
						else
						{
							if( (elapse_time>=pseq_buf[pseq_scnt].signal_on_time) && (elapse_time<pseq_buf[pseq_scnt].signal_off_time))
							{
								pseq_sig_on=1;
//								pseq_sig_tgl=1;

//								printf("%d 4 %ld\n", pseq_scnt, elapse_time);

								set_std_pseq_signal_start_time();
								FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
								printf("Pseq_power_thread: Signal_on, %ld\n",elapse_time);
							}
						}

						// AUX
						if((elapse_time >= pseq_buf[pseq_scnt].t3_1) && (pwr_seq_aux_state == 0))
						{
							aux_read_function(0);
							pwr_seq_aux_state++;

							printf("pwr_seq_aux_addr_data.reg_data[0] Go, %lld\n", elapse_time);
						}
						else if((elapse_time >= pseq_buf[pseq_scnt].t3_2) && (pwr_seq_aux_state == 1))
						{
							aux_read_function(1);
							pwr_seq_aux_state++;

							printf("pwr_seq_aux_addr_data.reg_data[1] Go, %lld\n", elapse_time);
						}
						else if((elapse_time >= pseq_buf[pseq_scnt].t3_3) && (pwr_seq_aux_state == 2))
						{
							aux_read_function(2);
							pwr_seq_aux_state++;

							printf("pwr_seq_aux_addr_data.reg_data[2] Go, %lld\n", elapse_time);
						}
						else if((elapse_time >= pseq_buf[pseq_scnt].t3_4) && (pwr_seq_aux_state == 3))
						{
							aux_read_function(3);
							pwr_seq_aux_state++;

							printf("pwr_seq_aux_addr_data.reg_data[3] Go, %lld\n", elapse_time);
						}
					}
					else
					{
						// 2023.11.27 unblock "else"
						if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
						{
							pseq_sig_on=0;
//							pseq_sig_tgl=1;
							FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);

//							printf("%d 3 %ld\n", pseq_scnt, elapse_time);

//							pseq_pwr2_on=0;
//							std_power_onoff_set(1, 4, 0);		//power2, all channel, off

							if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
							{
								// reset pwr_seq_aux_addr_data.reg_data
								memset(&pwr_seq_aux_addr_data.reg_data[0], 0, sizeof(pwr_seq_aux_addr_data.reg_data));

								pseq_rpcnt++;
								pseq_pwr1_on=1;

//								printf("%d 1 %ld\n", pseq_scnt, elapse_time);

								// test
//								set_w_time();
//								time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//								printf("[%ld] 628 - pwr1 on 2 start\n", time_d);

								pwr_seq_aux_state = 0;

								set_std_pseq_power_start_time();
								elapse_time=0;
								pause_time=0;
								old_pause_time=0;
								sig_pause_time=0;
								old_sig_pause_time=0;
								cycle_end=0;
#if (I2C_TEST == 1)
								printf("## Repeat %d, %ld\n", pseq_rpcnt, elapse_time);
								printf("# Time set by Pseq 0\n");
#endif
#ifdef EXTERNAL_POWER_CTRL
								pwr_ctrl_onoff(ID_PWR1, 4, 1);
								printf("Pseq_power_thread: PWR1_ON, %ld\n", elapse_time);
#else
								std_power_onoff_set(0, 4, 1);	//power1, all channel, on
#endif
								// test
//								set_w_time();
//								time_d=(w_time.tv_sec*1000000) + w_time.tv_usec;
//								printf("[%ld] 628 - pwr1 on 2 end\n", time_d);
							}
							else
							{
								if(pseq_scnt<(pseq_set_scnt-1))
								{
									pseq_scnt++;
									scenario_cnt=pseq_scnt+1;
									printf("## Scenario %d, %ld\n", scenario_cnt, elapse_time);
								}

								else
								{
									pseq_scnt=0;
									std_power_seq_run=0;		// power_sequence end
									set_onoff_flag(ENUM_OFF);
									rcb_ready_screen(ACK);
									set_opmode(READY);
									cycle_end=0;
									printf("Pseq stop\n");
								}

								pseq_rpcnt=0;
								pseq_sfirst=1;
							}
						}	// pseq sig missing 23.11.22
					}
				}
			}
		}
		else if(std_power_seq_run==2)
		{
//			if(shutdown_p==0)
//			{
//				shutdown_p=1;
//				printf("pseq forced off\n");
////				std_power_seq_run=0;	//pseq run flag off
//
//				FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
//				std_power_onoff_set(2, 4, 0);	//power1, all channel, off
//				std_power_onoff_set(0, 4, 0);	//power1, all channel, off
//
//				pseq_sig_on=0;		//sig on init
//				pseq_sig_tgl=1;	//sig toggle init
//				pseq_pwr2_on=0;		//pwr2 on init
//				pseq_pwr1_on=0;		//pwr1 on init
//				pseq_scnt=0;			//seq cnt init
//				pseq_rpcnt=0;			//rpt cnt init
//				pseq_sfirst=1;	//first seq flag init
//			}
//			else if( (shutdown_p==1)&&(shutdown_ac==1) )
//			{
//				std_power_seq_run=0;	//pseq run flag off
//				shutdown_p=0;
//				shutdown_ac=0;
//				set_onoff_flag(ENUM_OFF);
//			}
		}
		else
		{
//			pseq_sig_on=0;		//sig on init
//			pseq_sig_tgl=1;	//sig toggle init
//			pseq_pwr2_on=0;		//pwr2 on init
//			pseq_pwr1_on=0;		//pwr1 on init
//			pseq_scnt=0;			//seq cnt init
//			pseq_rpcnt=0;			//rpt cnt init
//			pseq_sfirst=1;	//first seq flag init
		}

		usleep(500);
//		usleep(1000);
	}

	return NULL;
}
void *std_pseq_acdet_thread(req_std_pwr_seq_test_t *data)
{
//	uint64_t 	elapse_time;
	uint8_t 	acdet_on_done, acdet_off_done;


	printf("pseq acdet i2c thread start\n");

	cycle_end = 0;
	acdet_on_done=0;
	acdet_off_done=0;
//	old_pause_time=0;

	typedef enum{idle, stop, i2c_1_on, i2c_2_on, i2c_1_off, i2c_2_off, acdet_i2c_1_on, acdet_i2c_2_on, acdet_i2c_1_off, acdet_i2c_2_off} i2c_state;

	typedef enum{on1_on2_off1_off2, on1_on2_off2_off1, on1_off1_on2_off2, on1_off1_off2_on2, on1_off2_on2_off1, on1_off2_off1_on2,
				on2_on1_off1_off2, on2_on1_off2_off1, on2_off1_on1_off2, on2_off1_off2_on1, on2_off2_on1_off1, on2_off2_off1_on1,
				off1_on1_on2_off2, off1_on1_off2_on2, off1_on2_on1_off2, off1_on2_off2_on1, off1_off2_on1_on2, off1_off2_on2_on1,
				off2_on1_on2_off1, off2_on1_off1_on2, off2_on2_on1_off1, off2_on2_off1_on1, off2_off1_on1_on2, off2_off1_on2_on1} flow_chart;
	i2c_state state;
	flow_chart flow;
	state=idle;

	while(1)
	{
		if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on1_on2_off1_off2;
				else flow = on1_on2_off2_off1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on1_off1_on2_off2;
				else flow = on1_off1_off2_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_1) flow = on1_off2_on2_off1;
				else flow = on1_off2_off1_on2;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on2_on1_off1_off2;
				else flow = on2_on1_off2_off1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on2_off1_on1_off2;
				else flow = on2_off1_off2_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_1) flow = on2_off2_on1_off1;
				else flow = on2_off2_off1_on1;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_2) flow = off1_on1_on2_off2;
				else flow = off1_on1_off2_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = off1_on2_on1_off2;
				else flow = off1_on2_off2_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_on_time_2) flow = off1_off2_on1_on2;
				else flow = off1_off2_on2_on1;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_1) flow = off2_on1_on2_off1;
				else flow = off2_on1_off1_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_1) flow = off2_on2_on1_off1;
				else flow = off2_on2_off1_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_on_time_2) flow = off2_off1_on1_on2;
				else flow = off2_off1_on2_on1;
			}
		}
		if(std_power_seq_run==1)
		{
//			elapse_time = get_std_pseq_power_elapse_time();
		/*	if(std_power_paused==1)	{
//				set_std_pseq_power_pause_start_time(); //set standard time of pause
				pause_time=get_std_pseq_power_pause_elapse_time()+old_pause_time;
				elapse_time=get_std_pseq_power_elapse_time()-pause_time;
			}
			else	{
				elapse_time=get_std_pseq_power_elapse_time()-pause_time;
				old_pause_time=pause_time;
			}*/
			switch(state)
			{
			case idle:
				if(cycle_end==1)
				{
					if(acdet_on_done&acdet_off_done) //all acdet cycle is done
					{
						if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
						{
							if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
							{
								state= idle;
							}
						}
					}//all acdet cycle is done
					else if(!acdet_on_done&&acdet_off_done) //acdet on is not done yet
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_on_time TP0 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);
							acdet_on_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
							{
								if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
								{
									state= idle;
								}
							}
						}
					}//acdet on is not done yet
					else if(acdet_on_done&!acdet_off_done) //acdet off is not done yet
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_off_time TP0 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
							{
								if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
								{
									state= idle;
								}
							}
						}
					}//acdet off is not done yet
					else //all the acdet process is not done yet
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_on_time TP1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_off_time TP1 %d %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);
									acdet_off_done=1;
									if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
									{
										if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
										{
											state= idle;
										}
									}
								}
							}
						}
						else
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_off_time TP2 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_on_time TP2 %d %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(1);
									acdet_on_done=1;
									if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
									{
										if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
										{
											state= idle;
										}
									}
								}
							}
						}
					}//all the acdet process is not done yet
				}
				else if(flow <= 5) //start with i2c on 1
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c on 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_1_on;
							}
						}
						else // acd on comes before moving to 1 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_1>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c on 1(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=i2c_1_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c on 1(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=i2c_1_on;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c on 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_1_on;
							}
						}
						else //acd off comes before moving to 1 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_1>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c on 1(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=i2c_1_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c on 1(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=i2c_1_on;
								}
							}
						}
					}
				}
				else if(flow>5 && flow<=11) //start with i2c on 2
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c on 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_2_on;
							}
						}
						else // acd on comes before moving to 2 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_2>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c on 2(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=i2c_2_on;
									}
								}
								else // just passed to 2 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c on 2(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=i2c_2_on;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c on 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_2_on;
							}
						}
						else //acd off comes before moving to 2 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_2>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c on 2(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=i2c_2_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c on 2(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=i2c_2_on;
								}
							}
						}
					}
				}
				else if(flow <=17 && flow >11) //start with i2c off 1
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_1_off;
							}
						}
						else // acd on comes before moving to 1 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_1>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c off 1(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=i2c_1_off;
									}
								}
								else // just passed to 1 off after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c off 1(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=i2c_1_off;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_1_off;
							}
						}
						else //acd off comes before moving to 1 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_1>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c off 1(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=i2c_1_off;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c off 1(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=i2c_1_off;
								}
							}
						}
					}
				}
				else if(flow <=23 && flow >17) //start with i2c off 2
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c off 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_2_off;
							}
						}
						else // acd on comes before moving to 2 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
#if (I2C_TEST == 1)
								printf("# IDLE to i2c off 2(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
								if(pseq_buf[pseq_scnt].i2c_off_time_2>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c off 2(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=i2c_2_off;
									}
								}
								else // just passed to 2 off after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c off 2(acdet on)_: %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=i2c_2_off;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to i2c off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=i2c_2_off;
							}
						}
						else //acd off comes before moving to 2 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_2>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to i2c off 2(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=i2c_2_off;
									}
								}
								else // just passed to 2 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to i2c off 2(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=i2c_2_off;
								}
							}
						}
					}
				}
				else printf("Error occurred %d, %ld\n", pseq_scnt, elapse_time);
				break;

			case i2c_1_on:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))
				{
					if(flag_i2c_1_on_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# i2c 1 on: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_i2c_1_on;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# i2c 1 on disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_i2c_1_on;
					}
				}
			break;

			case acdet_i2c_1_on:
				if((flow==0) || (flow==1) || (flow==12) || (flow==16) || (flow==18) || (flow==22) ) //from 1 on to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1on&i2c2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_on Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on on 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1on&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1on&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_on;
						}
					}
					else //Nothing between i2c1on&i2c2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
					}
				}

				else if((flow==2) || (flow==3) || (flow==6) || (flow==10) || (flow==19) || (flow==20) ) //from 1 on to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1on&i2c1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_on Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on on 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1on&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1on&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_off;
						}
					}
					else //Nothing between i2c1on&i2c2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
					}
				}

				else if((flow==4) || (flow==5) || (flow==7) || (flow==8) || (flow==13) || (flow==14)  ) //from 1 on to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1on&i2c2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_on Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on on 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1on&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1on&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_off;
						}
					}
					else //Nothing between i2c1on&i2c2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
					}
				}

				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_on On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_on Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_on Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_i2c_1_on Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;
			case i2c_2_on:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))
				{
					if(flag_i2c_2_on_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# i2c 2 on: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcd,0x0000);
//						i2c_2_on_end = 1;
						state=acdet_i2c_2_on;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# i2c 2 on disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_i2c_2_on;
					}
				}
				break;
			case acdet_i2c_2_on:
				if((flow==6) || (flow==7) || (flow==14) || (flow==17) || (flow==20) || (flow==23) ) //from 2 on to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2on&i2c1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_on Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2on&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c2on&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_on;
						}
					}
					else //Nothing between i2c2on&i2c1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
					}
				}

				else if((flow==0) || (flow==4) || (flow==8) || (flow==9) || (flow==18) || (flow==21) ) //from 2 on to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2on&i2c1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_on Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2on&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c2on&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_off;
						}
					}
					else //Nothing between i2c2on&i2c1off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
					}
				}

				else if((flow==1) || (flow==2) || (flow==10) || (flow==11) || (flow==12) || (flow==15)  ) //from 2 on to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2on&i2c2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_on Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2on&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1on&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_off;
						}
					}
					else //Nothing between i2c2on&i2c2off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
					}
				}
				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_on On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								cycle_end=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_on Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_on Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_i2c_2_on Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;
			case i2c_1_off:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))
				{
					if(flag_i2c_1_off_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# i2c 1 off: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_i2c_1_off;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# i2c 1 off disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_i2c_1_off;
					}
				}
				break;

			case acdet_i2c_1_off:
				if((flow==8) || (flow==11) || (flow==12) || (flow==13) || (flow==21) || (flow==22) ) //from 1 off to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1off&i2c1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_off Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1off&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1off&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_on;
						}
					}
					else //Nothing between i2c1off&i2c1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
					}
				}
				else if((flow==2) || (flow==5) || (flow==14) || (flow==15) || (flow==19) || (flow==23) ) //from 1 off to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1off&i2c2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_off Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1off&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1off&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_on;
						}
					}
					else //Nothing between i2c1off&i2c2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
					}
				}

				else if((flow==0) || (flow==3) || (flow==6) || (flow==9) || (flow==16) || (flow==17)  ) //from 1 off to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c1off&i2c2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_off Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c1off&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c1off&i2c2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_off;
						}
					}
					else //Nothing between i2c1off&i2c2off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=i2c_2_off;
					}
				}

				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_1_off On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								cycle_end=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_1_off Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_1_off Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_i2c_1_off Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;

			case i2c_2_off:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))
				{
					if(flag_i2c_2_off_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# i2c 2 off: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_i2c_2_off;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# i2c 2 off disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_i2c_2_off;
					}
				}
				break;

			case acdet_i2c_2_off:
				if((flow==9) || (flow==10) || (flow==15) || (flow==16) || (flow==18) || (flow==19) ) //from 2 off to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2off&i2c1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_off Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2off&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c2off&i2c1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_on;
						}
					}
					else //Nothing between i2c2off&i2c1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=i2c_1_on;
					}
				}
				else if((flow==3) || (flow==4) || (flow==13) || (flow==17) || (flow==20) || (flow==21) ) //from 2 off to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2off&i2c2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_off Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2off&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c2off&i2c2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_2_on;
						}
					}
					else //Nothing between i2c2off&i2c2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=i2c_2_on;
					}
				}
				else if((flow==1) || (flow==5) || (flow==7) || (flow==11) || (flow==22) || (flow==23) ) //from 2 off to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between i2c2off&i2c1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	// ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_off Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between i2c2off&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between i2c2off&i2c1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=i2c_1_off;
						}
					}
					else //Nothing between i2c2off&i2c1off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=i2c_1_off;
					}
				}
				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_i2c_2_off On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_i2c_2_off Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_i2c_2_off Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_i2c_2_off Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;
			case stop:
				break;

		}
	}
	else if(std_power_seq_run==2)
	{
		state=idle;
	}
	else
	{
		state=idle;
	}
	usleep(500);
}

return NULL;
}


void *std_ocmd_pseq_acdet_thread(req_std_ocmd_pwr_seq_test_t *data)
{
//	uint64_t 	elapse_time;
	uint8_t 	acdet_on_done, acdet_off_done;

	printf("pseq acdet ocmd thread start\n");

	cycle_end = 0;
	acdet_on_done = 0;
	acdet_off_done = 0;
//	old_pause_time=0;

	typedef enum{idle, stop, ocmd_1_on, ocmd_2_on, ocmd_1_off, ocmd_2_off, acdet_ocmd_1_on, acdet_ocmd_2_on, acdet_ocmd_1_off, acdet_ocmd_2_off} ocmd_state;

	typedef enum{on1_on2_off1_off2, on1_on2_off2_off1, on1_off1_on2_off2, on1_off1_off2_on2, on1_off2_on2_off1, on1_off2_off1_on2,
				on2_on1_off1_off2, on2_on1_off2_off1, on2_off1_on1_off2, on2_off1_off2_on1, on2_off2_on1_off1, on2_off2_off1_on1,
				off1_on1_on2_off2, off1_on1_off2_on2, off1_on2_on1_off2, off1_on2_off2_on1, off1_off2_on1_on2, off1_off2_on2_on1,
				off2_on1_on2_off1, off2_on1_off1_on2, off2_on2_on1_off1, off2_on2_off1_on1, off2_off1_on1_on2, off2_off1_on2_on1} flow_chart;
	ocmd_state state;
	flow_chart flow;
	state=idle;

	while(1)
	{
		if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on1_on2_off1_off2;
				else flow = on1_on2_off2_off1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on1_off1_on2_off2;
				else flow = on1_off1_off2_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_1) flow = on1_off2_on2_off1;
				else flow = on1_off2_off1_on2;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on2_on1_off1_off2;
				else flow = on2_on1_off2_off1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = on2_off1_on1_off2;
				else flow = on2_off1_off2_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_1) flow = on2_off2_on1_off1;
				else flow = on2_off2_off1_on1;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_2) flow = off1_on1_on2_off2;
				else flow = off1_on1_off2_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_2) flow = off1_on2_on1_off2;
				else flow = off1_on2_off2_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_on_time_2) flow = off1_off2_on1_on2;
				else flow = off1_off2_on2_on1;
			}
		}
		else if((pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_off_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
		{
			if((pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2) && (pseq_buf[pseq_scnt].i2c_on_time_1 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].i2c_off_time_1) flow = off2_on1_on2_off1;
				else flow = off2_on1_off1_on2;
			}
			else if ((pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_on_time_2 < pseq_buf[pseq_scnt].i2c_off_time_1))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_off_time_1) flow = off2_on2_on1_off1;
				else flow = off2_on2_off1_on1;
			}
			else if ((pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_1) && (pseq_buf[pseq_scnt].i2c_off_time_1 < pseq_buf[pseq_scnt].i2c_on_time_2))
			{
				if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].i2c_on_time_2) flow = off2_off1_on1_on2;
				else flow = off2_off1_on2_on1;
			}
		}
		if(std_power_seq_run==3)
		{
//			elapse_time = get_std_pseq_power_elapse_time();
		/*	if(std_power_paused==1)	{
//				set_std_pseq_power_pause_start_time(); //set standard time of pause
				pause_time=get_std_pseq_power_pause_elapse_time()+old_pause_time;
				elapse_time=get_std_pseq_power_elapse_time()-pause_time;
			}
			else	{
				elapse_time=get_std_pseq_power_elapse_time()-pause_time;
				old_pause_time=pause_time;
			}*/
			switch(state)
			{


			case idle:
				if(cycle_end==1)
				{
					if(acdet_on_done&acdet_off_done) //all acdet cycle is done
					{
						if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
						{
							if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
							{
								state= idle;
							}
						}
					}//all acdet cycle is done
					else if(!acdet_on_done&&acdet_off_done) //acdet on is not done yet
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("OCMD acdet_on_time TP0 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);
							acdet_on_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
							{
								if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
								{
									state= idle;
								}
							}
						}
					}//acdet on is not done yet
					else if(acdet_on_done&!acdet_off_done) //acdet off is not done yet
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("OCMD acdet_off_time TP0 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
							{
								if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
								{
									state= idle;
								}
							}
						}
					}//acdet off is not done yet
					else //all the acdet process is not done yet
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("OCMD acdet_on_time TP1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("OCMD acdet_off_time TP1 %d %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);
									acdet_off_done=1;
									if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
									{
										if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
										{
											state= idle;
										}
									}
								}
							}
						}
						else
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
#if (I2C_TEST == 1)
								printf("OCMD acdet_off_time TP2 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
								{
#if (I2C_TEST == 1)
									printf("OCMD acdet_on_time TP2 %d %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(1);
									acdet_on_done=1;
									if(elapse_time>=pseq_buf[pseq_scnt].sequence_time)
									{
										if(pseq_rpcnt<pseq_buf[pseq_scnt].repeat)		//repeat start point
										{
											state= idle;
										}
									}
								}
							}
						}
					}//all the acdet process is not done yet
				}

				else if(flow <= 5) //start with ocmd on 1
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd on 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_1_on;
							}
						}
						else // acd on comes before moving to 1 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_1>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd on 1(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=ocmd_1_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd on 1(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=ocmd_1_on;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd on 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_1_on;
							}
						}
						else //acd off comes before moving to 1 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_1>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd on 1(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=ocmd_1_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd on 1(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1) state=ocmd_1_on;
								}
							}
						}
					}
				}
				else if(flow>5 && flow<=11) //start with ocmd on 2
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd on 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_2_on;
							}
						}
						else // acd on comes before moving to 2 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_2>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd on 2(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=ocmd_2_on;
									}
								}
								else // just passed to 2 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd on 2(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=ocmd_2_on;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd on 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_2_on;
							}
						}
						else //acd off comes before moving to 2 on
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_on_time_2>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd on 2(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=ocmd_2_on;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd on 2(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2) state=ocmd_2_on;
								}
							}
						}
					}
				}
				else if(flow <=17 && flow >11) //start with ocmd off 1
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_1_off;
							}
						}
						else // acd on comes before moving to 1 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_1>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd off 1(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=ocmd_1_off;
									}
								}
								else // just passed to 1 off after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd off 1(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=ocmd_1_off;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_1_off;
							}
						}
						else //acd off comes before moving to 1 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_1>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd off 1(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=ocmd_1_off;
									}
								}
								else // just passed to 1 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd off 1(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1) state=ocmd_1_off;
								}
							}
						}
					}
				}
				else if(flow <=23 && flow >17) //start with ocmd off 2
				{
					if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time) // acd on comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd off 2: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_2_off;
							}
						}
						else // acd on comes before moving to 2 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on
							{
								ext_cnt_vdd_sel(1);
								acdet_on_done=1;
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd off 2(acdet on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
								if(pseq_buf[pseq_scnt].i2c_off_time_2>pseq_buf[pseq_scnt].acdet_off_time) // dealing with acd off after acd on
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd off 2(acdet on/off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(0);
										acdet_off_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=ocmd_2_off;
									}
								}
								else // just passed to 2 off after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd off 2(acdet on)_: %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=ocmd_2_off;
								}
							}
						}
					}
					else //acd off comes first
					{
						if(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time) // just leaving
						{
							if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2)
							{
#if (I2C_TEST == 1)
								printf("# IDLE to ocmd off 1: %d, %ld\n", pseq_scnt, elapse_time);
#endif
								state=ocmd_2_off;
							}
						}
						else //acd off comes before moving to 2 off
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
							{
								ext_cnt_vdd_sel(0);
								acdet_off_done=1;
								if(pseq_buf[pseq_scnt].i2c_off_time_2>pseq_buf[pseq_scnt].acdet_on_time) // dealing with acd on after acd off
								{
									if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
									{
#if (I2C_TEST == 1)
										printf("# IDLE to ocmd off 2(acdet off/on): %d, %ld\n", pseq_scnt, elapse_time);
#endif
										ext_cnt_vdd_sel(1);
										acdet_on_done=1;
										if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=ocmd_2_off;
									}
								}
								else // just passed to 2 on after acd on
								{
#if (I2C_TEST == 1)
									printf("# IDLE to ocmd off 2(acdet off): %d, %ld\n", pseq_scnt, elapse_time);
#endif
									if(elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2) state=ocmd_2_off;
								}
							}
						}
					}
				}

				else printf("Error occurred %d, %ld\n", pseq_scnt, elapse_time);

				break;

			case ocmd_1_on:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))
				{
					if(flag_ocmd_1_on_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# ocmd 1 on: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_ocmd_1_on;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# ocmd 1 on disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_ocmd_1_on;
					}
				}
			break;


			case acdet_ocmd_1_on:
				if((flow==0) || (flow==1) || (flow==12) || (flow==16) || (flow==18) || (flow==22) ) //from 1 on to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1on&ocmd2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_on Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1on&ocmd2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1on&ocmd_2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_on;
						}
					}
					else //Nothing between ocmd1on&ocmd2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
					}
				}
				else if((flow==2) || (flow==3) || (flow==6) || (flow==10) || (flow==19) || (flow==20) ) //from 1 on to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1on&ocmd1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_on Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1on&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1on&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_off;
						}
					}
					else //Nothing between ocmd1on&ocmd2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
					}
				}

				else if((flow==4) || (flow==5) || (flow==7) || (flow==8) || (flow==13) || (flow==14)  ) //from 1 on to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1on&ocmd2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_on Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1on&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1on&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_off;
						}
					}
					else //Nothing between ocmd1on&ocmd2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
					}
				}

				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_on On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_on Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_on Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_ocmd_1_on Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;




			case ocmd_2_on:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))
				{
					if(flag_ocmd_2_on_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# ocmd 2 on: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcd,0x0000);
//						i2c_2_on_end = 1;
						state=acdet_ocmd_2_on;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# ocmd 2 on disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_ocmd_2_on;
					}
				}
				break;


			case acdet_ocmd_2_on:
				if((flow==6) || (flow==7) || (flow==14) || (flow==17) || (flow==20) || (flow==23) ) //from 2 on to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2on&ocmd1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_on Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2on&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd2on&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_on;
						}
					}
					else //Nothing between ocmd2on&ocmd1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
					}
				}

				else if((flow==0) || (flow==4) || (flow==8) || (flow==9) || (flow==18) || (flow==21) ) //from 2 on to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2on&ocmd1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_on Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2on&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd2on&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_off;
						}
					}
					else //Nothing between ocmd2on&ocmd1off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
					}
				}

				else if((flow==1) || (flow==2) || (flow==10) || (flow==11) || (flow==12) || (flow==15)  ) //from 2 on to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2on&ocmd2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_on Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2on&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1on&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_off;
						}
					}
					else //Nothing between i2c2on&i2c2off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
					}
				}
				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_on On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								cycle_end=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_on Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_on Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_ocmd_2_on Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;

			case ocmd_1_off:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))
				{
					if(flag_ocmd_1_off_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# ocmd 1 off: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_ocmd_1_off;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# ocmd 1 off disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_ocmd_1_off;
					}
				}
				break;




			case acdet_ocmd_1_off:
				if((flow==8) || (flow==11) || (flow==12) || (flow==13) || (flow==21) || (flow==22) ) //from 1 off to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1off&ocmd1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_off Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1off&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1off&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_on;
						}
					}
					else //Nothing between i2c1off&i2c1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
					}
				}
				else if((flow==2) || (flow==5) || (flow==14) || (flow==15) || (flow==19) || (flow==23) ) //from 1 off to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1off&ocmd2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_off Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1off&ocmd2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1off&ocmd2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_on;
						}
					}
					else //Nothing between ocmd1off&ocmd2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
					}
				}

				else if((flow==0) || (flow==3) || (flow==6) || (flow==9) || (flow==16) || (flow==17)  ) //from 1 off to 2 off
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd1off&ocmd2off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_off Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd1off&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd1off&ocmd2off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_off;
						}
					}
					else //Nothing between ocmd1off&ocmd2off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))	state=ocmd_2_off;
					}
				}

				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_1_off On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								cycle_end=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_1_off Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_1_off Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_ocmd_1_off Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;

			case ocmd_2_off:
				if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_2))
				{
					if(flag_ocmd_2_off_en == 1)
					{
#if (I2C_TEST == 1)
						printf("# ocmd 2 off: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcd,0x0001);
						usleep(100);
						FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcd,0x0000);
						state=acdet_ocmd_2_off;
					}
					else
					{
#if (I2C_TEST == 1)
						printf("# ocmd 2 off disabled: %d, %ld\n", pseq_scnt, elapse_time);
#endif
						state=acdet_ocmd_2_off;
					}
				}
				break;
			case acdet_ocmd_2_off:
				if((flow==9) || (flow==10) || (flow==15) || (flow==16) || (flow==18) || (flow==19) ) //from 2 off to 1 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2off&ocmd1on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off On 1 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_off Off 1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 1_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off off 1_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2off&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off On 2 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd2off&ocmd1on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 2 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_on;
						}
					}
					else //Nothing between ocmd2off&ocmd1on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_1))	state=ocmd_1_on;
					}
				}
				else if((flow==3) || (flow==4) || (flow==13) || (flow==17) || (flow==20) || (flow==21) ) //from 2 off to 2 on
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2off&ocmd2on
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)	//ACD on first
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off On 3 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_off Off 3 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 3_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off off 3_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2off&ocmd2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off On 4 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_on_time_2>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_on_time_2<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd2off&ocmd2on
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 4 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_2_on;
						}
					}
					else //Nothing between ocmd2off&ocmd2on
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_on_time_2))	state=ocmd_2_on;
					}
				}
				else if((flow==1) || (flow==5) || (flow==7) || (flow==11) || (flow==22) || (flow==23) ) //from 2 off to 1 off
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time))//ACDon&ACDoff between ocmd2off&ocmd1off
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off On 5 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_off Off 5 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
								}
							}
						}
						else	//ACD off first
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 5_1 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0); //0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off off 5_1 %d, %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1); //0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_on_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_off_time)))//ACDon between ocmd2off&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off On 6 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_1>=pseq_buf[pseq_scnt].acdet_off_time)&&
							((pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)||(pseq_buf[pseq_scnt].i2c_off_time_1<pseq_buf[pseq_scnt].acdet_on_time)))//ACDoff between ocmd2off&ocmd1off
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 6 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							state=ocmd_1_off;
						}
					}
					else //Nothing between i2c2off&i2c1off
					{
						if((elapse_time>=pseq_buf[pseq_scnt].i2c_off_time_1))	state=ocmd_1_off;
					}
				}
				else //finish
				{
					if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)) //ACDon ACDoff after sequence
					{
						if(pseq_buf[pseq_scnt].acdet_on_time<pseq_buf[pseq_scnt].acdet_off_time)
						{
							if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
							{
#if (I2C_TEST == 1)
								printf("acdet_ocmd_2_off On 7 %d %ld\n", pseq_scnt, elapse_time);
#endif
								ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
								acdet_on_done=1;
								if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
								{
#if (I2C_TEST == 1)
									printf("acdet_ocmd_2_off Off 7 %d, %ld\n", pseq_scnt, elapse_time);
#endif
									ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
									acdet_off_done=1;
									cycle_end=1;
									state=idle;
								}
							}
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_on_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_off_time)) //ACDon after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_on_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off On 8 %d %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V, 2=5V
							acdet_on_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else if((pseq_buf[pseq_scnt].i2c_off_time_2<pseq_buf[pseq_scnt].acdet_off_time)&&(pseq_buf[pseq_scnt].i2c_off_time_2>=pseq_buf[pseq_scnt].acdet_on_time)) //ACDoff after sequence
					{
						if(elapse_time>=pseq_buf[pseq_scnt].acdet_off_time)
						{
#if (I2C_TEST == 1)
							printf("acdet_ocmd_2_off Off 8 %d, %ld\n", pseq_scnt, elapse_time);
#endif
							ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V, 2=5V
							acdet_off_done=1;
							cycle_end=1;
							state=idle;
						}
					}
					else
					{
#if (I2C_TEST == 1)
						printf("acdet_ocmd_2_off Fin without ACDet %d, %ld\n", pseq_scnt, elapse_time);
#endif
						cycle_end=1;
						state=idle;
					}
				}
				break;
			case stop:
				break;

		}

	}
	else if(std_power_seq_run==2)
	{
		state=idle;
	}
	else
	{
		state=idle;
	}
	usleep(500);
}

return NULL;
}




void inspect_hub_init(gboolean chk)
{
	bt_count = get_reboot_count();

	printf("dev init chk = %d\t%d\n", chk, bt_count);
	if(chk)		//initializing failed
	{
		bt_count = get_reboot_count();			// read boot_count file
		if (bt_count>3)		set_reboot_count(0);	// reset boot_count and pass init_failed
		else
		{
			set_reboot_count(bt_count+1);			//test ok
			ERRCHECK(system("reboot"));
		}
	}
	else
	{
		set_reboot_count(0);
	}
}


//char flag_wset;
//
//void chk_wtime(void)
//{
//	uint64_t	elapse_time;
//
//	if(!wtime_chk) return;
//
//	if(!flag_wset) {
//		set_w_time();
//		flag_wset=1;
//	}
//	else {
//		elapse_time=get_w_time();
//
//		printf("%d\n", elapse_time);
//		flag_wset=0;
//		wtime_chk=0;
//	}
//}

int main()
{

#ifdef USE_SHARED_MEM
//	void 		*sm=(void *)0;
	void 		*sm;

	if((status_shm_pid=shmget((key_t)SHM_KEY,sizeof(shared_status_t),0666|IPC_CREAT))<0){
		printf("shmget() fail\n");
	}
	sm=shmat(status_shm_pid,(void *)0,0);
//
	if(sm<((void *)0)){
		printf("Shared Memory fail\n");
	}
	status_shm=(shared_status_t *)sm;
#endif

//	ERRCHECK(system("stty -a < /dev/ttyTHS1"));
//	ERRCHECK(system("stty -a < /dev/ttyUSB0"));


	model_selection_end=NACK;
	group_selection_end=NACK;
	pattern_change_error=0;
	adim_change_flag=0;

//	old_index=4095;//test
//	new_index=0;//test


	print_title();
	create_dir();

	sig_setup();

	variables_init();
	msq_init();
	poll_init();

	i2c_gpio_init();

//	if( NULL == rcb_init() ) 	goto EXIT_JUMP;
	inspect_hub_init(NULL == rcb_init());
	if( NULL == pwr_init() )	goto EXIT_JUMP;

#if defined(USE_I2C_LCD)	// unused this project
	i2c_lcd_init();
#endif


	memset(qems_eqpid,0,sizeof(qems_eqpid));
	get_qems_info(qems_eqpid);

	if( 1 == ip_check() )
	{
		fprintf(stderr, "IP address changed!\n");
		set_reboot(1);
		goto EXIT_JUMP;
	}

	//ES628
	memset(power1_ip_info, 0, MAX_IP_ADDR);
	memset(power2_ip_info, 0, MAX_IP_ADDR);
	get_power_ip(power1_ip_info, power2_ip_info);

	rcb_start();
	usleep(2000000);	// version info visible for 2 second


	if(0==FPGA_update(-1, -1))	// In case of booting, you must update FPGA!!
	{
		rcb_write(rcb_fd, RCB_LINE2, "FPGA FAILED!");
		fprintf(stderr, "FPGA update failed!\n");
		usleep(2000000);
//		goto EXIT_JUMP;
	}

#if defined(USE_RCB485)
	if( NULL == rcb485_init() )	goto EXIT_JUMP;	// after downloading FPGA, this process must be done!
#endif

	FPGA_Open(FPGA_SPI_MODE, FPGA_SPI_BITS, FPGA_SPI_SPEED, FPGA_SPI_DELAY);

#ifdef USE_SUB_BOARD_ID
	get_sub_board_id();
#endif

	model_init();
#ifdef GROUP_SELECT
	group_init();
#endif
//	rcb_model_change();


/*
	model_selection_end = model_select(model_name, 0);
	model_variable_reset();

#ifdef GROUP_SELECT
	if (model_selection_end==ACK)	group_selection_end = group_select(group_list[group_idx]);

	if( (model_selection_end==ACK)&&(group_selection_end==ACK) )
	{
		rcb_ready_screen(ACK);
		set_opmode(READY);
	}
	else if ( (model_selection_end==ACK)&&(group_selection_end==NACK) )
	{
		rcb_group_change(group_init());
	}
	else
	{
		rcb_model_change();
	}
#endif
*/

	// 19-06-21 Vby1 re-driver option MUX setting
	re_driver_mux_default();
//	set_pre_emphasis_default();

//	task_thread(10, load_cpu);		//disable 19-07-03
	start_shell();
	font_init();

	task_thread(500, link_check_thread);

#ifdef EXTERNAL_POWER_CTRL
	task_thread(100, External_power_link_check_thread);
#endif

#ifdef USE_SHARED_MEM
//	task_thread(501, status_check_thread);		//test
#endif

	task_thread(200, std_pseq_acdet_thread);
	task_thread(200, std_ocmd_pseq_acdet_thread);
	task_thread(200, std_pseq_power_thread);
//	task_thread(504, std_pseq_i2c_thread);

	pll_interval_time = 300;

//	rcb_ready_screen(ACK);
//	set_opmode(READY);
	rcb_model_change();

	// ELVDD Alarm
	get_elvdd_alarm_flag_global();
	old_scan = FPGA_Read(FPGA_MEM_WR_CTRL);

	while(1)
	{
		if(exit_flag || get_reboot()) break;

		FPGA_key_scan(); // 24.02.21 temporary del

		fpga_read_scan();

		// from COMMUNICATOR(tcp)
		msg_analyze();

		// rolling pattern

		pattern_roll();
//		i2c_idd_sensing();
//		pwr_schedule_test();


//		pwr_offset_checking();
//		pwr_sensing();
//		pwr_version_checking();

//		video_player_check();		//gst-launch-1.0 only

		// usb detection
		usb_detect_monitoring();


		//test
//		chk_wtime();

		rcb_string_slide();

		// internal polling timeout
		poll_loop(POLL_LOOP_TIME);
	}

EXIT_JUMP:

	free_sub_bmp_list();
	free_file_list();

	FPGA_Close();
	screen_close();
	i2c_gpio_close();
	if(rcb_fd!=NULL) rcb_close(rcb_fd);
	if(pwr_fd!=NULL) pwr_close(pwr_fd);

	printf("\n %s process exit! \n", PG_NAME);

	if(get_reboot())
	{
		ERRCHECK(system("umount /dev/sda1"));
		ERRCHECK(system("reboot"));
	}

	return EXIT_SUCCESS;
}

