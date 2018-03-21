#include <string.h>

#include "main.h"

int readSettings(int *sock_port, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    char *str = TSVgetvalues(r, 0, "port");
    if (str == NULL) {
        return 0;
    }
    *sock_port = atoi(str);
    TSVclear(r);
    return 1;
}

int initDevice(DeviceList *list, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    int n = TSVntuples(r);
    if (n <= 0) {
        TSVclear(r);
        return 1;
    }
    RESIZE_M_LIST(list, n)
    if (LML != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while resizing list\n", F);
#endif
        TSVclear(r);
        return 0;
    }
    for (int i = 0; i < LML; i++) {
        char *id = TSVgetvalues(r, i, "id");
        char *sclk = TSVgetvalues(r, i, "sclk");
        char *mosi = TSVgetvalues(r, i, "mosi");
        char *miso = TSVgetvalues(r, i, "miso");
        char *cs = TSVgetvalues(r, i, "cs");
        LIiei(id);
        LIiei(sclk);
        LIiei(mosi);
        LIiei(miso);
        LIiei(cs);
        LL++;
    }
    TSVclear(r);
    if (LL != LML) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while reading rows\n", F);
#endif
        return 0;
    }
    return 1;
}

int initChannel(ChannelList *list, DeviceList *dl, LCorrectionList *lcl, LReductionList *lrl, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    int n = TSVntuples(r);
    if (n <= 0) {
        TSVclear(r);
        return 1;
    }
    RESIZE_M_LIST(list, n)
    if (LML != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while resizing list\n", F);
#endif
        TSVclear(r);
        return 0;
    }
    for (int i = 0; i < LML; i++) {
        char *id = TSVgetvalues(r, i, "id");
        char *device_id_str = TSVgetvalues(r, i, "device_id");
        char *device_channel_id = TSVgetvalues(r, i, "device_channel_id");
        char *mode = TSVgetvalues(r, i, "mode");
        char *lcorrection_id_str = TSVgetvalues(r, i, "lcorrection_id");
        char *lreduction_id_str = TSVgetvalues(r, i, "lreduction_id");
        int device_id = atoi(device_id_str);
        int lcorrection_id = atoi(lcorrection_id_str);
        int lreduction_id = atoi(lreduction_id_str);
        LIi.id = atoi(id);
        LIi.device = getDeviceById(device_id, dl);
        LIi.device_channel_id = atoi(device_channel_id);
        LIi.lcorrection = getLCorrectionById(lcorrection_id, lcl);
        LIi.lreduction = getLReductionById(lreduction_id, lrl);
        LIi.mode = getModeByStr(mode);
        if (initMutex(&LIi.mutex)) {
            LL++;
        }
    }
    TSVclear(r);
    if (LL != LML) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while reading rows\n", F);
#endif
        return 0;
    }
    return 1;
}

static int countChannelMaExpLength(int *ma_length, int *exp_length, int channel_id, TSVresult* r_ma, TSVresult* r_exp, TSVresult* r_map, int n_map, int n_ma, int n_exp) {
    *ma_length = *exp_length = 0;
    for (int j = 0; j < n_map; j++) {
        int fchannel_id = TSVgetis(r_map, j, "channel_id");
        if (TSVnullreturned(r_map)) {
            return 0;
        }
        if (channel_id == fchannel_id) {
            (*ma_length)++;
            (*exp_length)++;
        }
    }
    return 1;
}

