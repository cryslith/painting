#include <pam.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

const int WIDTH = 128;
const int HEIGHT = 128;
const unsigned char COLSKIP = 4;

typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;

bool nvalid(int r, int c, int w, int h, bool *touched,
            bool seektouch) {
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

bool setok(int level, int w, int h, bool *ok) {
    switch (level) {
    case 0:
        for (int i = 0; i < w * h; i++) {
            int r = i / w;
            int c = i % w;
            ok[i] = (r >= h / 3 && r < 2 * h / 3 &&
                     c >= w / 3 && c < 2 * w / 3);
        }
        return true;
    case 1:
        for (int i = 0; i < w * h; i++) {
            ok[i] = true;
        }
        return true;
    default:
        return false;
    }
}

int main(int argc, const char *argv[]) {
    int ret = 0;

    int w = WIDTH;
    int h = HEIGHT;
    int colskip = COLSKIP;
    bool shuffle = true;
    // TODO set parameters from command line
    color *img = malloc(sizeof(color) * w * h);
    if (img == NULL) {
        ret = 1;
        fprintf(stderr, "malloc img failed");
        goto err1;
    }
    bool *touched = malloc(sizeof(bool) * w * h);
    if (touched == NULL) {
        ret = 2;
        fprintf(stderr, "malloc touched failed");
        goto err2;
    }
    bool *border = malloc(sizeof(bool) * w * h);
    if (border == NULL) {
        ret = 3;
        fprintf(stderr, "malloc border failed");
        goto err3;
    }
    const int NUMCOLS = (256 / colskip) * (256 / colskip) * (256 / colskip);
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

    border[w * h / 2 + w / 2] = true;

    /*
    struct pam inpam;
    tuple *tuplerow;
    pm_init("", 0);
    pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));
    if (inpam.width != WIDTH) {
        fprintf(stderr, "width must be %d, not %d", WIDTH, inpam.width);
    }
    if (inpam.height != HEIGHT) {
        fprintf(stderr, "height must be %d, not %d", HEIGHT, inpam.height);
    }
    if (inpam.depth != 1) {
        fprintf(stderr, "depth must be 1, not %d", inpam.depth);
        return 1;
    }
    tuplerow = pnm_allocpamrow(&inpam);
    for (int row = 0; row < inpam.height; row++) {
        pnm_readpamrow(&inpam, tuplerow);
        for (int column = 0; column < inpam.width; column++) {
            
        }
    }
    */

    int level = 0;
    while (setok(level++, w, h, ok)) {
        int numplacements = min(NUMCOLS, w * h);
        while (nextcol < NUMCOLS) {
            int bestplace = -1;
            float bestscore = -INFINITY;
            color col = colors[nextcol++];
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
        }
    }

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
