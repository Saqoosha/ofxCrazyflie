//
//  ofxCrazyradio.h
//  Crazyflie
//
//  Created by Saqoosha on 13/05/13.
//
//

#pragma once

#include "libusb.h"
#include "ofxCRTPPacket.h"


class ofxCrazyradio {
 public:
  enum Request {
    SET_RADIO_CHANNEL = 0x01,
    SET_RADIO_ADDRESS = 0x02,
    SET_DATA_RATE = 0x03,
    SET_RADIO_POWER = 0x04,
    SET_RADIO_ARD = 0x05,
    SET_RADIO_ARC = 0x06,
    ACK_ENABLE = 0x10,
    SET_CONT_CARRIER = 0x20,
    SCANN_CHANNELS = 0x21,
    LAUNCH_BOOTLOADER = 0xff,
  };
  
  enum DataRate {
    DR_250KPS = 0,
    DR_1MPS = 1,
    DR_2MPS = 2,
  };
  
  enum Power {
    P_M18DBM = 0,
    P_M12DBM = 1,
    P_M6DBM = 2,
    P_0DBM = 3,
  };
  
  class Ack {
   public:
    bool ack_received;
    bool power_detector;
    int num_retransmission;
    int length;
    uint8_t data[32];
  };
  
  ofxCrazyradio();
  
  bool init();
  void close();
  
  void SetDataRate(DataRate data_rate);
  void SetChannel(uint16_t channel);
  void SetAddress(uint8_t *data);
  void SetPower(Power power);
  void SetArc(uint16_t arc);
  void SetArdBytes(uint16_t nbytes);
  void SetContCarrier(bool active);
  
  int ScanChannels(uint16_t start, uint16_t stop, uint8_t *channels, uint16_t length);
  
  Ack *SendPacket(uint8_t *data, uint8_t length);
  
 protected:
  void SendVendorSetup(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *data, uint16_t wLength);
  int GetVendorSetup(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *data, uint16_t wLength);
  
  libusb_context *ctx_;
  libusb_device_handle *handle_;
  int arc_;
  float version_;
};
