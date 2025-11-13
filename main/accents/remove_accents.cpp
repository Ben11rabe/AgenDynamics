#include "remove_accents.h"

std::string removeAccents(const std::string &input) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ) {

        unsigned char c = input[i];

        // Accents en UTF-8 : commencent par 0xC3
        if (c == 0xC3 && i + 1 < input.size()) {
            unsigned char next = input[i + 1];

            switch (next) {
                case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA5: // à á â ã å
                    out += 'a'; break;
                case 0xA4: // ä
                    out += 'a'; break;

                case 0xA8: case 0xA9: case 0xAA: case 0xAB: // è é ê ë
                    out += 'e'; break;

                case 0xAC: case 0xAD: case 0xAE: case 0xAF: // ì í î ï
                    out += 'i'; break;

                case 0xB2: case 0xB3: case 0xB4: case 0xB6: case 0xB5: // ò ó ô õ ö
                    out += 'o'; break;

                case 0xB9: case 0xBA: case 0xBB: case 0xBC: // ù ú û ü
                    out += 'u'; break;

                case 0xA7: // ç
                    out += 'c'; break;

                default: 
                    out += '?';
            }
            i += 2;
            continue;
        }

        // Autre caractère ASCII
        out += input[i];
        i++;
    }

    return out;
}
