#ifndef _AUDIO_SPK_H_
#define _AUDIO_SPK_H_

/* HCC Embedded generated source */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const unsigned char *device_descriptor;
    const unsigned char *string_descriptor;
    int number_of_languages;
    int number_of_strings;
    const unsigned char ***strings;
    int number_of_configurations;
    const unsigned char **configurations_fsls;
    const unsigned char **configurations_hs;
} usbd_config_t;

extern const usbd_config_t device_audio_spk;

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_SPK_H_ */