static int fillChannelFilter(FilterMAList *ma_list, FilterEXPList *exp_list, FilterList *f_list, int channel_id, TSVresult* r_ma, TSVresult* r_exp, TSVresult* r_map, int n_map, int n_ma, int n_exp) {
    for (int j = 0; j < n_map; j++) {
        int fchannel_id = TSVgetis(r_map, j, "channel_id");
        if (TSVnullreturned(r_map)) {
            return 0;
        }
        if (channel_id == fchannel_id) {
            int filter_id = TSVgetis(r_map, j, "filter_id");
            if (TSVnullreturned(r_map)) {
                return 0;
            }
            for (int k = 0; k < n_ma; k++) {
                int filter_p_id = TSVgetis(r_ma, k, "id");
                int filter_p_length = TSVgetis(r_ma, k, "length");
                if (TSVnullreturned(r_ma)) {
                    return 0;
                }
                if (filter_p_id == filter_id) {
                    if (ma_list->length >= ma_list->max_length) {
#ifdef MODE_DEBUG
                        fprintf(stderr, "%s(): ma_list overflow where channel_id=%d and filter_id=%d\n", F, channel_id, filter_id);
#endif
                        return 0;
                    }
                    if (!fma_init(&ma_list->item[ma_list->length], filter_p_id, filter_p_length)) {
                        return 0;
                    }
                    ma_list->length++;
                    if (f_list->length >= f_list->max_length) {
#ifdef MODE_DEBUG
                        fprintf(stderr, "%s(): f_list overflow where channel_id=%d and filter_id=%d\n", F, channel_id, filter_id);
#endif
                        return 0;
                    }
                    f_list->item[f_list->length].filter_ptr = &ma_list->item[ma_list->length-1];
                    f_list->item[f_list->length].filter_fun = fma_calc;
                    f_list->length++;
                }
            }
            for (int k = 0; k < n_exp; k++) {
                int filter_p_id = TSVgetis(r_exp, k, "id");
                float filter_p_a = TSVgetfs(r_exp, k, "a");
                if (TSVnullreturned(r_exp)) {
                    return 0;
                }
                if (filter_p_id == filter_id) {
                    if (exp_list->length >= exp_list->max_length) {
#ifdef MODE_DEBUG
                        fprintf(stderr, "%s(): exp_list overflow where channel_id=%d and filter_id=%d\n", F, channel_id, filter_id);
#endif
                        return 0;
                    }
                    if (!fexp_init(&exp_list->item[exp_list->length], filter_p_id, filter_p_a)) {
                        return 0;
                    }
                    exp_list->length++;
                    if (f_list->length >= f_list->max_length) {
#ifdef MODE_DEBUG
                        fprintf(stderr, "%s(): f_list overflow where channel_id=%d and filter_id=%d\n", F, channel_id, filter_id);
#endif
                        return 0;
                    }
                    f_list->item[f_list->length].filter_ptr = &exp_list->item[exp_list->length-1];
                    f_list->item[f_list->length].filter_fun = fexp_calc;
                    f_list->length++;
                }
            }
        }
    }
    return 1;
}

int initChannelFilter(ChannelList *list, const char *ma_path, const char *exp_path, const char *mapping_path) {
#define CLEAR_TSV_LIB  TSVclear(r_map);TSVclear(r_exp);TSVclear(r_ma);
#define RETURN_FAILURE  CLEAR_TSV_LIB return 0;
    TSVresult tsv1 = TSVRESULT_INITIALIZER;
    TSVresult* r_ma = &tsv1;
    if (!TSVinit(r_ma, ma_path)) {
        TSVclear(r_ma);
        return 0;
    }
    TSVresult tsv2 = TSVRESULT_INITIALIZER;
    TSVresult* r_exp = &tsv2;
    if (!TSVinit(r_exp, exp_path)) {
        TSVclear(r_exp);
        TSVclear(r_ma);
        return 0;
    }
    TSVresult tsv3 = TSVRESULT_INITIALIZER;
    TSVresult* r_map = &tsv3;
    if (!TSVinit(r_map, mapping_path)) {
        RETURN_FAILURE;
    }
    int n_map = TSVntuples(r_map);
    int n_ma = TSVntuples(r_ma);
    int n_exp = TSVntuples(r_exp);
    FORL{
        int ma_length = 0;
        int exp_length = 0;
        if (!countChannelMaExpLength(&ma_length, &exp_length, LIi.id, r_ma, r_exp, r_map, n_map, n_ma, n_exp)) {
            RETURN_FAILURE;
        }
        RESET_LIST(&LIi.fma_list);
        RESIZE_M_LIST(&LIi.fma_list, ma_length);
        if (LIi.fma_list.max_length != ma_length) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failure while resizing fma_list where channel_id=%d\n", F, LIi.id);
#endif
            RETURN_FAILURE;
        }
        RESET_LIST(&LIi.fexp_list);
        RESIZE_M_LIST(&LIi.fexp_list, exp_length);
        if (LIi.fexp_list.max_length != exp_length) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failure while resizing fexp_list where channel_id=%d\n", F, LIi.id);
#endif
            RETURN_FAILURE;
        }
        int f_length = exp_length + ma_length;
        RESET_LIST(&LIi.f_list);
        RESIZE_M_LIST(&LIi.f_list, f_length);
        if (LIi.f_list.max_length != f_length) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failure while resizing f_list where channel_id=%d\n", F, LIi.id);
#endif
            RETURN_FAILURE;
        }
        if (!fillChannelFilter(&LIi.fma_list, &LIi.fexp_list, &LIi.f_list, LIi.id, r_ma, r_exp, r_map, n_map, n_ma, n_exp)) {
            RETURN_FAILURE;
        }
    }
    CLEAR_TSV_LIB;
    return 1;
}

