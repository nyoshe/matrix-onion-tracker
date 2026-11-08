// Hand-expanded REGISTER_SCREEN: that macro's default-constructed instance
// can't carry a slot index, so each slot is spelled out below instead.

#include "CandlestickScreen.h"

static_assert(MAX_TRACKED_COINS == 6, "update the slot list in this file to match");

CandlestickScreen Coin0Screen(0);
CandlestickScreen Coin1Screen(1);
CandlestickScreen Coin2Screen(2);
CandlestickScreen Coin3Screen(3);
CandlestickScreen Coin4Screen(4);
CandlestickScreen Coin5Screen(5);

static ScreenRegistrar Coin0Registrar(&Coin0Screen, 10);
static ScreenRegistrar Coin1Registrar(&Coin1Screen, 11);
static ScreenRegistrar Coin2Registrar(&Coin2Screen, 12);
static ScreenRegistrar Coin3Registrar(&Coin3Screen, 13);
static ScreenRegistrar Coin4Registrar(&Coin4Screen, 14);
static ScreenRegistrar Coin5Registrar(&Coin5Screen, 15);
