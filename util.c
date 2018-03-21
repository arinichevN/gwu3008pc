
#include "main.h"

FUN_LIST_GET_BY_ID(Channel)
FUN_LIST_GET_BY_ID(Device)
FUN_LIST_GET_BY_ID(Thread)

int checkDevice(DeviceList *list) {
    int valid = 1;
    FORLi{
        if (!checkPin(LIi.sclk)) {
            fprintf(stderr, "%s(): check device configuration file: bad sclk where id=%d\n", F, LIi.id);
            valid = 0;
        }
        if (!checkPin(LIi.mosi)) {
            fprintf(stderr, "%s(): check device configuration file: bad mosi where id=%d\n", F, LIi.id);
            valid = 0;
        }
        if (!checkPin(LIi.miso)) {
            fprintf(stderr, "%s(): check device configuration file: bad miso where id=%d\n", F, LIi.id);
            valid = 0;
        }
        if (!checkPin(LIi.cs)) {
            fprintf(stderr, "%s(): check device configuration file: bad cs where id=%d\n", F, LIi.id);
            valid = 0;
        }
    }
    //unique id
    FORLi{
        FORLLj
        {
            if (LIi.id == LIj.id) {
                fprintf(stderr, "%s(): check device table: id should be unique, repetition found where id=%d\n", F, LIi.id);
                valid = 0;
            }
        }
    }
    return valid;
}

int checkChannel(ChannelList *list) {
    int valid = 1;
    FORLi{
        if (LIi.mode == MODE_NONE) {
            fprintf(stderr, "%s(): check channel configuration file: bad mode where channel id=%d\n", F, LIi.id);
            valid = 0;
        }
        if (LIi.device == NULL) {
            fprintf(stderr, "%s(): check channel configuration file: bad device_id (device not found) where channel id=%d\n", F, LIi.id);
            valid = 0;
        }
        if (LIi.device_channel_id >= CHANNEL_COUNT || LIi.device_channel_id < 0) {
            fprintf(stderr, "%s(): check channel configuration file: bad device_channel_id where channel id=%d\n", F, LIi.id);
            valid = 0;
        }
    }
    //unique id
    FORLi{
        for (size_t j = i + 1; j < list->length; j++) {
            if (LIi.id == LIj.id) {
                fprintf(stderr, "%s(): check channel configuration file: id should be unique, repetition found where id=%d\n", F, LIi.id);
                valid = 0;
            }
        }
    }
    //unique device_channel_id within device
    FORLi{
        if (LIi.device != NULL) {
            FORLLj{
                if (LIj.device != NULL) {
                    if (LIi.device->id == LIj.device->id && LIi.device_channel_id == LIj.device_channel_id) {
                        fprintf(stderr, "%s(): check channel configuration file: device_channel_id should be unique within device, repetition found where id=%d and id=%d\n", F, LIi.id, LIj.id);
                        valid = 0;
                    }
                }
            }
        }
    }
    return valid;
}

void freeChannelList(ChannelList *list) {
    FORLi{
        freeMutex(&LIi.mutex);
        fma_freeList(&LIi.fma_list);
        fexp_freeList(&LIi.fexp_list);
        FREE_LIST(&LIi.f_list);
    }
    FREE_LIST(list);
}

void freeThreadList(ThreadList *list) {
    FORLi{
        FREE_LIST(&LIi.channel_plist);
    }
    FREE_LIST(list);
}

void stopAllThreads(ThreadList * list) {
    FORLi{
#ifdef MODE_DEBUG
        printf("signaling thread %d to cancel...\n", LIi.id);
#endif
        if (pthread_cancel(LIi.thread) != 0) {
#ifdef MODE_DEBUG
            perror("pthread_cancel()");
#endif
        }
    }

    FORLi{
        void * result;
#ifdef MODE_DEBUG
        printf("joining thread %d...\n", LIi.id);
#endif
        if (pthread_join(LIi.thread, &result) != 0) {
#ifdef MODE_DEBUG
            perror("pthread_join()");
#endif
        }
        if (result != PTHREAD_CANCELED) {
#ifdef MODE_DEBUG
            printf("thread %d not canceled\n", LIi.id);
#endif
        }
    }
}

int getModeByStr(const char *s) {
    if (strcmp(s, MODE_SINGLE_ENDED_STR) == 0) {
        return MODE_SINGLE_ENDED;
    } else if (strcmp(s, MODE_DIFFERENTIAL_STR) == 0) {
        return MODE_DIFFERENTIAL;
    }
    return MODE_NONE;
}

char * modeToStr(int v) {
    switch (v) {
        case MODE_SINGLE_ENDED:
            return MODE_SINGLE_ENDED_STR;
        case MODE_DIFFERENTIAL:
            return MODE_DIFFERENTIAL_STR;
    }
    return MODE_NONE_STR;
}

int catFTS(Channel *item, ACPResponse *response) {
    return acp_responseFTSCat(item->id, item->value, item->tm, item->value_state, response);
}

