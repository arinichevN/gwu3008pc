
#ifndef GWU3008_H
#define GWU3008_H

#include "lib/app.h"
#include "lib/gpio.h"
#include "lib/timef.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/udp.h"
#include "lib/tsv.h"
#include "lib/lcorrection.h"
#include "lib/lreduction.h"
#include "lib/filter/ma.h"
#include "lib/filter/exp.h"

#define APP_NAME gwu3008pc
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./config/"
#endif
#define CONF_MAIN_FILE CONF_DIR "main.tsv"
#define CONF_DEVICE_FILE CONF_DIR "device.tsv"
#define CONF_CHANNEL_FILE CONF_DIR "channel.tsv"
#define CONF_THREAD_FILE CONF_DIR "thread.tsv"
#define CONF_THREAD_DEVICE_FILE CONF_DIR "thread_device.tsv"
#define CONF_LREDUCTION_FILE CONF_DIR "lreduction.tsv"
#define CONF_LCORRECTION_FILE CONF_DIR "lcorrection.tsv"
#define CONF_FILTER_MA_FILE CONF_DIR "filter_ma.tsv"
#define CONF_FILTER_EXP_FILE CONF_DIR "filter_exp.tsv"
#define CONF_CHANNEL_FILTER_FILE CONF_DIR "channel_filter.tsv"

#define CHANNEL_COUNT 8

#define MODE_SINGLE_ENDED_STR "se"
#define MODE_DIFFERENTIAL_STR "df"
#define MODE_NONE_STR "un"
#define MODE_SINGLE_ENDED 0x18
#define MODE_DIFFERENTIAL 0x10
#define MODE_NONE 0x00

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    WTIME, 
    FILTER_MA,
    FILTER_EXP
} StateAPP;

struct device_st {
    int id;
    int sclk;
    int mosi;
    int miso;
    int cs;
};
typedef struct device_st Device;

DEC_LIST(Device)

typedef struct{
    void *filter_ptr;
    void (*filter_fun)(float *, void *);
} Filter; 

DEC_LIST(Filter)

struct channel_st {
    int id;
    int device_channel_id;
    int mode; //single-ended or pseudo-differential
    struct device_st *device;
    float value;
    int value_state; //0 if reading value from device failed
    struct timespec tm; //measurement time
    LCorrection *lcorrection;
    LReduction *lreduction;
    FilterMAList fma_list;
    FilterEXPList fexp_list;
    FilterList f_list;
    Mutex mutex;
};
typedef struct channel_st Channel;

DEC_LIST(Channel)
DEC_PLIST(Channel)

struct thread_st {
    int id;
    ChannelPList channel_plist;
    pthread_t thread;
    struct timespec cycle_duration;
};
typedef struct thread_st Thread;
DEC_LIST(Thread)

extern int readSettings();

extern void serverRun(int *state, int init_state);

extern void *threadFunction(void *arg);

extern void initApp();

extern int initData();

extern void freeData();

extern void freeApp();

extern void exit_nicely();

extern void exit_nicely_e(char *s);

#endif

