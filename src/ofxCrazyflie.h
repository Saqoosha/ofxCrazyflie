//
//  ofxCrazyflie.h
//  Crazyflie
//
//  Created by Saqoosha on 13/05/13.
//
//

#pragma once

#include "ofMain.h"
#include "ofxCrazyradio.h"


class ofxCFLogEntry {
 public:
  ofxCFLogEntry(uint8_t *data) {
    id = data[1];
    type = data[2];
    switch (type) {
      case 0x01: size = 1; break;  // uint8_t
      case 0x02: size = 2; break;  // uint16_t
      case 0x03: size = 4; break;  // uint32_t
      case 0x04: size = 1; break;  // int8_t
      case 0x05: size = 2; break;  // int16_t
      case 0x06: size = 4; break;  // int32_t
      case 0x07: size = 4; break;  // float
      case 0x08: size = 2; break;  // FP16
      default: size = 0; break;
    }
    string group = string((const char *)(data + 3));
    name = group + "." + string((const char *)(data + 3 + group.length() + 1));
  }
  
//  uint8_t get_uint8() { return *(uint8_t*)value; }
//  uint16_t get_uint16() { return *(uint16_t*)value; }
//  float get_float() { return *(float*)value; }
  
  uint8_t id;
  uint8_t type;
  uint8_t size;
  string name;
//  uint8_t value[4];
};


class ofxCFLogBlock {
 public:
  // default update interval is 100ms
  ofxCFLogBlock(uint8_t id, uint8_t period = 10)
    : id_(id), period_(period) {
  }
  void AddEntry(ofxCFLogEntry *entry, void *data_ptr) {
    entries_.push_back(entry);
    data_ptrs_.push_back(data_ptr);
  }
  
  inline uint8_t get_id() { return id_; }
  inline uint8_t get_period() { return period_; }
//  inline vector<ofxCFLogEntry*> get_entries() { return entries_; }
  inline size_t get_num_entries() { return entries_.size(); }
  inline ofxCFLogEntry *get_entry(uint8_t index) { return entries_[index]; }
  inline void *get_data_ptr(uint8_t index) { return data_ptrs_[index]; }
  
 protected:
  uint8_t id_;
  uint8_t period_;
  vector<ofxCFLogEntry*> entries_;
  vector<void*> data_ptrs_;
};


class ofxCrazyflie {
 public:
  class RadioThread : public ofThread {
   public:
    RadioThread(ofxCrazyflie *crazyflie);
    void threadedFunction();
   protected:
    ofxCrazyflie *crazyflie_;
  };
  
 public:
  bool init();
  void close();
  void command(float roll, float pitch, float yaw, float thrust);
  
  float getBatteryLevel() { return battery_level_; }
  float getActualRoll() { return actual_roll_; }
  float getActualPitch() { return actual_pitch_; }
  float getActualYaw() { return actual_yaw_; }
  float getActualThrust() { return (float)actual_thrust_ / 0xffff; }
  float getMotorPWM(uint8_t index) { return (float)motor_pwm_[index] / 0xffff; }

 protected:
  uint8_t FindFirstChannel();
  
  void GetTOCInfo();
  void GetTOCItem(uint8_t index);
  
  void InitLogBlocks();
  void CreateLogBlock(ofxCFLogBlock *block);
  void DeleteLogBlock(uint8_t block_id);
  void StartLogBlock(uint8_t block_id);
  void ResetLogBlocks();
  void ParseLogBlock(ofxCRTPPacket *packet);

  void onUpdate(ofEventArgs &e);
  
  void _dumpPacket(ofxCRTPPacket *packet);
  
  
  ofxCrazyradio radio_;
  bool is_ready_;
  RadioThread *thread_;
  deque<ofxCRTPPacket*> packet_in_;
  deque<ofxCRTPPacket*> packet_out_;
  bool first_packet_;
  
  int num_log_entries_;
  int log_entry_request_index_;
  map<string, ofxCFLogEntry> log_entries_;
  vector<ofxCFLogBlock*> log_blocks_;
  
  float battery_level_;
  float actual_roll_;
  float actual_pitch_;
  float actual_yaw_;
  uint16_t actual_thrust_;
  int32_t motor_pwm_[4];
};