void printData(ACPResponse *response) {
#define DLi device_list.item[i]
#define CLi channel_list.item[i]
#define TLi thread_list.item[i]
#define CPLj channel_plist.item[j]
#define FMALj fma_list.item[j]
#define FEXPLj fexp_list.item[j]
#define FLj f_list.item[j]
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONF_MAIN_FILE: %s\n", CONF_MAIN_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_DEVICE_FILE: %s\n", CONF_DEVICE_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_CHANNEL_FILE: %s\n", CONF_CHANNEL_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_THREAD_FILE: %s\n", CONF_THREAD_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_THREAD_DEVICE_FILE: %s\n", CONF_THREAD_DEVICE_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_LREDUCTION_FILE: %s\n", CONF_LREDUCTION_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_LCORRECTION_FILE: %s\n", CONF_LCORRECTION_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", getpid());
    SEND_STR(q)

    acp_sendLReductionListInfo(&lreduction_list, response, &peer_client);

    acp_sendLCorrectionListInfo(&lcorrection_list, response, &peer_client);

    SEND_STR("+-----------------------------------------------------------------------+\n")
    SEND_STR("|                              device                                   |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |     id    |    sclk   |    mosi   |   miso    |    cs     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    FORLISTN(device_list, i) {
        snprintf(q, sizeof q, "|%11p|%11d|%11d|%11d|%11d|%11d|\n",
                (void *) &DLi,
                DLi.id,
                DLi.sclk,
                DLi.mosi,
                DLi.miso,
                DLi.cs
                );
        SEND_STR(q)
    }

    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------+\n")
    SEND_STR("|                                channel initial                                    |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |     id    | dev_ch_id | device_ptr|   mode    | lcorr_ptr | lredc_ptr |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    FORLISTN(channel_list, i) {
        char *mode_str = modeToStr(CLi.mode);
        snprintf(q, sizeof q, "|%11p|%11d|%11d|%11p|%11s|%11p|%11p|\n",
                (void *) &CLi,
                CLi.id,
                CLi.device_channel_id,
                (void *) CLi.device,
                mode_str,
                (void *) CLi.lcorrection,
                (void *) CLi.lreduction
                );
        SEND_STR(q)
    }

    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------+\n")
    SEND_STR("|                   channel runtime                         |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|     id    |   value   |value_state|  tm_sec   |  tm_nsec  |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+\n")
    FORLISTN(channel_list, i) {
        snprintf(q, sizeof q, "|%11d|%11.3f|%11d|%11ld|%11ld|\n",
                CLi.id,
                CLi.value,
                CLi.value_state,
                CLi.tm.tv_sec,
                CLi.tm.tv_nsec
                );
        SEND_STR(q)
    }

    SEND_STR("+-----------------------------------------------+\n")
    SEND_STR("|               channel ma filter               |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|channel_id | filter_id |filter_ptr |filter_leng|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    FORLISTN(channel_list, i) {

        FORLISTN(CLi.fma_list, j) {
            snprintf(q, sizeof q, "|%11d|%11d|%11p|%11d|\n",
                    CLi.id,
                    CLi.FMALj.id,
                    (void *) &CLi.FMALj,
                    CLi.FMALj.length
                    );
            SEND_STR(q)
        }
    }

    SEND_STR("+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------+\n")
    SEND_STR("|               channel exp filter              |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|channel_id | filter_id |filter_ptr | filter_a  |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    FORLISTN(channel_list, i) {

        FORLISTN(CLi.fexp_list, j) {
            snprintf(q, sizeof q, "|%11d|%11d|%11p|%11.3f|\n",
                    CLi.id,
                    CLi.FEXPLj.id,
                    (void *) &CLi.FEXPLj,
                    CLi.FEXPLj.a
                    );
            SEND_STR(q)
        }
    }

    SEND_STR("+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------+\n")
    SEND_STR("|    channel filter     |\n")
    SEND_STR("+-----------+-----------+\n")
    SEND_STR("|channel_id |filter_ptr |\n")
    SEND_STR("+-----------+-----------+\n")
    FORLISTN(channel_list, i) {

        FORLISTN(CLi.f_list, j) {
            snprintf(q, sizeof q, "|%11d|%11p|\n",
                    CLi.id,
                    CLi.FLj.filter_ptr
                    );
            SEND_STR(q)
        }
    }
    SEND_STR("+-----------+-----------+\n")

    SEND_STR("+-----------------------------------+\n")
    SEND_STR("|              thread               |\n")
    SEND_STR("+-----------+-----------+-----------+\n")
    SEND_STR("|     id    |channel_ptr| ch_dev_ptr|\n")
    SEND_STR("+-----------+-----------+-----------+\n")
    FORLISTN(thread_list, i) {

        FORLISTN(TLi.channel_plist, j) {
            snprintf(q, sizeof q, "|%11d|%11p|%11p|\n",
                    TLi.id,
                    (void *) TLi.CPLj,
                    (void *) TLi.CPLj->device
                    );
            SEND_STR(q)
        }
    }
    SEND_STR_L("+-----------+-----------+-----------+\n")
#undef DLi 
#undef CLi 
#undef TLi
#undef CPLj
#undef FMALj
#undef FEXPLj
#undef FLj
}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget temperature in format: sensorId\\ttemperature\\ttimeSec\\ttimeNsec\\tvalid; program id expected\n", ACP_CMD_GET_FTS);
    SEND_STR_L(q)
}