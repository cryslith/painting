#include <pam.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <getopt.h>

const int WIDTH = 128;
const int HEIGHT = 128;
const char *const OPTIONS = "w:h:c:si:t:";

typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;

int compute_colskip(int w, int h) {
    int c = (int) floor(256 / pow(w * h, 1.0 / 3.0));
    if (c < 1) {
        return 1;
    }
    return c;
}

bool nvalid(int r, int c, int w, int h, bool *touched, bool seektouch) {
    if (r < 0 || r >= h || c < 0 || c >= w) {
        return false;
    }
    return touched[r * w + c] == seektouch;
}

int neighbors(int pos, int w, int h, bool *touched, bool seektouch,
              int nls[9]) {
    int i = 0;
    int r = pos / w, c = pos % w;
    for (int ro = -1; ro <= 1; ro++) {
        for (int co = -1; co <= 1; co++) {
            if (!(ro || co)) {
                continue;
            }
            if (nvalid(r + ro, c + co, w, h, touched, seektouch)) {
                nls[i++] = (r + ro) * w + c + co;
            }
        }
    }
    return i;
}

float dist(color cola, color colb) {
    int rdist = colb.r - cola.r;
    int gdist = colb.g - cola.g;
    int bdist = colb.b - cola.b;
    return sqrt(rdist * rdist + gdist * gdist + bdist * bdist);
}

float score(int i, int w, int h, color col, color *img, bool *touched) {
    int nls[9];
    int nn = neighbors(i, w, h, touched, true, nls);
    float mindist = INFINITY;
    for (int n = 0; n < nn; n++) {
        float d = dist(col, img[nls[n]]);
        if (d < mindist) {
            mindist = d;
        }
    }
    return isfinite(mindist) ? -mindist : INFINITY;
}

bool setok_template(char *template_file, int w, int h, bool *ok) {
    FILE *thandle = pm_openr(template_file);
    if (thandle == NULL) {
        fprintf(stderr, "failed to open file %s\n", template_file);
        return false;
    }
    struct pam inpam;
    tuple *tuplerow;
    pm_init("", 0);
    pnm_readpaminit(thandle, &inpam, sizeof(struct pam));
    if (inpam.width != w) {
        fprintf(stderr, "width must be %d, not %d", w, inpam.width);
        pm_close(thandle);
        return false;
    }
    if (inpam.height != h) {
        fprintf(stderr, "height must be %d, not %d", h, inpam.height);
        pm_close(thandle);
        return false;
    }
    if (inpam.depth != 1) {
        fprintf(stderr, "depth must be 1, not %d", inpam.depth);
        pm_close(thandle);
        return false;
    }
    tuplerow = pnm_allocpamrow(&inpam);
    if (tuplerow == NULL) {
        fprintf(stderr, "failed to allocate pam row\n");
        pm_close(thandle);
        return false;
    }
    for (int row = 0; row < inpam.height; row++) {
        pnm_readpamrow(&inpam, tuplerow);
        for (int column = 0; column < inpam.width; column++) {
            ok[row * w + column] = (bool) tuplerow[column][0];
        }
    }
    pnm_freepamrow(tuplerow);
    pm_close(thandle);
    return true;
}

