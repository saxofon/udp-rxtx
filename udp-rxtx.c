/*
 *
 * udp-rxtx
 *
 * Author  : Per Hallsmark <per.hallsmark@windriver.com>
 *
 * License : GPLv2
 *
 * Yet another network traffic generator for testing...
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <sched.h>
#include <pthread.h>

#define MAX_CLIENTS         1
#define MAX_CPUS           15

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT     5555

#define DEFAULT_PKTDATASIZE 22
#define DEFAULT_PKTRATE      2
#define DEFAULT_PKTSPERCORE 50000

static char *server_ip = "0.0.0.0";
static short server_port = DEFAULT_PORT;
static char *connect2ip = DEFAULT_IP;

static uint32_t pdsz = DEFAULT_PKTDATASIZE;
static uint32_t pps = DEFAULT_PKTRATE;
static uint32_t ppc = DEFAULT_PKTSPERCORE;

static int rx=0, tx=0, burst=0;

struct txtd_s {
	uint32_t pps;
};

static void diep(char *s)
{
	perror(s);
	exit(1);
}

static struct timespec add_ts(struct timespec time1, struct timespec time2)
{
	struct timespec result;

	result.tv_sec = time1.tv_sec + time2.tv_sec;
	result.tv_nsec = time1.tv_nsec + time2.tv_nsec;
	if (result.tv_nsec >= 1000000000UL) {
		result.tv_sec++;
		result.tv_nsec -= 1000000000UL;
	}

	return (result);
}

static void *rxt(void *arg)
{
	int status, len;
	struct sockaddr_in sa_me, sa_peer;
	int me;
	uint8_t *buf = malloc(pdsz);
	struct sched_param param;
	int priority;

	param.sched_priority = 60;
	status = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (status) {
		printf("%s: couldn't set prio (%d)\n", __FUNCTION__, status);
	}

	if ((me = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset(&sa_me, 0, sizeof(sa_me));
	sa_me.sin_family = AF_INET;
	sa_me.sin_port = htons(server_port);
	sa_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(me, (struct sockaddr *)&sa_me, sizeof(sa_me)) == -1) {
		diep("bind");
	}

	for (;;) {
		status = recv(me, buf, pdsz, 0);
		if (status > 0) {
			len = status;
			//printf("%s: got %d bytes\n", __FUNCTION__, status);
		} else if (status < 0) {
			printf("%s: recv failure (%d)\n", __FUNCTION__, status);
			perror("recv");
		} else {
		}
	}

	free(buf);
}

static void *txt_spread(void *arg)
{
	int s;
	struct sockaddr_in sa_peer;
	struct timespec mark, period;
	uint32_t nsec;
	uint8_t *buf = malloc(pdsz);
	int status;
	struct sched_param param;
	int priority;
	struct txtd_s *txtd = (struct txtd_s *)arg;

	param.sched_priority = 30;
	status = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (status) {
		printf("%s: couldn't set prio (%d)\n", __FUNCTION__, status);
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &sa_peer, 0, sizeof(sa_peer));
	sa_peer.sin_family = AF_INET;
	sa_peer.sin_port = htons(server_port);
	if (inet_aton(connect2ip, &sa_peer.sin_addr)==0) {
		diep("inet_aton");
	}

	nsec = 1000000000 / txtd->pps;
	period.tv_sec = 0;
	period.tv_nsec = (int)(nsec);

	printf("spread %d packets/sec (interval around %d nsecs)\n", txtd->pps, nsec);

	clock_gettime(CLOCK_MONOTONIC, &mark);
	mark = add_ts(mark, period);
	for (;;) {
		status = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mark, NULL);
		if (status != 0) {
			printf("sendto returns %d\n", status);
		}
		mark = add_ts(mark, period);

		status = sendto(s, buf, pdsz, 0, (struct sockaddr*)&sa_peer, sizeof(sa_peer));
		if (status != pdsz) {
			printf("sendto returns %d\n", status);
		}
	}

	free(buf);
}

static void *txt_burst(void *arg)
{
	int s;
	struct sockaddr_in sa_peer;
	struct timespec mark, period;
	uint32_t nsec;
	uint8_t *buf = malloc(pdsz);
	int status;
	struct sched_param param;
	int priority;
	struct txtd_s *txtd = (struct txtd_s *)arg;
	int i;

	param.sched_priority = 30;
	status = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
	if (status) {
		printf("%s: couldn't set prio (%d)\n", __FUNCTION__, status);
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &sa_peer, 0, sizeof(sa_peer));
	sa_peer.sin_family = AF_INET;
	sa_peer.sin_port = htons(server_port);
	if (inet_aton(connect2ip, &sa_peer.sin_addr)==0) {
		diep("inet_aton");
	}

	period.tv_sec = 1;
	period.tv_nsec = 0;

	printf("burst %d packets/sec \n", txtd->pps);

	clock_gettime(CLOCK_MONOTONIC, &mark);
	mark = add_ts(mark, period);
	for (;;) {
		status = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mark, NULL);
		if (status != 0) {
			printf("sendto returns %d\n", status);
		}
		mark = add_ts(mark, period);

		for (i=0; i<txtd->pps; i++) {
			status = sendto(s, buf, pdsz, 0, (struct sockaddr*)&sa_peer, sizeof(sa_peer));
			if (status != pdsz) {
				printf("sendto returns %d\n", status);
			}
		}
	}

	free(buf);
}

int main(int argc, char *argv[])
{
	int opt;
	int status;
	int i;
	pthread_t rtid, ttid[MAX_CPUS];
	struct txtd_s txtd[MAX_CPUS];
	cpu_set_t cpuset;

	while ((opt = getopt(argc, argv, "bc:l:p:rt:")) != -1) {
		switch (opt) {
			case 'b': // burst 
				burst = 1;
				break;
			case 'c': // packets per core
				ppc = atol(optarg);
				break;
			case 'l': // total packet length
				pdsz = atol(optarg)-42;
				break;
			case 'p': // packets per sec
				pps = atol(optarg);
				break;
			case 'r': // run as reciever
				rx = 1;
				break;
			case 't': // run as sender
				tx = 1;
				connect2ip = strdup(optarg);
				break;
		}
	}

	rtid = 0;
	for (i=0; i<MAX_CPUS; i++) {
		ttid[i] = 0;
	}

	if (rx) {
		// create rx thread
		status = pthread_create(&rtid, NULL, rxt, NULL);
	}

	if (tx) {
		for (i=0; i<MAX_CPUS; i++) {
			if (ppc*(i+1) > pps) {
				txtd[i].pps = pps - ppc*i;
			} else {
				txtd[i].pps = ppc;
			}
				
			// create tx thread
			if (burst) {
				status = pthread_create(&ttid[i], NULL, txt_burst, &txtd[i]);
			} else {
				status = pthread_create(&ttid[i], NULL, txt_spread, &txtd[i]);
			}

			// lock each thread to its own core, start from 1
			// as core 0 may be the one handling irqs
			CPU_ZERO(&cpuset);
			CPU_SET(((i+1)==MAX_CPUS)?0:i+1, &cpuset);
			status = pthread_setaffinity_np(ttid[i], sizeof(cpu_set_t), &cpuset);

			if ((ppc*(i+1)+1) > pps) {
				break;
			}
		}
	}

	// now we just have to sit and wait for our threads todo the work...
	if (rtid) status = pthread_join(rtid, NULL);
	for (i=0; i<MAX_CPUS; i++) {
		if (ttid[i]) status = pthread_join(ttid[i], NULL);
	}

	exit(0);
}
