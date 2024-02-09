/***********************************************************************************
    Filename: utils.hpp
***********************************************************************************/

#ifndef _UTILS_H
#define _UTILS_H

#include <Arduino.h>
#include <stdint.h>
#include <vector>
#include <string>

//----------------------------------------------------------------------------------
//  Function Declareration
//----------------------------------------------------------------------------------
void dumpHex(uint8_t* data, int len, bool newLine = true);
void dumpInt(uint8_t* data, int len, bool newLine = true);
unsigned char *safeButUnsafeVectorPtr(std::vector<unsigned char> &v);
bool decrypt_TPL_AES_CBC_IV(std::vector<unsigned char> &frame, std::vector<unsigned char>::iterator &pos,
                            std::vector<unsigned char> &key, unsigned char *iv,
                            int *num_encrypted_bytes,
                            int *num_not_encrypted_at_end);
std::string str_snprintf(const char *fmt, size_t len, ...);
std::string to_string(int value);
char format_hex_pretty_char(uint8_t v);
void phex(uint8_t *str, int len = 16, int start = 0);
void printHexString(uint8_t *str, int len = 16, int start = 0);

#endif