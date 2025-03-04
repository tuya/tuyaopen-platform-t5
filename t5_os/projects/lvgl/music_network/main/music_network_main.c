#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"
#include "media_service.h"
#if (CONFIG_SYS_CPU0)
#include "lcd_act.h"
#include "media_app.h"
#if CONFIG_LVGL
#include "lv_vendor.h"
#include "lv_demo_music.h"
#endif
#include "driver/drv_tp.h"
#include <driver/lcd.h>
#include "audio_player.h"
#include "netstream_player.h"
//#include "aud_intf.h"
//#include "aud_intf_types.h"

extern uint8_t netstream_download_stop_flag;
#endif

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#if (CONFIG_SYS_CPU0)
#define CMDS_COUNT  (sizeof(s_music_commands) / sizeof(struct cli_command))


uint32_t current_song, song_number;

char *song_list[] = {
	"http://dl.bekencorp.com/d/test/2023/music/myheart.mp3?sign=R9SLp-MvEEkiauuuKDmPsyV_74wePmKsRdJbFWw4mBo=:0",
	"http://dl.bekencorp.com/d/test/2023/music/qingtian.mp3?sign=YzsQI3YhI1QU6KQ9trGAzMo6OHJxAo0aMVNQtr2DLsg=:0",
	"http://dl.bekencorp.com/d/test/2023/music/chengdu.mp3?sign=WSKOehaDFGv8csOmTEGjHXHLaygTurgT5NEgjgw4jCY=:0",
};

const lcd_open_t lcd_open =
{
	//.device_ppi = PPI_800X480,
	//.device_name = "h050iwv",
	.device_ppi = PPI_480X480,
	.device_name = "st7701s",
};

void cli_music_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	if (os_strcmp(argv[1], "connect") == 0 && !audio_player_wifi_connect_status_get()){
		ret = netstream_sta_connect(argv[2], argv[3]);
		if (ret != BK_OK)	{
			os_printf("audio player connetc fail\r\n");
			return;
		}
	}
	else if(audio_player_wifi_connect_status_get())
	{
		os_printf("device has wifi connetced now\n\r");
	}

	if (audio_player_wifi_connect_status_get())
	{
		if (os_strcmp(argv[1], "start") == 0) {
			audio_player_start(song_list[current_song - 1]);
		} else if (os_strcmp(argv[1], "pause") == 0) {
			audio_player_pause();
		} else if (os_strcmp(argv[1], "play") == 0) {
			audio_player_play();
		} else if (os_strcmp(argv[1], "set_volume") == 0) {
			uint8_t value = os_strtoul(argv[2], NULL, 0); // The range of value is 0x0 ~ 0x3F.
			audio_player_set_volume(value);
		} else if (os_strcmp(argv[1], "last") == 0) {
			audio_player_pre();
		} else if (os_strcmp(argv[1], "next") == 0) {
			audio_player_next();
		} else if (os_strcmp(argv[1], "close") == 0) {
			ret = audio_player_deinit();
			if (ret != BK_OK)	{
				os_printf("audio player deinit fail\r\n");
				return;
			}
		}
	}
	else
	{
		os_printf("You need to connect device to available wifi first\n\r");
		os_printf("connect comond: music_network connect {ssid} {passwd}\n\r");
		return;
	}

}

static const struct cli_command s_music_commands[] =
{
	{"music_network", "music_network", cli_music_cmd},
};

int cli_music_init(void)
{
	return cli_register_commands(s_music_commands, CMDS_COUNT);
}


#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
	0x38000000,
	0x30020000,
	0x38020000,
	0x30000000
};
#endif

void music_init(void)
{
#ifdef CONFIG_LV_USE_DEMO_MUSIC
	bk_err_t ret;
	lv_vnd_config_t lv_vnd_config;

	cli_music_init();

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = (30 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open.device_ppi)*ppi_to_pixel_y(lcd_open.device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open.device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_init(&lv_vnd_config);
	ret = media_app_lcd_open((lcd_open_t *)&lcd_open);
	if (ret != BK_OK)
	{
		os_printf("media_app_lcd_open failed\r\n");
	}

	lv_vendor_start();
    lcd_driver_backlight_open();

	lv_vendor_disp_lock();
	lv_demo_music();
	lv_vendor_disp_unlock();

#endif
}

void user_app_main(void)
{
	bk_err_t ret = BK_OK;
	current_song = 1;

	song_number = sizeof(song_list) / sizeof(song_list[0]);

	os_printf("current_song: %d, song_number: %d\r\n", current_song, song_number);

	ret = netstream_player_init();
	if (ret != BK_OK)	{
		os_printf("netstream player init fail\r\n");
		return;
	}
}
#endif

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();

#if (CONFIG_SYS_CPU0)
    media_service_init();
	music_init();
#endif

	return 0;
}