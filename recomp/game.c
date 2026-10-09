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

/* ---- Intro.Intro: the Fednet splash, by hand (BASIC with inline
 * assembler). The promo picture is CLOADed into screen bank 1 while bank 2
 * shows TV static (fuzzy%, with the Fuzz voice hissing); the picture fades
 * in through the static, jittering, and is shown while the game loads.
 * (With 476K of RAM disc it also loads and starts the music: there's no RAM
 * disc here.) Its blocks are at &7F000, clear of the BASIC heap, so that
 * the game's DIMs land where its translation expects. */
#define INTRO_OFF  0x7F000u                      /* off%: OS_Word 22's block */
#define INTRO_VDU  0x7F010u                      /* bl%: OS_ReadVduVariables */
#define INTRO_PTR  0x7F020u                      /* b%: the pointer shape (Code.Ptr) */

static uint32_t r1 = 1023, r2 = 1023;            /* fuzzy%'s random numbers */

static uint32_t rd_word(uint32_t a)              /* an ARM2 LDR: unaligned rotates */
{
    uint32_t w = *(uint32_t *)(MEM + (a & ~3u)), k = (a & 3) * 8;
    return k ? (w >> k) | (w << (32 - k)) : w;
}

/* fuzzy%: static into the 40960-byte bank at bl, merged (merge != 0) with
 * the picture in the bank before it */
static void fuzzy(uint32_t bl, uint32_t merge)
{
    int32_t r4 = 160 * 255 + 156;
    uint32_t r9 = bl - 40960;
    for (;;) {
        uint32_t n = r2 & 0xFF, rot = n & 31;
        uint32_t ror = n == 0 || rot == 0 ? r2 : (r2 >> rot) | (r2 << (32 - rot));
        r1 ^= ror; r2 += r1;
        if (merge == 0) {                        /* .directstore% */
            WR32(bl + r4, r1); r4 -= 4; if (r4 < 0) return;
            WR32(bl + r4, r2); r4 -= 4; if (r4 < 0) return;
            continue;
        }
        for (int half = 0; half < 2; half++) {
            uint32_t r8 = r1;                    /* (r0 AND r1) OR r0 */
            if (merge & 4) r8 |= r2;
            if (half == 0 && (merge & 4) && (r1 & 15)) r9 += r2 >> 31;
            uint32_t r10 = rd_word(r9 + r4);
            if (merge & 8) r10 ^= 0xFFFFFFFFu;
            WR32(bl + r4, r10 | r8);
            r4 -= 4;
            if (r4 < 0) return;
        }
    }
}

static void bank(int op, int n) { R[0] = op; R[1] = n; roger_swi(0x06); }   /* OS_Byte 112/113 */

static void intro(void)
{
    mode(8); off();
    oscli_c("FX 200,1"); bank(112, 2); cls();
    int maxvol = inkey(-99) ? 24 : 0;            /* Space held: quieter */
    oscli_c(maxvol ? "SET StuntQuiet TRUE" : "SET StuntQuiet FALSE");
    /* PROCsetup */
    oscli_c("RMLOAD <SR2000$Dir>.Code.Decomp");
    colour_rgb(3, 0, 0, 0); colour(131); cls();
    bank(112, 1);
    WR32(INTRO_VDU, 148); WR32(INTRO_VDU + 4, 0xFFFFFFFFu);   /* ScreenStart: bank 1 */
    R[0] = R[1] = INTRO_VDU; roger_swi(0x31);
    uint32_t bl = RD32(INTRO_VDU);
    bank(113, 2);
    oscli_c("LOAD <SR2000$Dir>.CODE.Ptr 7F020");
    uint32_t sh = INTRO_PTR + 16;                /* b%!6 = b%+16 (bytes), b%?1 = 2 */
    for (int k = 0; k < 4; k++) MEM[INTRO_PTR + 6 + k] = (uint8_t)(sh >> 8 * k);
    MEM[INTRO_PTR + 1] = 2;
    R[0] = 21; R[1] = INTRO_PTR; roger_swi(0x07);
    bank(112, 2);
    char cmd[80];
    snprintf(cmd, sizeof cmd, "CLOAD <SR2000$Dir>.Intro.PromoPic &%X", bl);
    oscli_c(cmd);
    oscli_c("RMLoad <SR2000$Dir>.Code.Sounds.Fuzz");
    colour_rgb(3, 0, 0, 0); colour_rgb(1, 196, 196, 196); colour_rgb(2, 96, 96, 96); colour_rgb(0, 255, 255, 255);
    bl += 40960;                                 /* bank 2, where the static goes */
    oscli_c("VOLUME 1");
    voices(1); oscli_c("CHANNELVOICE 1 Fuzz");
    bank(113, 2); sound_on(1);
    snprintf(cmd, sizeof cmd, "VOLUME %d", 105 - maxvol); oscli_c(cmd);
    sound(1, -15, 0x100, -1);
    time_set(0);                                 /* static, the volume flickering */
    do {
        fuzzy(bl, 0); wait_();
        if (rnd(10) == 1) { snprintf(cmd, sizeof cmd, "VOLUME %d", 100 + rnd(9) - maxvol); oscli_c(cmd); }
    } while (time_get() < 150);
    MEM[INTRO_OFF] = 2;                          /* off%: display start, jittering */
    time_set(0);                                 /* the picture through the static */
    uint32_t merge = 0;
    do {
        fuzzy(bl, merge); wait_();
        if (rnd(5) == 1) { snprintf(cmd, sizeof cmd, "VOLUME %d", 90 + rnd(27) - maxvol); oscli_c(cmd);
                           merge = r1 ^ r2; }
        uint32_t off = rnd(5) == 1 ? 40960 + rnd(5120) : 40960;
        for (int k = 0; k < 4; k++) MEM[INTRO_OFF + 1 + k] = (uint8_t)(off >> 8 * k);
        R[0] = 22; R[1] = INTRO_OFF; roger_swi(0x07);
    } while (time_get() < 250);
    for (int k = 0; k < 4; k++) MEM[INTRO_OFF + 1 + k] = (uint8_t)(40960u >> 8 * k);
    R[0] = 22; R[1] = INTRO_OFF; roger_swi(0x07);
    fuzzy(bl, 8); wait_();
    bank(113, 1);                                /* the picture */
    oscli_c("RMKILL Fuzz"); vdu(6, -1);
}

static void init(void)
{
    intro();
    bas_main();                                  /* PAGE=508500: CHAIN "...code.3d_data" */
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
