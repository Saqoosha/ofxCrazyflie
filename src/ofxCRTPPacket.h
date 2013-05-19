//
//  ofxCRTPPacket.h
//  Crazyflie
//
//  Created by Saqoosha on 13/05/13.
//
//

#pragma once


class ofxCRTPPacket {
  static uint32_t next_sequence_;
  
 public:
  enum ofxCRTPPort {
    CONSOLE = 0,
    PARAMETERS = 2,
    COMMANDER = 3,
    LOG = 5,
    DEBUGGING = 14,
    LINK_LAYER = 15,
  };

  // length is actual data length, not include header
  ofxCRTPPacket(uint8_t *data = NULL, uint8_t length = 0, bool need_response = true);
  
  void SetHeader(ofxCRTPPort port, uint8_t channel);
  void SetData(uint8_t *data, uint8_t length);
  
  inline uint32_t sequence() { return sequence_; }
  inline ofxCRTPPort port() { return ofxCRTPPort((data_[0] & 0xf0) >> 4); }
  inline uint8_t channel() { return data_[0] & 0x03; }
  inline uint8_t *data() { return &data_[1]; }
  inline uint8_t *raw_data() { return data_; }
  inline uint8_t length() { return length_; }
  inline bool need_response() { return need_response_; }
  inline void set_need_response(bool value) { need_response_ = value; }
  
 protected:
  uint32_t sequence_;
  uint8_t data_[32];
  uint8_t length_;
  bool need_response_;
};

