#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

#define CAPT_BUF_SIZE 0x400

#define R(...) " " #__VA_ARGS__ " "

#define ARR_LEN(x) (sizeof(x) / sizeof(*(x)))
#define align(x, al) (size_t)((((x) / (al)) * (al)) + (((x) % (al)) ? (al) : 0))
#define align_down(x, al) (size_t) (((x) / (al)) * (al))

#define for_range(name, start, end) for (size_t name = start; name < end; name++)

#define ret_code(x)                                                            \
  {                                                                            \
    ret = x;                                                                   \
    goto cleanup;                                                              \
  }

#define ret_label(label, x)                                              \
  {                                                                            \
    ret = x;                                                                   \
    goto label;                                                              \
  }

#define err(str) fprintf(stderr, str);
#define error(str, ...) fprintf(stderr, str, __VA_ARGS__);

#define catch(err, ret)                                                  \
  ;                                                                            \
  if (ret) { \
      fprintf(stderr, "%s:%d: " err ": %d\n", __FILE_NAME__, __LINE__, ret); \
      ret_code(ret);                                                    \
  }

#define alsa_catch(err, ret) \
;            \
  if (ret < 0) { \
      fprintf(stderr, "%s:%d: " err ": Alsa error: %s\n", __FILE_NAME__, __LINE__, snd_strerror(ret)); \
      ret_code(ret);                                                    \
  }


int main(int argc, char **argv) {
    int ret = 0;
    snd_pcm_t *handle = {0};
    snd_pcm_hw_params_t *hw_params = {0};
    int16_t *buf = malloc(sizeof (*buf) * CAPT_BUF_SIZE * 2);
    
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

    for (size_t i = 0; i < 200; i++) {
        if (snd_pcm_readi(handle, buf, CAPT_BUF_SIZE) != CAPT_BUF_SIZE) {
            err("Read error\n");
            return 1;
        }

        for_range(j, 0, CAPT_BUF_SIZE) {
            printf("%d ", buf[j]);
        }

        putc('\n', stdout);
    }

    printf("Hello world\n");

  cleanup:
    if (handle) snd_pcm_close(handle);
    if (hw_params) snd_pcm_hw_params_free(hw_params);

    return ret;
}
