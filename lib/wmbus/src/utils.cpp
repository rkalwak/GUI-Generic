/***********************************************************************************
    Filename: utils.cpp
***********************************************************************************/

#include "utils.hpp"
#include "aes.hpp"
#include <vector>


//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
//  void dumpHex(uint8_t* data, uint8_t len, bool newLine) 
//
//  DESCRIPTION:
//      Print data buffer in HEX 
//
//  ARGUMENTS: 
//      uint8_t* data  - Data to perform the CRC-16 operation on.
//      uint8_t len    - Length to print
//      bool newLine   - Should add new line at the end
//-------------------------------------------------------------------------------------------------------
void dumpHex(uint8_t *data, int len, bool newLine) {
  char buffHex[3];
  for (int i = 0; i < len; i++) {
    sprintf(buffHex, "%02X", data[i]);
    Serial.print(buffHex);
  }
  if (newLine) {
    Serial.println();
  }
  else {
    Serial.print(" ");
  }
}

void dumpInt(uint8_t *data, int len, bool newLine)
{
   for (int i = 0; i < len; i++)
  {
    Serial.print(data[i]);
    Serial.print(",");
  }
  Serial.println(";");
}

std::string str_snprintf(const char *fmt, size_t len, ...)
{
  std::string str;
  va_list args;

  str.resize(len);
  va_start(args, len);
  size_t out_length = vsnprintf(&str[0], len + 1, fmt, args);
  va_end(args);

  if (out_length < len)
    str.resize(out_length);

  return str;
}
std::string to_string(int value) { return str_snprintf("%d", 32, value); }
char format_hex_pretty_char(uint8_t v) { return v >= 10 ? 'A' + (v - 10) : '0' + v; }
std::string format_hex_pretty(const uint16_t *data, size_t length)
{
  if (length == 0)
    return "";
  std::string ret;
  ret.resize(5 * length - 1);
  for (size_t i = 0; i < length; i++)
  {
    ret[5 * i] = format_hex_pretty_char((data[i] & 0xF000) >> 12);
    ret[5 * i + 1] = format_hex_pretty_char((data[i] & 0x0F00) >> 8);
    ret[5 * i + 2] = format_hex_pretty_char((data[i] & 0x00F0) >> 4);
    ret[5 * i + 3] = format_hex_pretty_char(data[i] & 0x000F);
    if (i != length - 1)
      ret[5 * i + 2] = '.';
  }
  if (length > 4)
    return ret + " (" + to_string(length) + ")";
  return ret;
}

std::string format_hex_pretty(const uint8_t *data, size_t length)
{
  if (length == 0)
    return "";
  std::string ret;
  ret.resize(3 * length - 1);
  for (size_t i = 0; i < length; i++)
  {
    ret[3 * i] = format_hex_pretty_char((data[i] & 0xF0) >> 4);
    ret[3 * i + 1] = format_hex_pretty_char(data[i] & 0x0F);
    if (i != length - 1)
      ret[3 * i + 2] = '.';
  }
  if (length > 4)
    return ret + " (" + to_string(length) + ")";
  return ret;
}

std::string format_hex_pretty(std::vector<unsigned char> data)
{
  return format_hex_pretty(data.data(), data.size());
}

void phex(uint8_t *str, int len, int start )
{

  unsigned char i;
  for (i = start; i < len; ++i)
  {
    printf("%.2x,", str[i]);
    if (i != 0 && i % 16 == 0)
      printf("\n");
  }
  printf("\n");
}

void printHexString(uint8_t *str, int len, int start)
{

  unsigned char i;
  for (i = start; i < len; ++i)
  {
    printf("%.2X", str[i]);
  }
  printf("\n");
}

unsigned char *safeButUnsafeVectorPtr(std::vector<unsigned char> &v) {
    if (v.size() == 0) return NULL;
    return &v[0];
}

bool decrypt_TPL_AES_CBC_IV(std::vector<unsigned char> &frame,
                            std::vector<unsigned char>::iterator &pos,
                            std::vector<unsigned char> &key,
                            unsigned char *iv,
                            int *num_encrypted_bytes,
                            int *num_not_encrypted_at_end) {
  std::vector<unsigned char> buffer;
  buffer.insert(buffer.end(), pos, frame.end());
  size_t num_bytes_to_decrypt = frame.end()-pos;
  uint8_t tpl_num_encr_blocks = 3; //t->tpl_num_encr_blocks

  if (tpl_num_encr_blocks) {
    num_bytes_to_decrypt = tpl_num_encr_blocks*16;
  }

  *num_encrypted_bytes = num_bytes_to_decrypt;
  *num_not_encrypted_at_end = buffer.size()-num_bytes_to_decrypt;

  if (key.size() == 0) {
    return false;
  }

  if (buffer.size() < num_bytes_to_decrypt) {
    num_bytes_to_decrypt = buffer.size();
  }

  // The content should be a multiple of 16 since we are using AES CBC mode.
  if (num_bytes_to_decrypt % 16 != 0) {
    assert (num_bytes_to_decrypt % 16 == 0);
  }

  unsigned char buffer_data[num_bytes_to_decrypt];
  memcpy(buffer_data, safeButUnsafeVectorPtr(buffer), num_bytes_to_decrypt);
  unsigned char decrypted_data[num_bytes_to_decrypt];
  memcpy(decrypted_data, buffer_data,num_bytes_to_decrypt);
  //Serial.println("IV:");
  //dumpHex(iv, 16);
  //Serial.println("Payload:");
  //dumpHex(decrypted_data, num_bytes_to_decrypt);
  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, &key[0], iv);

  AES_CBC_decrypt_buffer(&ctx, decrypted_data, num_bytes_to_decrypt);

  // Remove the encrypted bytes.
  frame.erase(pos, frame.end());

  // Insert the decrypted bytes.
  //frame.insert(frame.end(), decrypted_data, decrypted_data+num_bytes_to_decrypt);
  frame.insert(frame.end(), decrypted_data, decrypted_data+num_bytes_to_decrypt);

  if (num_bytes_to_decrypt < buffer.size()) {
    frame.insert(frame.end(), buffer.begin()+num_bytes_to_decrypt, buffer.end());
  }
  return true;
}