/* Decompress the game's CLOAD files with its own DeComp module, run by
 * libroger's ARM2 interpreter.
 *
 *   cload --dir=DIR NAME=ADDR...
 *
 * Each NAME (a RISC OS name in !SR2000, e.g. Code.Poly_Code) is *CLOADed
 * at ADDR (hex), as the BASIC does,
 * and the memory from ADDR for the decompressed length (the file's first
 * word) is written to DIR/<leaf>. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <roger.h>

#define DECOMP "../extracted/disc1/!SR2000/Code/DeComp,fffffa45-00000000"
#define MODBASE 0x1800000u

static uint8_t nocode[0x80000 / 32], nomod[0x1000 / 32];
static struct roger_image image = {0x8000, 0x8000 + 0x80000, NULL, 0, nocode, 'P'};
static struct roger_image decomp = {MODBASE, MODBASE + 2428, NULL, 0, nomod, 'D'};
static const struct roger_module modules[] = {{DECOMP, &decomp, 0}, {NULL, NULL, 0}};

static const char *outdir = ".";
static char *files[32];
static int nfiles;

static int arg(const char *a)
{
    if (!strncmp(a, "--dir=", 6)) { outdir = a + 6; return 1; }
    if (strchr(a, '=') && a[0] != '-' && nfiles < 32) { files[nfiles++] = strdup(a); return 1; }
    return 0;
}

static void init(void)
{
    for (int k = 0; k < nfiles; k++) {
        char *eq = strrchr(files[k], '=');
        *eq = 0;
        uint32_t addr = strtoul(eq + 1, 0, 16);
        char host[2048];
        roger_hostpath(files[k], host);
        FILE *f = fopen(host, "rb");
        uint32_t len = 0;
        if (!f || fread(&len, 4, 1, f) != 1) { perror(files[k]); exit(1); }
        fclose(f);
        char cmd[512], hp[2048];
        snprintf(cmd, sizeof cmd, "CLOAD %s %x", files[k], addr);
        roger_cli(cmd);
        const char *leaf = strrchr(files[k], '.');
        leaf = leaf ? leaf + 1 : files[k];
        snprintf(hp, sizeof hp, "%s/%.*s", outdir, (int)strcspn(leaf, ","), leaf);
        FILE *o = fopen(hp, "wb");
        if (!o) { perror(hp); exit(1); }
        fwrite(MEM + addr, 1, len, o);
        fclose(o);
        printf("%s: &%x bytes at &%x -> %s\n", files[k], len, addr, hp);
    }
    roger_finish("done");
}

static struct roger_config config = {
    .title = "cload",
    .image_path = "/dev/null",           /* no program: init does the work */
    .image = &image,
    .modules = modules,
    .appdir = "../extracted/disc1/!SR2000",
    .initial_mode = 12,
    .wimpslot = 640 * 1024,
    .interpret_gaps = 1,
    .arg = arg,
    .init = init,
    .usage = "  --dir=DIR FILE=ADDR...   (hex addresses)\n",
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
