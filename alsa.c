#include "mlib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>
#include <complex.h>
#include <math.h>
#include <signal.h>

#define CAPT_BUF_SIZE 0x400

#define BUF_SIZE (CAPT_BUF_SIZE * sizeof (int16_t))

int16_t buf[BUF_SIZE * 8];

size_t written_size;
volatile uint16_t sample;
volatile int stream_end;

uint32_t srate = 16000;
uint16_t nchan = 1;
uint16_t bits_per_sample = 16;

FILE *wave_file;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int wav_set_file_size(void) {
    fwrite_int(wave_file, uint32_t, written_size + 8);
    printf("Wav: wrote bytes: %zu\n", written_size);

    fseek(wave_file, 4, SEEK_SET);

    fwrite_int(wave_file, uint32_t, written_size + 44);
    return 0;
}

void *pthread_audio_process(void *audio) {
    uint16_t cur_sample = 0;
    
    while (1) {
    /* for_range(i, 0, 100) { */
        pthread_mutex_lock(&mutex);
        char *audio_buf = (char *)&buf[cur_sample];

        /* if (audio_buf[0] < 10000) { */
        if (fwrite(audio_buf, 1, 2 * CAPT_BUF_SIZE, wave_file) != 2 * CAPT_BUF_SIZE) {
                err("Wave write error\n");
                return NULL;
            }
            
            written_size += CAPT_BUF_SIZE * 2;
        
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
    /* ret = pthread_create(&fft_thread, NULL, &pthread_audio_process, (void *)buf); */
    /* pid_t fft_pid = fork(); */

    int16_t lbuf[BUF_SIZE];


    if (ret) {
        err("Audio thread spawn failed");
        return 1;
    }

    while (1) {
    /* for_range(i, 0, 100) { */
        if (stream_end) {
            break;
        }

        /* if (snd_pcm_avail(handle) >= CAPT_BUF_SIZE) { */
            if (snd_pcm_readi(handle, lbuf, CAPT_BUF_SIZE) != CAPT_BUF_SIZE) {
                err("Read error\n");
                ret_code(1);
            }

            sample += CAPT_BUF_SIZE;
            if (sample >= BUF_SIZE) {
                sample = 0;
            }

            if (fwrite(lbuf, sizeof(*lbuf), CAPT_BUF_SIZE, wave_file) != CAPT_BUF_SIZE) {
                err("Wave write error\n");
                ret_code(1);
            }
            
            written_size += CAPT_BUF_SIZE * sizeof (*lbuf);

            /* pthread_cond_signal(&cond); */
        /* } */
    }

  cleanup:
    wav_set_file_size();
    fclose(wave_file);
    exit(0);

    stream_end = 1;
    pthread_cond_signal(&cond);
    pthread_join(fft_thread, NULL);
    
    return ret;
}

int write_wav_header(void) {
    fwrite_str(wave_file, "RIFF");
    fwrite_int(wave_file, uint32_t, 0);
    fwrite_str(wave_file, "WAVE");
    fwrite_str(wave_file, "fmt ");

    fwrite_int(wave_file, uint32_t, 16);
    fwrite_int(wave_file, uint16_t, 1);
    fwrite_int(wave_file, uint16_t, nchan);
    fwrite_int(wave_file, uint32_t, srate);
    fwrite_int(wave_file, uint32_t, srate * bits_per_sample * nchan / 8);
    fwrite_int(wave_file, uint16_t, bits_per_sample * nchan / 8);
    fwrite_int(wave_file, uint16_t, bits_per_sample);
    fwrite_str(wave_file, "data");

    return 0;
}

void save_wave(int i) {
    stream_end = 1;
}

int main(int argc, char **argv) {
    int ret = 0;
    snd_pcm_t *handle = {0};
    snd_pcm_hw_params_t *hw_params = {0};

    if (argc < 3) {
        err("Invalid args\n");
        return -1;
    }

    wave_file = fopen(argv[2], "w");

    if (!wave_file) {
        err("Can not open wave file\n");
        return -1;
    }

    write_wav_header();
    
    ret = snd_pcm_open(&handle, argv[1], SND_PCM_STREAM_CAPTURE, 0);
    alsa_catch("Device initialization failed", ret);

    ret = snd_pcm_hw_params_malloc(&hw_params);
    alsa_catch("Hw params alloc failed", ret);

    ret = snd_pcm_hw_params_any(handle, hw_params);
    alsa_catch("HW params init failed", ret);

    ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    alsa_catch("Failed to set access mode", ret);

    ret = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
    alsa_catch("Failed to set capture format", ret);

    unsigned int rate = srate;
    ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, NULL);
    alsa_catch("Failed to set capture rate", ret);

    printf("Capture rate: %u\n", rate);

    ret = snd_pcm_hw_params_set_channels(handle, hw_params, nchan);
    alsa_catch("Failed to set capture channel number", ret);

    ret = snd_pcm_hw_params(handle, hw_params);
    alsa_catch("Failed to write HW params", ret);

    ret = snd_pcm_prepare(handle);
    alsa_catch("Device prepate failed", ret);

    ret = snd_pcm_start(handle);
    alsa_catch("Failed to start the device", ret);
    sleep(1);
    struct sigaction save_act = {0};
    save_act.sa_handler = &save_wave;
    save_act.sa_flags = 0;

    sigaction(SIGINT, &save_act, NULL);
    sigaction(SIGKILL, &save_act, NULL);

    ret = audio_read(handle);
    catch("", ret);

    wav_set_file_size();

    printf("Hello world\n");

  cleanup:
    fclose(wave_file);
    if (handle) snd_pcm_close(handle);
    if (hw_params) snd_pcm_hw_params_free(hw_params);

    return ret;
}
