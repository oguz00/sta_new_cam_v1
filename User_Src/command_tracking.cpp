
/**
 * @file commands_tracking.c
 * @brief Circular buffer implementasyonu
 *
 * @author oguz00
 * @date 2025-11-17
 * @version 1.0
 */

#include "command_tracking.h"
#include"main.h"/* HAL_GetTick() için gerekli*/



/* Public Fonksiyonlar */

void CmdRingBuffer_Init (cmdRingBuffer_t *ring_buf_ptr)
{
	if(ring_buf_ptr!=nullptr)
	{
		/*Tüm yapıyı sıfırla*/
		(void)memset(ring_buf_ptr,0,sizeof(cmdRingBuffer_t));
		 /* Pointer'lari baslangica al */
		ring_buf_ptr->head=0U;
		ring_buf_ptr->tail=0U;
		ring_buf_ptr->count=0U;
	}
}
bool CmdRingBuffer_PushComplete(
		cmdRingBuffer_t *ring_buf_ptr,
//		const uint8_t *expected_resp_ptr,
		uint32_t resp_len,
		const uint8_t *orig_req_ptr,
		uint32_t req_len,
		queryBitEnum query_type,
		const void *mapping_ptr)
{
		bool result=false;
		cmdBlock_t *block_ptr;

		/* parametre kontrolleri*/
		if((ring_buf_ptr!=nullptr) && (orig_req_ptr != nullptr) && (mapping_ptr !=nullptr))

			/* buffer dolu mu kontrol et*/
			if(ring_buf_ptr->count < CMD_BUFFER_SIZE)
			{
				/*Uzunluk sınırlar içinde mi kontrol et*/
				if((req_len<=CMD_MAX_LENGTH) && (resp_len<= CMD_MAX_LENGTH))
				{
					/*Yeni slot'un pointer'ını al */
					block_ptr=&ring_buf_ptr->buffer[ring_buf_ptr->head];
					/*Block'u temizle */
					(void)memset(block_ptr,0,sizeof(cmdBlock_t));
					/* Beklenen yanıt varsa kopyala*/
//					if(expected_resp_ptr!=nullptr)
//					{
//						(void)memcpy(block_ptr->expected_response, expected_resp_ptr, resp_len);
//						  block_ptr->response_length = resp_len;
//					}
					/* Orjinal istegi kopyala */
					(void)memcpy(block_ptr->original_request, orig_req_ptr, req_len);
					block_ptr->request_lenth = req_len;
	                /* Metadata'yi kaydet */
	                block_ptr->nmbr = query_type;
	                block_ptr->timestamp = HAL_GetTick();  /* Suanki zamani al */
	                block_ptr->mapping = mapping_ptr;
	                /* Head pointer'i ilerlet (circular) */
	                ring_buf_ptr->head = (ring_buf_ptr->head + 1U) % CMD_BUFFER_SIZE;
	                ring_buf_ptr->count = ring_buf_ptr->count + 1U;
	                result = true;
				}
			}
	    return result;
}
bool CmdRingBuffer_Pop(
		cmdRingBuffer_t *ring_buf_ptr,
		cmdBlock_t *block_ptr)
{
	bool result=false;
	/* Parametre kontrolü*/
	if((ring_buf_ptr!=nullptr) && (block_ptr !=nullptr))
	{
		/* Buffer bos değil mi kontrol et */
		if(ring_buf_ptr->count >0U)
		{

			/* En eski entry'yi kopyalaa */
			(void)memcpy(block_ptr, &ring_buf_ptr->buffer[ring_buf_ptr->tail],
					sizeof(cmdBlock_t));
			 /* Tail pointer'i ilerlet (circular) */
			ring_buf_ptr->tail= (ring_buf_ptr->tail +1U) &CMD_BUFFER_SIZE;
			ring_buf_ptr->count=ring_buf_ptr->count -1U;

			result=true;
		}

	}
	return result;
}

bool CmdRingBuffer_Peek(
    const cmdRingBuffer_t *ring_buf_ptr,
    cmdBlock_t *block_ptr)
{
    bool result = false;

    /* Parametre kontrolu */
    if ((ring_buf_ptr != NULL) && (block_ptr != NULL)) {

        /* Buffer bos degil mi kontrol et */
        if (ring_buf_ptr->count > 0U) {

            /* En eski entry'yi kopyala (silme) */
            (void)memcpy(block_ptr, &ring_buf_ptr->buffer[ring_buf_ptr->tail], sizeof(cmdBlock_t));

            result = true;
        }
    }

    return result;
}

bool CmdRingBuffer_IsEmpty(const cmdRingBuffer_t *ring_buf_ptr)
{
    bool result = true;

    /* NULL kontrolu */
    if (ring_buf_ptr != NULL) {
        result = (ring_buf_ptr->count == 0U);
    }

    return result;
}

bool CmdRingBuffer_IsFull(const cmdRingBuffer_t *ring_buf_ptr)
{
    bool result = false;

    /* NULL kontrolu */
    if (ring_buf_ptr != NULL) {
        result = (ring_buf_ptr->count == CMD_BUFFER_SIZE);
    }

    return result;
}

uint32_t CmdRingBuffer_Size(const cmdRingBuffer_t *ring_buf_ptr)
{
    uint32_t result = 0U;

    /* NULL kontrolu */
    if (ring_buf_ptr != NULL) {
        result = ring_buf_ptr->count;
    }

    return result;
}

void CmdRingBuffer_Clear(cmdRingBuffer_t *ring_buf_ptr)
{
    /* NULL kontrolu */
    if (ring_buf_ptr != NULL) {

        /* Pointer'lari sifirla */
        ring_buf_ptr->head = 0U;
        ring_buf_ptr->tail = 0U;
        ring_buf_ptr->count = 0U;

        /* Buffer icerigini temizle */
        (void)memset(ring_buf_ptr->buffer, 0, sizeof(ring_buf_ptr->buffer));
    }
}


bool CmdRingBuffer_RemoveIfTimeout(
    cmdRingBuffer_t *ring_buf_ptr,
    uint32_t timeout_ms,
    uint32_t current_time)
{
    bool result = false;
    uint32_t elapsed_time;
    uint32_t oldest_timestamp;

    /* NULL kontrolu */
    if (ring_buf_ptr != NULL) {

        /* Buffer bos degil mi kontrol et */
        if (ring_buf_ptr->count > 0U) {

            /* En eski komutun zamanini al */
            oldest_timestamp = ring_buf_ptr->buffer[ring_buf_ptr->tail].timestamp;

            /* Gecen zamani hesapla (uint32_t wraparound'u otomatik hallolur) */
            elapsed_time = current_time - oldest_timestamp;

            /* Timeout asildi mi kontrol et */
            if (elapsed_time >= timeout_ms) {

                /* En eski komutu kaldir */
                ring_buf_ptr->tail = (ring_buf_ptr->tail + 1U) % CMD_BUFFER_SIZE;
                ring_buf_ptr->count = ring_buf_ptr->count - 1U;

                result = true;
            }
        }
    }

    return result;
}



































