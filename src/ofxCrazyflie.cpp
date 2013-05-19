//
//  ofxCrazyflie.cpp
//  Crazyflie
//
//  Created by Saqoosha on 13/05/13.
//
//

#include <iostream>
#include "ofxCrazyflie.h"


static ofxCRTPPacket ping((uint8_t *)"\xff", 0, true);


ofxCrazyflie::RadioThread::RadioThread(ofxCrazyflie *crazyfile)
    : crazyflie_(crazyfile) {
}


void ofxCrazyflie::RadioThread::threadedFunction() {
  int wait_time = 0;
  int emtpy_count = 0;
  
  while (isThreadRunning() || crazyflie_->packet_out_.size()) {

    ofxCRTPPacket *packet = NULL;
    ofxCrazyradio::Ack *ack = NULL;
    
    if (crazyflie_->packet_out_.size() == 0) {
      sleep(wait_time);
      packet = &ping;
    } else {
      lock();
      packet = crazyflie_->packet_out_.front();
      crazyflie_->packet_out_.pop_front();
      unlock();
    }
    
    ack = crazyflie_->radio_.SendPacket(packet->raw_data(), packet->length() + 1);
    
    if (packet->need_response()) {
      if (ack == NULL || !ack->ack_received) {
        delete ack;
        if (packet != &ping) {
          lock();
          crazyflie_->packet_out_.push_front(packet);
          unlock();
        }
        ofLog(OF_LOG_WARNING, "retry: %d, %d, %d", packet->sequence(), packet->port(), packet->channel());
        sleep(200);
        continue;
      }
      if (packet != &ping) {
//        ofLog(OF_LOG_VERBOSE, "got response: %d, %d, %d", packet->sequence(), packet->port(), packet->channel());
        delete packet;
      }
    }
    
    if (ack->length > 0) {
      lock();
      crazyflie_->packet_in_.push_back(new ofxCRTPPacket(ack->data, MAX(0, ack->length - 1)));
      unlock();
      wait_time = 0;
      emtpy_count = 0;
    } else {
      wait_time = ++emtpy_count > 10 ? 10 : 0;
    }
    delete ack;
  }
}


bool ofxCrazyflie::init() {
  thread_ = NULL;
  is_ready_ = false;
  first_packet_ = true;

  if (!radio_.init()) {
    ofLog(OF_LOG_ERROR, "Crazyradio init failed.");
    return false;
  }
  
  uint8_t channel = FindFirstChannel();
  if (channel == 0) {
    radio_.close();
    ofLog(OF_LOG_ERROR, "Crazyflie not found.");
    return false;
  }
  
  radio_.SetArc(10);
  radio_.SetDataRate(ofxCrazyradio::DR_250KPS);
  ofLog(OF_LOG_NOTICE, "Connecting to channel %d", channel);
  radio_.SetChannel(channel);

  thread_ = new RadioThread(this);
  thread_->startThread();
  
  ofAddListener(ofEvents().update, this, &ofxCrazyflie::onUpdate);
  
  is_ready_ = true;

  ResetLogBlocks();
  GetTOCInfo();

  return true;
}


void ofxCrazyflie::close() {
  if (thread_) {
    command(0.0f, 0.0f, 0.0f, 0.0f);
    for (vector<ofxCFLogBlock*>::iterator it = log_blocks_.begin(); it != log_blocks_.end(); it++) {
      ofxCFLogBlock *block = *it;
      DeleteLogBlock(block->get_id());
      delete block;
    }
    thread_->stopThread();
    thread_->waitForThread();
    thread_ = NULL;
    radio_.close();
  }
}


void ofxCrazyflie::command(float roll, float pitch, float yaw, float thrust) {
  if (!is_ready_) return;
  
  uint8_t data[14];
  *(float*)(data + 0) = ofClamp(roll, -90.0f, 90.0f);
  *(float*)(data + 4) = ofClamp(-pitch, -90.0f, 90.0f);
  *(float*)(data + 8) = ofClamp(yaw, -180.0f, 180.0f);
  *(uint16_t *)(data + 12) = ofClamp(thrust, 0.0f, 1.0f) * 0xffff;
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->set_need_response(false);
  packet->SetHeader(ofxCRTPPacket::COMMANDER, 0);
  packet->SetData(data, 14);
  if (packet_out_.size() < 50) packet_out_.push_back(packet);
}


uint8_t ofxCrazyflie::FindFirstChannel() {
  radio_.SetArc(1);
  radio_.SetDataRate(ofxCrazyradio::DR_250KPS);
  uint8_t channels[64];
  int num_channels = radio_.ScanChannels(0, 125, channels, 64);
  ofLog(OF_LOG_NOTICE, "%d channels found", num_channels);
  return num_channels > 0 ? channels[0] : 0;
}


void ofxCrazyflie::GetTOCInfo() {
  uint8_t data[] = {0x01};
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 0);
  packet->SetData(data, 1);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::GetTOCItem(uint8_t index) {
  ofLog(OF_LOG_VERBOSE, "GetTOCItem: %d", index);
  uint8_t data[] = {0x00, index};
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 0);
  packet->SetData(data, 2);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::InitLogBlocks() {
  ofxCFLogBlock *block = new ofxCFLogBlock(log_blocks_.size(), 100);
  block->AddEntry(&log_entries_.at("pm.vbat"), &battery_level_);
  log_blocks_.push_back(block);
  CreateLogBlock(block);

  block = new ofxCFLogBlock(log_blocks_.size(), 10);
  block->AddEntry(&log_entries_.at("stabilizer.roll"), &actual_roll_);
  block->AddEntry(&log_entries_.at("stabilizer.pitch"), &actual_pitch_);
  block->AddEntry(&log_entries_.at("stabilizer.yaw"), &actual_yaw_);
  block->AddEntry(&log_entries_.at("stabilizer.thrust"), &actual_thrust_);
  log_blocks_.push_back(block);
  CreateLogBlock(block);

  block = new ofxCFLogBlock(log_blocks_.size(), 10);
  block->AddEntry(&log_entries_.at("motor.m1"), &motor_pwm_[0]);
  block->AddEntry(&log_entries_.at("motor.m2"), &motor_pwm_[1]);
  block->AddEntry(&log_entries_.at("motor.m3"), &motor_pwm_[2]);
  block->AddEntry(&log_entries_.at("motor.m4"), &motor_pwm_[3]);
  log_blocks_.push_back(block);
  CreateLogBlock(block);
}


