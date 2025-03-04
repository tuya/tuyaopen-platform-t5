// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "stdio.h"
#include <driver/audio_ring_buff.h>
#include "aud_tras.h"


#define AUD_TRAS "aud_tras"

#define LOGI(...) BK_LOGI(AUD_TRAS, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_TRAS, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_TRAS, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_TRAS, ##__VA_ARGS__)

#define TU_QITEM_COUNT      (40)

#define AUD_TRAS_BUFF_SIZE    (320 * 5)

typedef struct
{
	beken_thread_t aud_tras_task_hdl;
	beken_queue_t aud_tras_int_msg_que;
	uint8_t *aud_tras_buff_addr;
	RingBufferContext aud_tras_rb;			//save mic data needed to send by aud_tras task
} aud_tras_info_t;

static aud_tras_setup_t aud_trs_setup_bk = {0};
static aud_tras_info_t *aud_tras_info = NULL;


bk_err_t aud_tras_send_msg(aud_tras_op_t op, void *param)
{
	bk_err_t ret;
	aud_tras_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (aud_tras_info->aud_tras_int_msg_que) {
		ret = rtos_push_to_queue(&aud_tras_info->aud_tras_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("aud_tras_send_int_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

bk_err_t aud_tras_deinit(void)
{
	bk_err_t ret;
	aud_tras_msg_t msg;

	msg.op = AUD_TRAS_EXIT;
	msg.param = NULL;
	if (aud_tras_info->aud_tras_int_msg_que) {
		ret = rtos_push_to_queue_front(&aud_tras_info->aud_tras_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("audio send msg: AUD_TRAS_EXIT fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void aud_tras_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	uint32_t fill_size = 0;
	aud_tras_setup_t *aud_trs_setup = NULL;
	aud_trs_setup = (aud_tras_setup_t *)param_data;
	uint8_t *aud_temp_data = NULL;
	GLOBAL_INT_DECLARATION();
	aud_temp_data = os_malloc(320);
	if (!aud_temp_data)
	{
		LOGE("malloc aud_temp_data\n");
		goto aud_tras_exit;
	}
	os_memset(aud_temp_data, 0, 320);

	aud_tras_send_msg(AUD_TRAS_TX, NULL);

	aud_tras_msg_t msg;
	while (1) {
		ret = rtos_pop_from_queue(&aud_tras_info->aud_tras_int_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUD_TRAS_IDLE:
					break;

				case AUD_TRAS_EXIT:
					LOGD("goto: AUD_TRAS_EXIT \r\n");
					goto aud_tras_exit;
					break;

				case AUD_TRAS_TX:
					fill_size = ring_buffer_get_fill_size(&aud_tras_info->aud_tras_rb);
					for (int n = 0; n < fill_size/320; n++) {
//						GPIO_UP(6);
						GLOBAL_INT_DISABLE();
						ring_buffer_read(&aud_tras_info->aud_tras_rb, aud_temp_data, 320);
						GLOBAL_INT_RESTORE();
						aud_trs_setup->aud_tras_send_data_cb(aud_temp_data, 320);
//						GPIO_DOWN(6);
					}

					rtos_delay_milliseconds(5);
					aud_tras_send_msg(AUD_TRAS_TX, NULL);
					break;

				default:
					break;
			}
		}
	}

aud_tras_exit:
	if (aud_temp_data) {
		os_free(aud_temp_data);
		aud_temp_data == NULL;
	}

	if (aud_tras_info->aud_tras_buff_addr) {
		ring_buffer_clear(&aud_tras_info->aud_tras_rb);
		os_free(aud_tras_info->aud_tras_buff_addr);
		aud_tras_info->aud_tras_buff_addr == NULL;
	}

	/* delete msg queue */
	ret = rtos_deinit_queue(&aud_tras_info->aud_tras_int_msg_que);
	if (ret != kNoErr) {
		LOGE("delete message queue fail \r\n");
	}
	aud_tras_info->aud_tras_int_msg_que = NULL;
	LOGI("delete aud_tras_int_msg_que \r\n");

	os_memset(&aud_trs_setup_bk, 0, sizeof(aud_tras_setup_t));

	/* delete task */
	aud_tras_info->aud_tras_task_hdl = NULL;

	if (aud_tras_info)
	{
		os_free(aud_tras_info);
		aud_tras_info = NULL;
	}

	rtos_delete_thread(NULL);
}

bk_err_t aud_tras_init(aud_tras_setup_t *setup_cfg)
{
	bk_err_t ret = BK_OK;

	aud_tras_info = os_malloc(sizeof(aud_tras_info_t));

	if (aud_tras_info == NULL)
	{
		LOGE("malloc aud_tras_info\n");
		return BK_FAIL;
	}
	os_memset(aud_tras_info, 0, sizeof(aud_tras_info_t));

	aud_tras_info->aud_tras_buff_addr = os_malloc(AUD_TRAS_BUFF_SIZE + 4);
	if (!aud_tras_info->aud_tras_buff_addr) {
		LOGE("malloc aud_tras_buff_addr\n");
		goto out;
	}
	ring_buffer_init(&aud_tras_info->aud_tras_rb, aud_tras_info->aud_tras_buff_addr, AUD_TRAS_BUFF_SIZE + 4, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	LOGD("aud_tras_info->aud_tras_rb: %p \n", &aud_tras_info->aud_tras_rb);

	os_memcpy(&aud_trs_setup_bk, setup_cfg, sizeof(aud_tras_setup_t));

	if ((!aud_tras_info->aud_tras_task_hdl) && (!aud_tras_info->aud_tras_int_msg_que))
	{
		ret = rtos_init_queue(&aud_tras_info->aud_tras_int_msg_que,
							  "aud_tras_int_que",
							  sizeof(aud_tras_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr)
		{
			LOGE("ceate audio transfer internal message queue fail\n");
			goto out;
		}
		LOGI("ceate audio transfer internal message queue complete\n");

		ret = rtos_create_thread(&aud_tras_info->aud_tras_task_hdl,
								 4,
								 "aud_tras",
								 (beken_thread_function_t)aud_tras_main,
								 1024 * 2,
								 (beken_thread_arg_t)&aud_trs_setup_bk);
		if (ret != kNoErr)
		{
			LOGE("Error: Failed to create aud_tras task \n");
			return kGeneralErr;
		}

		LOGI("init aud_tras task complete \n");
	}
	else
	{
		goto out;
	}

	return BK_OK;

out:
	if (aud_tras_info->aud_tras_int_msg_que)
	{
		ret = rtos_deinit_queue(&aud_tras_info->aud_tras_int_msg_que);
		if (ret != kNoErr) {
			LOGE("delete message queue fail \r\n");
		}
		aud_tras_info->aud_tras_int_msg_que = NULL;
	}

	if (aud_tras_info->aud_tras_buff_addr) {
		ring_buffer_clear(&aud_tras_info->aud_tras_rb);
		os_free(aud_tras_info->aud_tras_buff_addr);
		aud_tras_info->aud_tras_buff_addr == NULL;
	}

	if (aud_tras_info)
	{
		os_free(aud_tras_info);
		aud_tras_info = NULL;
	}

	return BK_FAIL;
}

RingBufferContext *aud_tras_get_tx_rb(void)
{
	return &aud_tras_info->aud_tras_rb;
}

