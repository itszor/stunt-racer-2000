/* Stunt Racer 2000 (Fednet / The Fourth Dimension, 1993; version 1.01,
 * 1997): what libroger needs to know about the game.
 *
 * !Run -> !Run2 (checks the RAM disc) -> !Run3: CDA, CDA2Mg, Wcw, then
 * Intro.Intro (BASIC), which CHAINs Code.3d_data - the game, a BASIC
 * program (basic.c, translated by SuperSpider's tools/bas2c.py) that
 * CLOADs its machine code (gen/, the core image from &8F00) and CALLs it.
 * Both discs are merged into ../game/!SR2000. */
#include <stdio.h>
#include <string.h>
#include "bas.h"

/* the core image's translation (make regen). The image itself (the
 * blobs as the BASIC CLOADs them, ../unpacked/core) is loaded up front too:
 * the CLOADs then write over it with the same bytes, which libroger sees as
 * stores to compiled code that leave it as translated. (Nothing else uses
 * &8F00-&75C9C: the BASIC's PAGE and heap are above it.) */
extern const struct roger_image recomp_image;

#define MB(k) 0x1840000u + (k) * 0x10000u
#define IMG(k) {MB(k), MB(k) + 0x10000u, NULL, 0, nomod[k], 'a' + (k)}
#define MOD(name, k) {"../game/!SR2000/Code/" name, &modimg[k], 1}
static uint8_t nomod[17][0x10000 / 32];
static struct roger_image modimg[17] = {IMG(0), IMG(1), IMG(2), IMG(3), IMG(4), IMG(5), IMG(6), IMG(7), IMG(8),
                                        IMG(9), IMG(10), IMG(11), IMG(12), IMG(13), IMG(14), IMG(15), IMG(16)};
static struct roger_module modules[] = {
    MOD("DeComp,fffffa45-00000000", 0), MOD("ReadDisc,fffffa47-00000000", 1),
    MOD("RTFM,fffffa45-00000000", 2), MOD("ExSound,fffffa45-00000000", 3),
    MOD("Sounds/Mod2,fffffa45-00000000", 4), MOD("Sounds/Engine,fffffa45-00000000", 5),
    MOD("Sounds/ExplodeMD2,fffffa45-00000000", 6), MOD("Sounds/CheckPt,fffffa45-00000000", 7),
    MOD("Sounds/Lap,fffffa45-00000000", 8), MOD("Sounds/Start,fffffa45-00000000", 9),
    MOD("Sounds/Finished,fffffa45-00000000", 10), MOD("Sounds/BottomOut,fffffa45-00000000", 11),
    MOD("Sounds/SCrash,fffffa45-00000000", 12), MOD("Sounds/grass_skid,fffffa45-00000000", 13),
    MOD("Sounds/Transport,fffffa45-00000000", 14), MOD("Sounds/Zoom,fffffa45-00000000", 15),
    MOD("Sounds/Fuzz,fffffa45-00000000", 16),
    {NULL, NULL, 0},
};

static const char *const prefixes[] = {
    "adfs::StuntR1.$.", "adfs::StuntR2.$.", "<Obey$Dir>.",
    NULL,
};
static const char *const variables[] = {
    "SR2000$Dir", "!SR2000", "SR2000$Dir1", "adfs::StuntR1.$.!SR2000",
    "SR2000$Dir2", "adfs::StuntR2.$.!SR2000", "StuntQuiet", "FALSE", NULL,
};

/* the assembler: one patch, made on RISC OS 3.5 and later (OS_Byte 129
 * reports > 164): P% = Pp% ; [OPT 0: MOV PC, R14 :] */
void bas_asm(const char *text)
{
    extern int32_t P_I;
    if (strstr(text, "MOV PC , 14") || strstr(text, "MOV PC ,14") || strstr(text, "MOV PC,14"))
        PK(P_I) = (int32_t)0xE1A0F00E;
    else fprintf(stderr, "bas_asm: not handled: %s\n", text);
}

/* Read_Disc (the ReadDisc module): is the game installed on a hard disc?
 * Yes - both discs are in ../game. OS_GBPB 5 (the disc's name): the one
 * the BASIC is asking for, from L% as FNda works it out - StuntR1,
 * StuntR2, or a track series disc StuntR2-n (L% 12-19). */
static int game_swi(uint32_t n, int x)
{
    extern int32_t L_I;
    if (n == 0xB8500) { R[0] = (uint32_t)TRUE; return 1; }
    if (n == 0x0C && R[0] == 5) {
        char name[16];
        int l = L_I;
        if (l > 11 && l < 20) snprintf(name, sizeof name, "StuntR2-%d", l - 10);
        else snprintf(name, sizeof name, "StuntR%d", l == 11 ? 2 : l);
        int k = (int)strlen(name);
        MEM[R[2]] = k;
        memcpy(MEM + R[2] + 1, name, k);
        MEM[R[2] + 1 + k] = 0;
        return 1;
    }
    return 0;
}

static void init(void)
{
    oscli_c("RMLoad <SR2000$Dir>.Code.DeComp");     /* (Intro's PROCsetup) */
    bas_main();
    roger_finish("program ended");
}

static struct roger_config config = {
    .title = "Stunt Racer 2000 (recompiled)",
    .image_path = "../unpacked/core",
    .image = &recomp_image,
    .modules = modules,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = variables,
    .initial_mode = 13,
    .wimpslot = 2048 * 1024,
    .interpret_gaps = 1,
    .init = init,
    .swi = game_swi,
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
