
#include "../main.h"

int mcp3008phys_init(Device *item) {
    pinModeOut(item->sclk);
    pinModeOut(item->cs);
    pinModeIn(item->miso);
    pinModeOut(item->mosi);
    return 1;
}
//10khz clock frequency (page 22)

void mcp3008phys_a2dread(Channel *item) {
#ifdef CPU_ANY
    item->value = 0.0f;
    item->tm = getCurrentTime();
    item->value_state = 1;
    lreduct(&item->value, item->lreduction);
    lcorrect(&item->value, item->lcorrection);
    return;
#endif
    int id = item->device_channel_id;
    int mode = item->mode;
    int cs = item->device->cs;
    int sclk = item->device->sclk;
    int mosi = item->device->mosi;
    int miso = item->device->miso;

    pinHigh(cs);
    pinLow(sclk);
    pinLow(cs);

    /*
        int cmd = id;
        cmd |= 0x18;
        cmd <<= 3;
     */
    int cmd = id;
    cmd |= mode;
    cmd <<= 3;
    for (int i = 0; i < 5; i++) {
        if (cmd & 0x80) {
            pinHigh(mosi);
        } else {
            pinLow(mosi);
        }
        cmd <<= 1;
        pinHigh(sclk);
        pinLow(sclk);
    }

    int out = 0;
    for (int i = 0; i < 12; i++) {
        pinHigh(sclk);
        pinLow(sclk);
        out <<= 1;
        if (pinRead(miso)) {
            out |= 0x1;
        }
    }
    pinHigh(cs);
    out >>= 1;
    item->value = (float) out;
    item->value_state = 1;
    for (int i = 0; i < item->f_list.length; i++) {
        item->f_list.item[i].filter_fun(&item->value, item->f_list.item[i].filter_ptr);
    }
    lreduct(&item->value, item->lreduction);
    lcorrect(&item->value, item->lcorrection);
}


