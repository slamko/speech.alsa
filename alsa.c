#include "mlib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>

#define CAPT_BUF_SIZE 0x400

#define BUF_SIZE (CAPT_BUF_SIZE * sizeof (int16_t))
int16_t buf[CAPT_BUF_SIZE * sizeof (int16_t)];
volatile uint16_t sample;
volatile int stream_end;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *pthread_audio_process(void *audio) {
    int16_t *audio_buf = audio;
    uint16_t cur_sample = 0;
    
    while (1) {
        pthread_mutex_lock(&mutex);
        
        while(cur_sample == sample % ARR_LEN(buf)) {
            if (stream_end) {
                pthread_exit(NULL);
            }
            
            pthread_cond_wait(&cond, &mutex);
        }
        
        if (cur_sample >= BUF_SIZE) {
            cur_sample = 0;
        }
        
        printf("Received: \n");

        for_range(i, 0, CAPT_BUF_SIZE) {
            printf("%d  ", audio_buf[i + cur_sample]);
        }
        cur_sample += BUF_SIZE;

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int audio_read(snd_pcm_t *handle) {
    pthread_t fft_thread = {0};
    int ret = {0};
    ret = pthread_create(&fft_thread, NULL, &pthread_audio_process, (void *)buf);
    /* pid_t fft_pid = fork(); */

    if (ret) {
        err("Audio thread spawn failed");
        return 1;
    }

    while (1) {
        if (snd_pcm_avail(handle) >= CAPT_BUF_SIZE) {
            if (snd_pcm_readi(handle, buf, CAPT_BUF_SIZE) != CAPT_BUF_SIZE) {
                err("Read error\n");
                return 1;
            }

            sample += CAPT_BUF_SIZE;
            if (sample >= BUF_SIZE) {
                sample = 0;
            }
 
            pthread_cond_signal(&cond);
        }

        printf("%d ", buf[0]);
    }

    stream_end = 1;
    pthread_cond_signal(&cond);
    pthread_join(fft_thread, NULL);
    
  cleanup:
    return ret;
}

int main(int argc, char **argv) {
    int ret = 0;
    snd_pcm_t *handle = {0};
    snd_pcm_hw_params_t *hw_params = {0};
    
    ret = snd_pcm_open(&handle, argv[1], SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    alsa_catch("Device initialization failed", ret);

    ret = snd_pcm_hw_params_malloc(&hw_params);
    alsa_catch("Hw params alloc failed", ret);

    ret = snd_pcm_hw_params_any(handle, hw_params);
    alsa_catch("HW params init failed", ret);

    ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    alsa_catch("Failed to set access mode", ret);

    ret = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
    alsa_catch("Failed to set capture format", ret);

    unsigned int rate = 48000;
    ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, NULL);
    alsa_catch("Failed to set capture rate", ret);

    printf("Capture rate: %u\n", rate);

    ret = snd_pcm_hw_params_set_channels(handle, hw_params, 2);
    alsa_catch("Failed to set capture channel number", ret);

    ret = snd_pcm_hw_params(handle, hw_params);
    alsa_catch("Failed to write HW params", ret);

    ret = snd_pcm_prepare(handle);
    alsa_catch("Device prepate failed", ret);

    ret = snd_pcm_start(handle);
    alsa_catch("Failed to start the device", ret);
    sleep(1);

    ret = audio_read(handle);
    catch("", ret);

    printf("Hello world\n");

  cleanup:
    if (handle) snd_pcm_close(handle);
    if (hw_params) snd_pcm_hw_params_free(hw_params);

    return ret;
}