void ofxCrazyflie::CreateLogBlock(ofxCFLogBlock *block) {
  uint8_t data[32];
  data[0] = 0x00;  // Create block
  data[1] = block->get_id();
  uint8_t *p = data + 2;
  for (int i = 0; i < block->get_num_entries(); i++) {
    ofxCFLogEntry *entry = block->get_entry(i);
    uint8_t type = entry->type;
    *p++ = (type << 4) | type;
    *p++ = entry->id;
  }
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 1);
  packet->SetData(data, p - data);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::DeleteLogBlock(uint8_t block_id) {
  uint8_t data[32];
  data[0] = 0x02;  // Delete block
  data[1] = block_id;
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 1);
  packet->SetData(data, 2);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::StartLogBlock(uint8_t block_id) {
  ofxCFLogBlock *block = log_blocks_[block_id];
  uint8_t data[32];
  data[0] = 0x03;  // Delete block
  data[1] = block->get_id();
  data[2] = block->get_period();
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 1);
  packet->SetData(data, 3);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::ResetLogBlocks() {
  uint8_t data[32];
  data[0] = 0x05;  // Reset block
  ofxCRTPPacket *packet = new ofxCRTPPacket();
  packet->SetHeader(ofxCRTPPacket::LOG, 1);
  packet->SetData(data, 1);
  packet_out_.push_back(packet);
}


void ofxCrazyflie::ParseLogBlock(ofxCRTPPacket *packet) {
  uint8_t *data = packet->data();
  if (log_blocks_.size() <= data[0]) {
    ofLog(OF_LOG_ERROR, "No log block with id: %d", (int)data[0]);
    return;
  }
  ofxCFLogBlock *block = log_blocks_[data[0]];
  data += 4;
  for (int i = 0; i < block->get_num_entries(); i++) {
    ofxCFLogEntry *entry = block->get_entry(i);
    memcpy(block->get_data_ptr(i), data, entry->size);
    data += entry->size;
  }
}


void ofxCrazyflie::onUpdate(ofEventArgs &e) {
  thread_->lock();
  while (!packet_in_.empty()) {
    ofxCRTPPacket *packet = packet_in_.front();
    packet_in_.pop_front();
    
    if (first_packet_) {
//      ResetLogBlocks();
//      GetTOCInfo();
      first_packet_ = false;
    }
    
    switch (packet->port()) {
      case ofxCRTPPacket::CONSOLE: {
        for (int i = 0; i < packet->length(); i++) {
          std::cout << packet->data()[i];
        }
        break;
      }
      case ofxCRTPPacket::COMMANDER: {
        ofLog(OF_LOG_VERBOSE, "commander ack:");
        _dumpPacket(packet);
        break;
      }
      case ofxCRTPPacket::LOG: {
        switch (packet->channel()) {
          case 0: {  // TOC
//            cout << "Log TOC:" << endl;
//            _dumpPacket(packet);
            switch (packet->data()[0]) {  // first data bytes indicates the command
              case 0x00: {  // TOC item
                ofxCFLogEntry entry(packet->data());
                ofLog(OF_LOG_NOTICE, "Log entry: %d: %s (type=%d)", (int)entry.id, entry.name.c_str(), (int)entry.type);
                log_entries_.insert(make_pair(entry.name, entry));
                if (++log_entry_request_index_ < num_log_entries_) {
                  GetTOCItem(log_entry_request_index_);
                } else {
                  InitLogBlocks();
                }
                break;
              }
              case 0x01: {  // TOC CRC and count
                uint8_t *p = packet->data();
                num_log_entries_ = p[1];
                ofLog(OF_LOG_NOTICE, "Number of log entry: %d", num_log_entries_);
                ofLog(OF_LOG_NOTICE, "CRC32 of log TOC: 0x%08lX", *(uint32_t*)(p + 2));
                log_entry_request_index_ = 0;
                GetTOCItem(0);
                break;
              }
            }
            break;
          }
          case 1: {  // Log setting
//            cout << "Log setting:" << endl;
//            _dumpPacket(packet);
            switch (packet->data()[0]) {  // command?
              case 0x00: {  // create block
                if (packet->data()[2] == 0) {  // no error
                  StartLogBlock(packet->data()[1]);
                }
                break;
              }
            }
            break;
          }
          case 2: {  // Log data
//            ofLog(OF_LOG_VERBOSE, "Log data:");
//            _dumpPacket(packet);
            ParseLogBlock(packet);
            break;
          }
        }
        break;
      }
      default: {
        _dumpPacket(packet);
        break;
      }
    }
    delete packet;
  }
  thread_->unlock();
}


void ofxCrazyflie::_dumpPacket(ofxCRTPPacket *packet) {
  ofLog(OF_LOG_VERBOSE, "  port: %d", (int)packet->port());
  ofLog(OF_LOG_VERBOSE, "  channel: %d", (int)packet->channel());
  for (int i = 0; i < packet->length(); i++) {
    ofLog(OF_LOG_VERBOSE, "    data: %d: %d", i, (int)packet->data()[i]);
  }
}

