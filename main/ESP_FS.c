#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_spiffs.h"
#include "esp_log.h"

#include "esp_h264_enc.h"
#include "esp_heap_caps.h"

static const char* TAG = "FileSystem";

void app_main(void)
{
    // SPIFFS config
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t result = esp_vfs_spiffs_register(&config);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize (%s)", esp_err_to_name(result));
    }

    if (esp_spiffs_mounted(config.partition_label)) {
        printf("SPIFFS is mounted!\n");
    } else {
        printf("SPIFFS is not mounted!\n");
    }

    size_t total = 0, used = 0;
    result = esp_spiffs_info(config.partition_label, &total, &used);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Total: %d, used: %d", total, used);
    }

    // H264 config
    int one_image_size = 0;
    esp_h264_err_t ret = ESP_H264_ERR_OK;
    esp_h264_enc_t handle = NULL;
    esp_h264_enc_cfg_t cfg = DEFAULT_H264_ENCODER_CONFIG();
    esp_h264_enc_frame_t out_frame = { 0 };
    esp_h264_raw_frame_t in_frame = { 0 };
    int frame_count = 0;
    int ret_w = 0;


    FILE *in = fopen("/spiffs/input.yuv", "rb");
    if (in == NULL) {
        printf("Input file cann't open \r\n");
    }
    
    FILE *out = fopen("/spiffs/compressed.h264", "wb");
    if (out == NULL) {
        printf("Output file cann't open \r\n");
        return;
    }

    cfg.fps = 4;
    cfg.width = 640;
    cfg.height = 480;
    cfg.gop_size = 4;
    cfg.target_bitrate = 100000;
    cfg.pic_type = ESP_H264_RAW_FMT_I420;
    one_image_size = cfg.height * cfg.width * 1.5; // 1.5 : Pixel is 1.5 on ESP_H264_RAW_FMT_I420.
    in_frame.raw_data.buffer = heap_caps_aligned_alloc(16, one_image_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (in_frame.raw_data.buffer == NULL) {
        printf("Allcation memory failed \r\n");
    }

    ret = esp_h264_enc_open(&cfg, &handle);
    if (ret != ESP_H264_ERR_OK) {
        printf("Open failed. ret %d, handle %p \r\n", ret, handle);
    }

 
            clock_t start_timeMain = clock();
    while(1) 
    {
        ret_w = fread(in_frame.raw_data.buffer, 1, one_image_size, in);
        if (ret_w != one_image_size) {
            printf("Encoder finished, ret %d \r\n", ret_w);
            goto h264_example_exit;
        }
        in_frame.raw_data.len = ret_w;
        in_frame.pts = frame_count * (1000 / cfg.fps);
        ESP_LOGE(TAG, "START");
        clock_t start_time = clock();
        ret = esp_h264_enc_process(handle, &in_frame, &out_frame);
        clock_t end_time = clock();
        double duration = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("TIMER: %.6f секунд\n", duration);
        ESP_LOGE(TAG, "END");
        if (ret != ESP_H264_ERR_OK) {
            printf("Process failed. ret %d \r\n", ret);
            goto h264_example_exit;
        }
        for (size_t layer = 0; layer < out_frame.layer_num; layer++) {
            ret_w = fwrite(out_frame.layer_data[layer].buffer, 1, out_frame.layer_data[layer].len, out);
            if (ret_w != out_frame.layer_data[layer].len) {
                printf("fwrite happened error, ret %d \r\n", ret_w);
                goto h264_example_exit;
            }
        }
        frame_count++;
    }

    h264_example_exit:
    clock_t end_timeMain = clock();
    double durationMain = ((double)(end_timeMain - start_timeMain)) / CLOCKS_PER_SEC;
    printf("TIMER: %.6f секунд\n", durationMain);

    if (in) {
        fclose(in);
    }
    if (out) {
        fclose(out);
    }
    if (in_frame.raw_data.buffer) {
        free(in_frame.raw_data.buffer);
        in_frame.raw_data.buffer = NULL;
    }
    esp_h264_enc_close(handle);

    printf("FINISH \n");
    return;
}