static int checkThreadDevice(TSVresult* r) {
    int n = TSVntuples(r);
    int valid = 1;
    //unique thread_id and device_id
    for (int k = 0; k < n; k++) {
        char *thread_id_k_str = TSVgetvalues(r, k, "thread_id");
        char *device_id_k_str = TSVgetvalues(r, k, "device_id");
        int thread_id_k = atoi(thread_id_k_str);
        int device_id_k = atoi(device_id_k_str);
        for (int g = k + 1; g < n; g++) {
            char *thread_id_g_str = TSVgetvalues(r, g, "thread_id");
            char *device_id_g_str = TSVgetvalues(r, g, "device_id");
            int thread_id_g = atoi(thread_id_g_str);
            int device_id_g = atoi(device_id_g_str);
            if (thread_id_k == thread_id_g && device_id_k == device_id_g) {
                fprintf(stderr, "%s(): check thread_device configuration file: thread_id and device_id shall be unique (row %d and row %d)\n", F, k, g);
                valid = 0;
            }
        }

    }
    //unique device_id
    for (int k = 0; k < n; k++) {
        char *device_id_k_str = TSVgetvalues(r, k, "device_id");
        int device_id_k = atoi(device_id_k_str);
        for (int g = k + 1; g < n; g++) {
            char *device_id_g_str = TSVgetvalues(r, g, "device_id");
            int device_id_g = atoi(device_id_g_str);
            if (device_id_k == device_id_g) {
                fprintf(stderr, "%s(): check thread_device configuration file: device_id shall be unique (row %d and row %d)\n", F, k, g);
                valid = 0;
                break;
            }
        }

    }
    return valid;
}

static int countThreadChannel(int thread_id_in, ChannelList *cl, TSVresult* r) {
    int c = 0;
    int n = TSVntuples(r);
    for (int k = 0; k < n; k++) {
        char *thread_id_str = TSVgetvalues(r, k, "thread_id");
        char *device_id_str = TSVgetvalues(r, k, "device_id");
        int thread_id = atoi(thread_id_str);
        int device_id = atoi(device_id_str);
        if (thread_id == thread_id_in) {

            FORLISTP(cl, j) {
                if (cl->item[j].device->id == device_id) {
                    c++;
                }
            }
        }
    }
    return c;
}

int initThread(ThreadList *list, ChannelList *cl, DeviceList *dl, const char *thread_path, const char *thread_device_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, thread_path)) {
        TSVclear(r);
        return 0;
    }
    int n = TSVntuples(r);
    if (n <= 0) {
        TSVclear(r);
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): no data rows in file\n", F);
#endif
        return 0;
    }
    RESIZE_M_LIST(list, n)
    if (LML != n) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while resizing list\n", F);
#endif
        TSVclear(r);
        return 0;
    }
    for (int i = 0; i < LML; i++) {
        char *id = TSVgetvalues(r, i, "id");
        char *cd_sec = TSVgetvalues(r, i, "cd_sec");
        char *cd_nsec = TSVgetvalues(r, i, "cd_nsec");
        LIi.id = atoi(id);
        LIi.cycle_duration.tv_sec = atoi(cd_sec);
        LIi.cycle_duration.tv_nsec = atoi(cd_nsec);
        RESET_LIST(&LIi.channel_plist)
        LL++;
    }
    TSVclear(r);
    if (LL != LML) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): failure while reading rows\n", F);
#endif
        return 0;
    }
    if (!TSVinit(r, thread_device_path)) {
        TSVclear(r);
        return 0;
    }
    n = TSVntuples(r);
    if (n <= 0) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): no data rows in thread device file\n", F);
#endif
        TSVclear(r);
        return 0;
    }
    if (!checkThreadDevice(r)) {
        TSVclear(r);
        return 0;
    }

    FORLi{
        int thread_channel_count = countThreadChannel(LIi.id, cl, r);
        //allocating memory for thread channels
        RESET_LIST(&LIi.channel_plist)
        if (thread_channel_count <= 0) {
            continue;
        }
        RESIZE_M_LIST(&LIi.channel_plist, thread_channel_count)
        if (LIi.channel_plist.max_length != thread_channel_count) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failure while resizing channel_plist list\n", F);
#endif
            TSVclear(r);
            return 0;
        }
        //assigning channels to this thread
        for (int k = 0; k < n; k++) {
            char *thread_id_str = TSVgetvalues(r, k, "thread_id");
            char *device_id_str = TSVgetvalues(r, k, "device_id");
            int thread_id = atoi(thread_id_str);
            int device_id = atoi(device_id_str);
            if (thread_id == LIi.id) {

                FORLISTP(cl, j) {
                    if (cl->item[j].device->id == device_id) {
                        LIi.channel_plist.item[LIi.channel_plist.length] = &cl->item[j];
                        LIi.channel_plist.length++;
                    }
                }
            }
        }
        if (LIi.channel_plist.max_length != LIi.channel_plist.length) {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): failure while assigning channels to threads: some not found\n", F);
#endif
            TSVclear(r);
            return 0;
        }
    }
    TSVclear(r);

    //starting threads
    FORLi{
        if (!createMThread(&LIi.thread, &threadFunction, &LIi)) {
            return 0;
        }
    }
    return 1;
}
