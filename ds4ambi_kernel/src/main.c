#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <psp2/kernel/clib.h>
#include <psp2kern/kernel/debug.h>
#include <psp2kern/kernel/sysmem.h>
#include <sys/types.h>

static SceUID g_thread_uid = -1;
static bool   g_thread_run = true;

static SceUID event_flag_id;

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

#define IMPORT(modname, lnid, fnid, name)\
    if (module_get_export_func(KERNEL_PID, #modname, lnid, fnid, (uintptr_t*)&_##name))\
        ksceDebugPrintf("ERROR: import "#modname">"#name"\n");

int (*_ksceCtrlSetLightBar)(int port, SceUInt8 r, SceUInt8 g, SceUInt8 b);
int ksceCtrlSetLightBar(int port, SceUInt8 r, SceUInt8 g, SceUInt8 b){
    return _ksceCtrlSetLightBar(port, r, g, b);
}

static int display_vblank_cb_func(int notifyId, int notifyCount, int notifyArg, void *common)
{
    ksceKernelSetEventFlag(event_flag_id, 1);
    return 0;
}

static int ambi_thread(SceSize args, void *argp) {

    ksceDisplayWaitSetFrameBufCB();

    SceUID display_vblank_cb_uid;
    display_vblank_cb_uid = ksceKernelCreateCallback("ambi_display_vblank", 0,
                             display_vblank_cb_func, NULL);

    ksceDisplayRegisterVblankStartCallback(display_vblank_cb_uid);

    ksceDebugPrintf("cb uid : 0x%08X\n", display_vblank_cb_uid);

    uint32_t hashmap[1024];
    uint8_t old_r = 0;
    uint8_t old_g = 0;
    uint8_t old_b = 0;

    while (g_thread_run) {
        unsigned int out_bits;

        int ret = ksceKernelWaitEventFlagCB(event_flag_id, 1,
            SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR_PAT,
            &out_bits, (SceUInt32[]){1000000});

        if (ret == 0)
        {
            SceDisplayFrameBufInfo fb_info;
            int head = ksceDisplayGetPrimaryHead();
            memset(&fb_info, 0, sizeof(fb_info));
            fb_info.size = sizeof(fb_info);
            ret = ksceDisplayGetProcFrameBufInternal(-1, head, 0, &fb_info);
            if (ret < 0 || fb_info.paddr == 0)
                ret = ksceDisplayGetProcFrameBufInternal(-1, head, 1, &fb_info);
            if (ret >= 0)
            {
                if (fb_info.framebuf.base && fb_info.framebuf.width > 0 && fb_info.framebuf.height > 0) {

                    uint32_t rr = 0;
                    uint32_t rg = 0;
                    uint32_t rb = 0;
                    uint32_t rcount = 0;
                    memset(&hashmap, 0, sizeof(hashmap));

                    for (int y = 0; y < fb_info.framebuf.height; y += 4)
                    {
                        uint8_t buf[960*4]; // TODO: malloc/free?

                        ksceKernelMemcpyUserToKernelForPid(fb_info.pid, (uint8_t*)buf, (uintptr_t)(fb_info.framebuf.base+y*fb_info.framebuf.pitch), 960*4);
//                        ksceDebugPrintf("PIXELS : (0)0x%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3]);

                        for (int x = 0; x < fb_info.framebuf.width; x += 10*4)
                        {
                            uint8_t r = buf[x];
                            uint8_t g = buf[x+1];
                            uint8_t b = buf[x+2];

                            uint8_t rx = r / 26;
                            uint8_t gx = g / 26;
                            uint8_t bx = b / 26;

                            uint32_t idx = rx * 100 + gx * 10 + bx;

                            hashmap[idx]++;

                            if (rcount < hashmap[idx])
                            {
                                rr += r;
                                rg += g;
                                rb += b;
                                rcount = hashmap[idx];
                            }
                        }
                    }


                    if (rcount > 0) {
                        uint8_t nr = rr / rcount;
                        uint8_t ng = rg / rcount;
                        uint8_t nb = rb / rcount;

                        if ( old_r != nr && old_b != nb && old_g != nb ) {
                            uint32_t inr = (nr + old_r) / 2;
                            uint32_t ing = (ng + old_g) / 2;
                            uint32_t inb = (nb + old_b) / 2;

                            old_r = nr;
                            old_g = ng;
                            old_b = nb;
                            ksceCtrlSetLightBar(0, (uint8_t)inr, (uint8_t)ing, (uint8_t)inb);
                            ksceCtrlSetLightBar(1, (uint8_t)inr, (uint8_t)ing, (uint8_t)inb);
                            ksceCtrlSetLightBar(2, (uint8_t)inr, (uint8_t)ing, (uint8_t)inb);
                            ksceCtrlSetLightBar(3, (uint8_t)inr, (uint8_t)ing, (uint8_t)inb);
                            ksceCtrlSetLightBar(4, (uint8_t)inr, (uint8_t)ing, (uint8_t)inb);
                        }
                    }




//                    ksceDebugPrintf("PIXELS : (0)0x%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3]);
                }
            }
        }
        ksceKernelDelayThread(30 * 1000);
    }

    ksceDisplayUnregisterVblankStartCallback(display_vblank_cb_uid);
    ksceKernelDeleteCallback(display_vblank_cb_uid);

    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    IMPORT(SceCtrl, 0xD197E3C7, 0xDA11D433, ksceCtrlSetLightBar);

    event_flag_id = ksceKernelCreateEventFlag("ambi_event_flag", 0, 0, NULL);

    g_thread_uid = ksceKernelCreateThread("ambi_thread", ambi_thread, 0x3C, 0x3000, 0, 0x10000, 0);
    ksceDebugPrintf("Thread : 0x%08X\n", g_thread_uid);

    ksceKernelStartThread(g_thread_uid, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    ksceKernelSetEventFlag(event_flag_id, 1);

    if (g_thread_uid >= 0) {
        g_thread_run = 0;
        ksceKernelWaitThreadEnd(g_thread_uid, NULL, NULL);
        ksceKernelDeleteThread(g_thread_uid);
    }
    ksceKernelDeleteEventFlag(event_flag_id);

    return SCE_KERNEL_STOP_SUCCESS;
}
