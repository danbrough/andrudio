#ifndef _PACKET_QUEUE_H_
#define _PACKET_QUEUE_H_

#include <libavformat/avformat.h>
#include <pthread.h>


typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} PacketQueue;

void packet_queue_library_init();

void packet_queue_init(PacketQueue *q);

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);

int packet_queue_put(PacketQueue *q, AVPacket *pkt);

void packet_queue_abort(PacketQueue *q);

void packet_queue_end(PacketQueue *q);

void packet_queue_flush(PacketQueue *q);

#endif
