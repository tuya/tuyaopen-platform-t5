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

#pragma once

#include "media_mailbox_list_util.h"
#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>

#ifdef __cplusplus
extern "C" {
#endif

void uvc_pipeline_event_handle(media_mailbox_msg_t *msg);
bk_err_t uvc_pipeline_init(void);

#ifdef __cplusplus
}
#endif

