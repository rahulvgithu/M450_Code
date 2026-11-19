#pragma once

namespace tarif {

const unsigned long STARTGEBUEHR_RAPPEN = 100;    // CHF 1.00
const unsigned long PREIS_PRO_MINUTE_RAPPEN = 10; // CHF 0.10

// CHF 1.00 + CHF 0.10 pro angefangene Minute
inline unsigned long gebuehrRappen(unsigned long dauerMs) {
    unsigned long minuten = (dauerMs + 59999UL) / 60000UL;
    return STARTGEBUEHR_RAPPEN + minuten * PREIS_PRO_MINUTE_RAPPEN;
}

} // namespace tarif
