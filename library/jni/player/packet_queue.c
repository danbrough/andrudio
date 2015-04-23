#include "packet_queue.h"

AVPacket flush_pkt;

void packet_queue_library_init() {
	av_init_packet(&flush_pkt);
	flush_pkt.data = (uint8_t *) &flush_pkt;
}

/* packet queue handling */
void packet_queue_init(PacketQueue *q) {
	memset(q, 0, sizeof(PacketQueue));
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond, NULL);
	packet_queue_put(q, &flush_pkt);
}

void packet_queue_flush(PacketQueue *q) {
	AVPacketList *pkt, *pkt1;

	pthread_mutex_lock(&q->mutex);
	for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	pthread_mutex_unlock(&q->mutex);
}

void packet_queue_end(PacketQueue *q) {
	packet_queue_flush(q);
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->cond);
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
	AVPacketList *pkt1;

	/* duplicate the packet */
	if (pkt != &flush_pkt && av_dup_packet(pkt) < 0)
		return -1;

	pkt1 = av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	pthread_mutex_lock(&q->mutex);

	if (!q->last_pkt)

		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	/* should duplicate packet data in DV case */

	pthread_cond_signal(&q->cond);

	pthread_mutex_unlock(&q->mutex);
	return 0;
}

void packet_queue_abort(PacketQueue *q) {
	pthread_mutex_lock(&q->mutex);

	q->abort_request = 1;

	pthread_cond_signal(&q->cond);

	pthread_mutex_unlock(&q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
	AVPacketList *pkt1;
	int ret;

	pthread_mutex_lock(&q->mutex);

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size + sizeof(*pkt1);
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			pthread_cond_wait(&q->cond, &q->mutex);
		}
	}
	pthread_mutex_unlock(&q->mutex);
	return ret;
}
