#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
//#include <psp2/power.h>
#include <psp2/sysmodule.h>
#include <taihen.h>
#include <string.h>
#include <math.h>

tai_hook_ref_t display_hook;
static SceUID hook;

static uint32_t hashmap[1024];
uint8_t old_r = 0;
uint8_t old_g = 0;
uint8_t old_b = 0;

void computeColor(const SceDisplayFrameBuf *pParam)
{
    uint32_t* data = (uint32_t*)(pParam->base);

    uint32_t rr = 0;
    uint32_t rg = 0;
    uint32_t rb = 0;
    uint32_t rcount = 0;

    memset(&hashmap, 0, sizeof(hashmap));

    for (int y = 0; y < pParam->height; y += 4)
    {
        for (int x = 0; x < pParam->width; x += 10)
        {
            uint32_t pixel = data[y*pParam->pitch + x];

            uint8_t r = pixel & 0x000000FF;
            uint8_t g = (pixel & 0x0000FF00) >> 8;
            uint8_t b = (pixel & 0x00FF0000) >> 16;

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
            old_r = nr;
            old_g = ng;
            old_b = nb;
            sceCtrlSetLightBar(0, nr, ng, nb);
            sceCtrlSetLightBar(1, nr, ng, nb);
            sceCtrlSetLightBar(2, nr, ng, nb);
            sceCtrlSetLightBar(3, nr, ng, nb);
            sceCtrlSetLightBar(4, nr, ng, nb);
        }
    }
}


int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, SceDisplaySetBufSync sync)
{
    computeColor(pParam);
    return TAI_CONTINUE(int, display_hook, pParam, sync);
}


void _start() __attribute__ ((weak, alias ("module_start")));
int module_start() {
    tai_module_info_t info = {0};
    info.size = sizeof(info);
    taiGetModuleInfo(TAI_MAIN_MODULE, &info);

    hook = taiHookFunctionImport(&display_hook, TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x7A410B64, sceDisplaySetFrameBuf_patched);
    sceClibPrintf("Hook: 0x%08X\n", hook);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop() {
    taiHookRelease(hook, display_hook);
    return SCE_KERNEL_STOP_SUCCESS;
}
