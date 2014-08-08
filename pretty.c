#include <pam.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

const unsigned int WIDTH = 128;
const unsigned int HEIGHT = 128;
const unsigned char COLSKIP = 4;
const bool SHUFFLE = true;
unsigned int NUMCOLS;

typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;

bool nvalid(int r, int c, bool *touched, bool seektouch) {
    if (r < 0 || r >= HEIGHT || c < 0 || c >= WIDTH) {
        return false;
    }
    return touched[r * WIDTH + c] == seektouch;
}

int neighbors(int pos, bool *touched, bool seektouch, int nls[9]) {
    int i = 0;
    int r = pos / WIDTH, c = pos % WIDTH;
    for (int ro = -1; ro <= 1; ro++) {
        for (int co = -1; co <= 1; co++) {
            if (!(ro || co)) {
                continue;
            }
            if (nvalid(r + ro, c + co, touched, seektouch)) {
                nls[i++] = (r + ro) * WIDTH + c + co;
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

float score(int i, color col, color *img, bool *touched) {
    int nls[9];
    int nn = neighbors(i, touched, true, nls);
    float mindist = INFINITY;
    for (int n = 0; n < nn; n++) {
        float d = dist(col, img[nls[n]]);
        if (d < mindist) {
            mindist = d;
        }
    }
    return -mindist;
}

int main(int argc, const char *argv[]) {
    color img[WIDTH * HEIGHT];
    bool touched[WIDTH * HEIGHT];
    bool border[WIDTH * HEIGHT];
    //    bool initial[WIDTH * HEIGHT];

    int seed = time(NULL);
    srand(seed);
    fprintf(stderr, "seed: %d\n", seed);

    NUMCOLS = (256 / COLSKIP) * (256 / COLSKIP) * (256 / COLSKIP);
    color *colors = malloc(NUMCOLS * sizeof(color));
    if (colors == NULL) {
        fprintf(stderr, "malloc colors failed\n");
        return EXIT_FAILURE;
    }

    int nextcol = 0;
    for (int r = 0; r < 256; r += COLSKIP) {
        for (int g = 0; g < 256; g += COLSKIP) {
            for (int b = 0; b < 256; b += COLSKIP) {
                colors[nextcol++] = (color) {r, g, b};
            }
        }
    }
    nextcol = 0;

    if (SHUFFLE) {
        int j;
        color swap;
        for (int c = 0; c < NUMCOLS; c++) {
            j = rand() % (NUMCOLS - c) + c;
            swap = colors[c];
            colors[c] = colors[j];
            colors[j] = swap;
        }
    }

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        touched[i] = false;
        border[i] = false;
    }

    img[WIDTH * HEIGHT / 2 + WIDTH / 2] = colors[nextcol++];
    touched[WIDTH * HEIGHT / 2 + WIDTH / 2] = true;
    int nls[9];
    int nn = neighbors(WIDTH * HEIGHT / 2 + WIDTH / 2, touched, false, nls);
    for (int n = 0; n < nn; n++) {
        border[nls[n]] = true;
    }

    /*
    struct pam inpam;
    tuple *tuplerow;
    pm_init(argv[0], 0);
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

    int numplacements = min(NUMCOLS, WIDTH * HEIGHT);
    while (nextcol < NUMCOLS) {
        int bestplace = -1;
        float bestscore = -INFINITY;
        color col = colors[nextcol++];
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            if (!border[i]) {
                continue;
            }
            float s = score(i, col, img, touched);
            if (s > bestscore) {
                bestplace = i;
                bestscore = s;
            }
            else if (s == bestscore) {
                if (rand() % 2) {
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
        int nn = neighbors(bestplace, touched, false, nls);
        for (int n = 0; n < nn; n++) {
            border[nls[n]] = true;
        }
        if (nextcol % 1024 == 0) {
            fprintf(stderr, "placed color %d/%d\n", nextcol, numplacements);
        }
    }

    free(colors);

    printf("P3\n");
    printf("%u %u\n", WIDTH, HEIGHT);
    printf("255\n");
    printf("# rand() seed: %d\n", seed);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (touched[i]) {
            printf("%u %u %u\n", img[i].r, img[i].g, img[i].b);
        }
        else {
            printf("0 0 0\n");
        }
    }
}
