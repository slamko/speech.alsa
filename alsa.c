#include "mlib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>
#include <complex.h>
#include <math.h>

#define CAPT_BUF_SIZE 0x400

#define BUF_SIZE (CAPT_BUF_SIZE * sizeof (int16_t))

int16_t buf[BUF_SIZE * 8];

volatile uint16_t sample;
volatile int stream_end;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define PI 3.14159

void dft(complex float *dft, float *signal, size_t slen) {

    for_range(k, 0, slen) {
        dft[k] = 0;

        for_range(n, 0, slen) {
            dft[k] += signal[n] * cexp(-1 * I * 2 * PI * k * n / slen);
        }
    }
}

void *pthread_audio_process(void *audio) {
    uint16_t cur_sample = 0;
    
    /* while (1) { */
    for_range(i, 0, 100) {
        int16_t *audio_buf = &buf[cur_sample];
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
        
        printf("\nReceived: \n");

        for_range(i, 0, CAPT_BUF_SIZE) {
            printf("%d  ", audio_buf[i]);
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

    /* while (1) { */
    for_range(i, 0, 100) {
        if (snd_pcm_avail(handle) >= CAPT_BUF_SIZE) {
            if (snd_pcm_readi(handle, buf + sample, CAPT_BUF_SIZE) != CAPT_BUF_SIZE) {
                err("Read error\n");
                ret_code(1);
            }

            sample += CAPT_BUF_SIZE;
            if (sample >= BUF_SIZE) {
                sample = 0;
            }
 
            pthread_cond_signal(&cond);
        }
    }

  cleanup:
    stream_end = 1;
    pthread_cond_signal(&cond);
    pthread_join(fft_thread, NULL);
    
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

    float inp_buf[33] = {0};
    for_range(i, 0, ARR_LEN(inp_buf)) {
        inp_buf[i] = (float)INT16_MAX * sin(((float)i / 32.0) * 2 * PI);
    }
    
    size_t dft_len = 33;
    complex float dft_buf[CAPT_BUF_SIZE] = {0};
    dft(dft_buf, inp_buf, dft_len);

    puts("\nINP: \n");
    for_range(i, 0, dft_len) {
        printf("%zu: %f \n", i, inp_buf[i]);
    }

    puts("\nDFT: \n");
    for_range(i, 0, dft_len) {
        printf("%zu: %f + j*%f \n", i, creal(dft_buf[i]) / (float)INT16_MAX, cimag(dft_buf[i]) / (float)INT16_MAX);
    }

  cleanup:
    if (handle) snd_pcm_close(handle);
    if (hw_params) snd_pcm_hw_params_free(hw_params);

    return ret;
}