int main(int argc, char *argv[]) {
    int ret = 0;

    int w = WIDTH;
    int h = HEIGHT;
    int colskip;
    bool chose_colskip = false;
    bool shuffle = true;
    int numtemplates = 0;

    int oc;
    while ((oc = getopt(argc, argv, OPTIONS)) != -1) {
        switch(oc) {
        case 'w':
            if (sscanf(optarg, "%d", &w) != 1) {
                fprintf(stderr, "invalid width %s\n", optarg);
                return 255;
            }
            break;
        case 'h':
            if (sscanf(optarg, "%d", &h) != 1) {
                fprintf(stderr, "invalid height %s\n", optarg);
                return 255;
            }
            break;
        case 'c':
            if (sscanf(optarg, "%d", &colskip) != 1) {
                fprintf(stderr, "invalid color skip factor %s\n", optarg);
                return 255;
            }
            chose_colskip = true;
            break;
        case 's':
            shuffle = false;
            break;
        case 'i':
            // handle this later
            break;
        case 't':
            numtemplates++;
            // record template name later
            break;
        default:
            fprintf(stderr, "unknown option %c\n", oc);
            return 255;
        }
    }
    if (!chose_colskip) {
        colskip = compute_colskip(w, h);
    }

    color *img = malloc(sizeof(color) * w * h);
    if (img == NULL) {
        ret = 1;
        fprintf(stderr, "malloc img failed\n");
        goto err1;
    }
    bool *touched = malloc(sizeof(bool) * w * h);
    if (touched == NULL) {
        ret = 2;
        fprintf(stderr, "malloc touched failed\n");
        goto err2;
    }
    bool *border = malloc(sizeof(bool) * w * h);
    if (border == NULL) {
        ret = 3;
        fprintf(stderr, "malloc border failed\n");
        goto err3;
    }
    const int NUMCOLS = (int) pow(ceil(256.0 / colskip), 3);
    color *colors = malloc(NUMCOLS * sizeof(color));
    if (colors == NULL) {
        ret = 4;
        fprintf(stderr, "malloc colors failed\n");
        goto err4;
    }
    bool *ok = malloc(sizeof(bool) * w * h);
    if (ok == NULL) {
        ret = 5;
        fprintf(stderr, "malloc ok failed\n");
        goto err5;
    }
    char **template_files = malloc(sizeof(char *) * numtemplates);
    if (template_files == NULL) {
        ret = 6;
        fprintf(stderr, "malloc template_files failed\n");
        goto err6;
    }

    int seed = time(NULL);
    srand(seed);
    fprintf(stderr, "seed: %d\n", seed);

    int nextcol = 0;
    for (int r = 0; r < 256; r += colskip) {
        for (int g = 0; g < 256; g += colskip) {
            for (int b = 0; b < 256; b += colskip) {
                colors[nextcol++] = (color) {r, g, b};
            }
        }
    }
    nextcol = 0;

    if (shuffle) {
        int j;
        color swap;
        for (int c = 0; c < NUMCOLS; c++) {
            j = rand() % (NUMCOLS - c) + c;
            swap = colors[c];
            colors[c] = colors[j];
            colors[j] = swap;
        }
    }

    for (int i = 0; i < w * h; i++) {
        touched[i] = false;
        border[i] = false;
    }

    optind = 1;
    bool any_init = false;
    int level = 0;
    while ((oc = getopt(argc, argv, OPTIONS)) != -1) {
        if (oc == 'i') {
            int r, c;
            if (sscanf(optarg, "%d,%d", &r, &c) != 2) {
                fprintf(stderr, "invalid initial coordinates %s\n", optarg);
                return 255;
            }
            border[r * w + c] = true;
            any_init = true;
        }
        else if (oc == 't') {
            template_files[level++] = optarg;
        }
    }
    if (!any_init) {
        border[w * h / 2 + w / 2] = true;
    }

    level = 0;
    int numplacements = min(NUMCOLS, w * h);
    for (level = 0; level <= numtemplates; level++) {
        if (level < numtemplates) {
            fprintf(stderr, "setting template\n");
            if (!setok_template(template_files[level], w, h, ok)) {
                fprintf(stderr, "reading template failed\n");
                ret = 255;
                goto err;
            }
        }
        else {
            fprintf(stderr, "not setting template\n");
            for (int i = 0; i < w * h; i++) {
                ok[i] = true;
            }
        }
        while (nextcol < NUMCOLS) {
            int bestplace = -1;
            float bestscore = -INFINITY;
            color col = colors[nextcol];
            for (int i = 0; i < w * h; i++) {
                if (!border[i] || !ok[i]) {
                    continue;
                }
                float s = score(i, w, h, col, img, touched);
                if (s > bestscore) {
                    bestplace = i;
                    bestscore = s;
                }
                else if (s == bestscore) {
                    if (rand() % 2 == 0) {
                        bestplace = i;
                    }
                }
            }
            if (bestplace < 0) {
                break;
            }
            img[bestplace] = col;
            touched[bestplace] = true;
            border[bestplace] = false;
            int nls[9];
            int nn = neighbors(bestplace, w, h, touched, false, nls);
            for (int n = 0; n < nn; n++) {
                border[nls[n]] = true;
            }
            if (nextcol % 1024 == 0) {
                fprintf(stderr, "placed color %d/%d\n", nextcol,
                        numplacements);
            }
            nextcol++;
        }
    }
    fprintf(stderr, "placed color %d/%d\n", nextcol, numplacements);
    fprintf(stderr, "done placing colors\n");

    printf("P3\n");
    printf("%d %d\n", w, h);
    printf("255\n");
    printf("# rand() seed: %d\n", seed);
    for (int i = 0; i < w * h; i++) {
        if (touched[i]) {
            printf("%u %u %u\n", img[i].r, img[i].g, img[i].b);
        }
        else {
            printf("0 0 0\n");
        }
    }

 err:
    free(template_files);
 err6:
    free(ok);
 err5:
    free(colors);
 err4:
    free(border);
 err3:
    free(touched);
 err2:
    free(img);
 err1:
    return ret;
}
