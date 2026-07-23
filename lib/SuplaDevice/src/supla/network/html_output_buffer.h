/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef SRC_SUPLA_NETWORK_HTML_OUTPUT_BUFFER_H_
#define SRC_SUPLA_NETWORK_HTML_OUTPUT_BUFFER_H_

#include <stddef.h>

namespace Supla {

constexpr int SUPLA_HTML_OUTPUT_BUFFER_SIZE = 512;

class HtmlOutputBuffer {
 public:
  using FlushCallback = bool (*)(void *context, const char *buf, int size);

  explicit HtmlOutputBuffer(char *buffer = nullptr, int bufferLen = 0);

  void setBuffer(char *buffer, int bufferLen);
  void send(void *context,
            FlushCallback flushCallback,
            const char *buf,
            int size = -1);
  bool flush(void *context, FlushCallback flushCallback);
  bool error() const;

 private:
  bool flushPending(void *context, FlushCallback flushCallback);

  char *buffer_ = nullptr;
  int bufferLen_ = 0;
  int bufferPos_ = 0;
  bool error_ = false;
};

};  // namespace Supla

#endif  // SRC_SUPLA_NETWORK_HTML_OUTPUT_BUFFER_H_
