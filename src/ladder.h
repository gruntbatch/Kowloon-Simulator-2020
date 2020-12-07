#pragma once


enum Continue {
    DOWN,
    UP,
};


typedef enum Continue (*Up)(void);
typedef void (*Down)(void);


void AddRung(Up up, Down down);
int Climb(void);
