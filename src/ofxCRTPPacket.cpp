//
//  ofxCRTPPacket.cpp
//  Crazyflie
//
//  Created by Saqoosha on 2013/05/17.
//
//

#include "ofxCRTPPacket.h"


uint32_t ofxCRTPPacket::next_sequence_ = 0;


ofxCRTPPacket::ofxCRTPPacket(uint8_t *data, uint8_t length, bool need_response)
    : length_(length), need_response_(need_response) {
  assert(length < 32);
  memset(data_, 0, 32);
  if (data) {
    memcpy(data_, data, length_ + 1);
  }
  sequence_ = next_sequence_++;
}


void ofxCRTPPacket::SetHeader(ofxCRTPPort port, uint8_t channel) {
  data_[0] = (port & 0x0f) << 4 | 0x03 << 2 | (channel & 0x03);
}


void ofxCRTPPacket::SetData(uint8_t *data, uint8_t length) {
  assert(0 < length && length < 32);
  length_ = length;
  memcpy(&data_[1], data, length_);
}
