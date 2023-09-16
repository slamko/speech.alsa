#ifndef MLIB_H
#define MLIB_H

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

#define fwrite_str(file, str) fwrite(str, sizeof (char), sizeof(str) - 1, file)
#define fwrite_int(file, type, integ) { type val = integ; fwrite(&val, sizeof (type), 1, file); }




#endif
