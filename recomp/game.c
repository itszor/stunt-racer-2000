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

#include "gen/modules.inc"      /* DeComp, ReadDisc, RTFM, ExSound, the sounds */

/* Code.SUtil, which the BASIC *LOADs into a DIMmed block and CALLs */
extern const struct roger_image recomp_sutil_image;
static const struct roger_program programs[] = {{"SUtil", &recomp_sutil_image, 1}, {NULL, NULL, 0}};

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
    .programs = programs,
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
