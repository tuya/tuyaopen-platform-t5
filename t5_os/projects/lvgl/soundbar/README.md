## Project: lvgl/soundbar

## Life Cycle: 2023-06-12 ~~ 2023-12-12

## Application: LVGL music UI demo and Bluetooth Dual Mode(such as A2DP) demo.

## Special Macro Configuration Description:
1、CONFIG_LV_USE_DEMO_MUSIC 	  // Config lvgl music demo
2、CONFIG_LV_COLOR_DEPTH          // Config lvgl color depth
3、CONFIG_LV_USE_PERF_MONITOR     // Config lvgl monitor FPS and CPU
4、CONFIG_LVGL                    // Config lvgl enable
5、CONFIG_LVGL_USE_PSRAM          // Config lvgl use psram
6、CONFIG_TP                      // Config lcd touch
7、CONFIG_A2DP_SINK_DEMO          // Config A2DP sink demo
8、CONFIG_AAC_DECODER             // Config AAC decoder

## Complie Command:
1、make bk7256 PROJECT=lvgl/soundbar

## CPU: 
1、BK7256: CPU0

## WIFI: STA/AP

## BLUETOOTH: BLE/BT

## Multi-Media: LVGL/LCD

