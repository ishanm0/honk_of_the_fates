#ifndef NEOPIXEL_H
#define NEOPIXEL_H

typedef enum
{
    ROCK,
    PAPER,
    SISSORS,
    UNSPECIFIED
} spell;

void WS2812_SPI_Init(void);

void spellPulse(spell used_spell, int start);
void setDefaultRGB(int RED, int GREEN, int BLUE);
void getDefaultRGB(int *RED, int *GREEN, int *BLUE);

#endif /* NEOPIXEL_H */