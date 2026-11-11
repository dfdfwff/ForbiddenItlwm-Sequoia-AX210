// EAPOL-Key length checks (the receive path in ieee80211_pae_input.c uses the same function).
#include <EAPOLKeyLength.hpp>
#include <cassert>
#include <cstdio>
#include <initializer_list>
using ItlEAPOL::keyLengthsValid;
static const size_t kHeader = 99, kMax = 2048;   // sizeof(struct ieee80211_eapol_key), MCLBYTES
int main(){
    // A declared body shorter than the fixed header is refused, whatever the key data length says.
    assert(!keyLengthsValid(99, 0, 65535, kHeader, kMax));
    assert(!keyLengthsValid(99, 0, 0, kHeader, kMax));
    assert(!keyLengthsValid(99, 94, 65535, kHeader, kMax));
    assert(!keyLengthsValid(99, 94, 0, kHeader, kMax));
    // The smallest real frame: header only, no key data.
    assert(keyLengthsValid(99, 95, 0, kHeader, kMax));
    assert(!keyLengthsValid(99, 95, 1, kHeader, kMax));
    // Key data must fit inside the declared frame, exactly.
    assert(keyLengthsValid(200, 196, 101, kHeader, kMax));
    assert(!keyLengthsValid(200, 196, 102, kHeader, kMax));
    // The declared frame must fit in what was received.
    assert(!keyLengthsValid(150, 196, 0, kHeader, kMax));
    // And under the receive limit.
    assert(keyLengthsValid(4000, 2044, 0, kHeader, kMax));
    assert(!keyLengthsValid(4000, 2045, 0, kHeader, kMax));
    // Every combination: accepted means header and key data both lie inside the received bytes.
    for (unsigned body = 0; body < 400; ++body)
        for (unsigned pay = 0; pay < 400; ++pay)
            for (size_t packet : {size_t(0), size_t(98), size_t(99), size_t(150), size_t(300), size_t(500)})
                if (keyLengthsValid(packet, uint16_t(body), uint16_t(pay), kHeader, kMax))
                    assert(4 + body >= kHeader && 4 + body <= packet && kHeader + pay <= 4 + body);
    puts("eapol_key_length: ok");
}
