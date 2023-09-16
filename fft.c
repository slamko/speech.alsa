#include <complex.h>
#include <math.h>
#include "mlib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PI 3.14159

#define CAPT_BUF_SIZE 0x400

void dft(complex float *dft, float *signal, size_t slen) {

    for_range(k, 0, slen) {
        dft[k] = 0;

        for_range(n, 0, slen) {
            dft[k] += signal[n] * cexp(-1 * I * 2 * PI * k * n / slen);
        }
    }
}

void _ditfft(float complex *fft, size_t stride, const float *signal, size_t slen) {
    if (!slen) {
        return;
    }
    
    if (slen == 1) {
        fft[0] = signal[0];
        return;
    }

    size_t new_stride = stride * 2;
    _ditfft(fft, new_stride, signal, slen / 2);
    _ditfft(fft + slen / 2, new_stride, signal + stride, slen / 2);

    for_range(k, 0, slen / 2) {
        float complex even = fft[k];
        float complex odd = fft[k + slen / 2];

        float complex q = cexp(-2*PI * I * k / slen) * odd;

        fft[k] = even + q;
        fft[k + slen/2] = even - q;
    }
}

void ditfft(float complex *fft, const float *signal, size_t slen) {
    return _ditfft(fft, 1, signal, slen);
}

void test() {
    float inp_buf[32] = {0};
    for_range(i, 0, ARR_LEN(inp_buf)) {
        inp_buf[i] = (float)INT16_MAX * (sin(((float)i / 4.0) * 2 * PI) + cos(((float)i / 8.0) * 2 * PI));
    }
    
    size_t dft_len = 32;
    float complex dft_buf[CAPT_BUF_SIZE] = {0};
    ditfft(dft_buf, inp_buf, dft_len);

    puts("\nINP: \n");
    for_range(i, 0, dft_len) {
        printf("%zu: %f \n", i, inp_buf[i]);
    }

    puts("\nDFT: \n");
    for_range(i, 0, dft_len) {
        printf("%zu: %f + j*%f \n", i, creal(dft_buf[i]) / (float)INT16_MAX, cimag(dft_buf[i]) / (float)INT16_MAX);
    }


}
