#include "application/usbd_config.h"

/* HCC Embedded generated source */

const unsigned char configuration_fsls_audio_spk_cfg[109]={

/* Configuration cfg */
    0x09,    /* length */
    0x02,    /* desctype */
    0x6d,0x00,    /* size */
    0x02,    /* nifc */
    0x01,    /* cfg_id */
    0x00,    /* cstr */
    0x80,    /* attrib */
    0x00,    /* power */

/* ClassDriverAudio clsaudio */

/* AudioControlInterface ctrlif */
    0x09,    /* length */
    0x04,    /* desctype */
    0x00,    /* ifc_id */
    0x00,    /* alt_set */
    0x00,    /* nep */
    0x01,    /* class */
    0x01,    /* sclass */
    0x00,    /* proto */
    0x00,    /* istr */

/* AudioControlHeader  */
    0x09,    /* length */
    0x24,    /* desctype */
    0x01,    /* subtype */
    0x00,0x01,    /* bcdACD */
    0x1e,0x00,    /* size */
    0x01,    /* blnCollection 'One streaming interface' */
    0x01,    /* first_ifc 'The index of the streaming interface' */

/* AudioInTerminal  */
    0x0c,    /* length */
    0x24,    /* desctype */
    0x02,    /* subtype */
    0x01,    /* bTerminalId */
    0x01,0x01,    /* wTerminalType 'USB streaming' */
    0x00,    /* bAssocTerminal 'None' */
    0x02,    /* bNrChannels 'Two channels (stereo)' */
    0x00,0x00,    /* wChannelCoding 'None' */
    0x00,    /* iChannelNames */
    0x00,    /* iTerminal */

/* AudioOutTerminal  */
    0x09,    /* length */
    0x24,    /* desctype */
    0x03,    /* subtype */
    0x02,    /* bTerminalId */
    0x01,0x03,    /* wTerminalType 'Speaker' */
    0x00,    /* bAssocTerminal 'None' */
    0x01,    /* bSourceId 'Sorce connected to unit 1' */
    0x00,    /* iTerminal */

/* AudioStreamingInterface offif */
    0x09,    /* length */
    0x04,    /* desctype */
    0x01,    /* ifc_id */
    0x00,    /* alt_set */
    0x00,    /* nep */
    0x01,    /* class */
    0x02,    /* sclass */
    0x00,    /* protocol */
    0x00,    /* istr */

/* AudioStreamingInterface onif */
    0x09,    /* length */
    0x04,    /* desctype */
    0x01,    /* ifc_id */
    0x01,    /* alt_set */
    0x02,    /* nep */
    0x01,    /* class */
    0x02,    /* sclass */
    0x00,    /* protocol */
    0x00,    /* istr */

/* AudioStreamingGeneralDescriptor  */
    0x07,    /* length */
    0x24,    /* desctype */
    0x01,    /* subtype */
    0x01,    /* bTerminalLink 'Id of Output terminal' */
    0x01,    /* bDelay 'Delay 1 mS' */
    0x01,0x00,    /* wFormatTag 'PCM format' */

/* AudioFormat1Descriptor  */
    0x0b,    /* length */
    0x24,    /* desctype */
    0x02,    /* subtype */
    0x01,    /* bFormatType */
    0x02,    /* bNrChannels 'Two chanels' */
    0x02,    /* bSubFrameSize '2 Bytes' */
    0x10,    /* bBitResolution '16 bits per sample' */
    0x01,    /* bSamFreqType 'One frequency supported' */
    0x80,0x3e,0x00,    /* tSamFreq '16 KHz' */

/* AudioEndPoint  */
    0x09,    /* length */
    0x05,    /* desctype */
    0x01,    /* addr */
    0x05,    /* attrib 'asynchron, ISO' */
    0x40,0x00,    /* psize '64 bytes / packet' */
    0x01,    /* interval '1 packet / frame' */
    0x00,    /* bRefresh 'Not used' */
    0x82,    /* bSynchAddress */

/* AudioStreamingEndPoint  */
    0x07,    /* bLength */
    0x25,    /* bDescritorType */
    0x01,    /* bDescritorSubtype */
    0x01,    /* bmAttributes 'Ep has sampling freq control' */
    0x00,    /* bLockDelayUinits */
    0x00,0x00,    /* wLockDelay */

/* AudioSynchronizationEndPoint syncep */
    0x09,    /* bLength */
    0x05,    /* bDescritorType */
    0x82,    /* addr */
    0x01,    /* attrib */
    0x03,0x00,    /* wMaxPacketSize */
    0x01,    /* bInterval */
    0x02,    /* bRefresh */
    0x00     /* bSynchAddress */
};

const unsigned char string_audio_spk_eng_manufact[26]={
    0x1a,0x03,0x48,0x00,0x43,0x00,0x43,0x00,    /* ..H.C.C. */
    0x2d,0x00,0x45,0x00,0x6d,0x00,0x62,0x00,    /* -.E.m.b. */
    0x65,0x00,0x64,0x00,0x64,0x00,0x65,0x00,    /* e.d.d.e. */
    0x64,0x00                                   /* d.       */
};

const unsigned char string_audio_spk_eng_product[16]={
    0x10,0x03,0x53,0x00,0x70,0x00,0x65,0x00,    /* ..S.p.e. */
    0x61,0x00,0x6b,0x00,0x65,0x00,0x72,0x00     /* a.k.e.r. */                                                /*          */
};

const unsigned char string_audio_spk_eng_serial[18]={
    0x12,0x03,0x56,0x00,0x65,0x00,0x72,0x00,    /* ..V.e.r. */
    0x20,0x00,0x31,0x00,0x2e,0x00,0x30,0x00,    /*  .1...0. */
    0x30,0x00                                   /* 0.       */
};

const unsigned char *string_descriptor_audio_spk_eng[3]={
    string_audio_spk_eng_manufact,
    string_audio_spk_eng_product,
    string_audio_spk_eng_serial
};

const unsigned char *configurations_fsls_audio_spk[1]={
    configuration_fsls_audio_spk_cfg
};

const unsigned char device_descriptor_audio_spk[18]={
    0x12,    /* length */
    0x01,    /* desctype */
    0x10,0x01,    /* usb_ver */
    0x00,    /* dclass */
    0x00,    /* dsubclass */
    0x00,    /* dprotocol */
    0x08,    /* psize */
    0xca,0xc1,    /* vid */
    0xc7,0xba,    /* pid */
    0x00,0x00,    /* relno */
    0x01,    /* mstr */
    0x02,    /* pstr */
    0x03,    /* sstr */
    0x01     /* ncfg */
};

const unsigned char string_descriptor_audio_spk[4]={
    0x04,0x03,0x09,0x04
};

const unsigned char **strings_audio_spk[1]={
    string_descriptor_audio_spk_eng
};

const usbd_config_t device_audio_spk={
    device_descriptor_audio_spk,
    string_descriptor_audio_spk,
    1,
    3,
    strings_audio_spk,
    1,
    configurations_fsls_audio_spk,
    (const unsigned char **)0
};

