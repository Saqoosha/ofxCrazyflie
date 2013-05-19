//
//  ofxCrazyradio.cpp
//  Crazyflie
//
//  Created by Saqoosha on 13/05/13.
//
//

#include <iostream>
#include "ofxCrazyradio.h"


ofxCrazyradio::ofxCrazyradio()
    : ctx_(NULL),
      handle_(NULL),
      version_(0.0f) {
}


bool ofxCrazyradio::init() {
  int ret = libusb_init(&ctx_);
//  std::cout << "libusb_init: " << ret << std::endl;
//  libusb_set_debug(ctx_, 3);
  
  bool found = false;
  libusb_device **list;
  ssize_t count = libusb_get_device_list(ctx_, &list);
//  std::cout << "libusb_get_device_list: " << count << std::endl;
  for (int i = 0; i < count; i++) {
    struct libusb_device_descriptor desc;
    ret = libusb_get_device_descriptor(list[i], &desc);
    if (desc.idVendor == 0x1915 && desc.idProduct == 0x7777) {
      found = true;
      ret = libusb_open(list[i], &handle_);
//      std::cout << "libusb_open: " << ret << std::endl;
      
//      std::cout << "version: " << std::hex << desc.bcdDevice << std::dec << std::endl;
      
//      uint8_t string_desc[256];
//      ret = libusb_get_string_descriptor_ascii(handle_, desc.iManufacturer, string_desc, 256);
//      std::cout << "libusb_get_string_descriptor_ascii: " << ret << std::endl;
//      std::cout << string_desc << std::endl;
//
      
//      for (int n = 0; n < desc.bNumConfigurations; n++) {
//        libusb_config_descriptor *config_desc;
//        const libusb_interface_descriptor *inter_desc;
//        const libusb_endpoint_descriptor *ep_desc;
//        const libusb_interface *inter;
//        libusb_get_config_descriptor(list[i], n, &config_desc);
//        std::cout << "config: " << (int)config_desc->bConfigurationValue << std::endl;
//        std::cout << "bNumInterfaces: " << (int)config_desc->bNumInterfaces << std::endl;
//        for (int i = 0; i < (int)config_desc->bNumInterfaces; i++) {
//          inter = &config_desc->interface[i];
//          std::cout << "  num_altsetting: " << inter->num_altsetting << std::endl;
//          for (int j = 0; j < inter->num_altsetting; j++) {
//            inter_desc = &inter->altsetting[j];
//            std::cout << "    Interface number: " << (int)inter_desc->bInterfaceNumber << std::endl;
//            std::cout << "    number of endpoint: " << (int)inter_desc->bNumEndpoints << std::endl;
//            for (int k = 0; k < (int)inter_desc->bNumEndpoints; k++) {
//              ep_desc = &inter_desc->endpoint[k];
//              std::cout << "     desc type: " << (int)ep_desc->bDescriptorType << std::endl;
//              std::cout << "     ep address: " << (int)ep_desc->bEndpointAddress << std::endl;
//              std::cout << "     attributes: " << std::hex << (int)ep_desc->bmAttributes << std::dec << std::endl;
//            }
//          }
//        }
//      }
      
//      ret = libusb_kernel_driver_active(handle_, 0);
//      std::cout << "libusb_kernel_driver_active: " << ret << std::endl;
      
      ret = libusb_claim_interface(handle_, 0);
//      std::cout << "libusb_claim_interface: " << ret << std::endl;
      
      libusb_set_configuration(handle_, 1);
      SetDataRate(DR_250KPS);
      SetChannel(2);
      arc_ = -1;
      SetContCarrier(false);
      uint8_t address[] = "\xe7\xe7\xe7\xe7\xe7";
      SetAddress(address);
      SetPower(P_0DBM);
      SetArc(3);
      SetArdBytes(32);
      break;
    }
  }
  libusb_free_device_list(list, 1);
  
  return found;
}


void ofxCrazyradio::close() {
  if (handle_) {
    libusb_release_interface(handle_, 0);
//    libusb_reset_device(handle_);
    libusb_close(handle_);
    handle_ = NULL;
  }
  libusb_exit(ctx_);
  ctx_ = NULL;
}


void ofxCrazyradio::SetChannel(uint16_t channel) {
  uint8_t empty[1];
  SendVendorSetup(SET_RADIO_CHANNEL, channel, 0, empty, 0);
}


void ofxCrazyradio::SetAddress(uint8_t *data) {
  SendVendorSetup(SET_RADIO_ADDRESS, 0, 0, data, 5);
}


void ofxCrazyradio::SetDataRate(DataRate data_rate) {
  uint8_t empty[1];
  SendVendorSetup(SET_DATA_RATE, data_rate, 0, empty, 0);
}


void ofxCrazyradio::SetPower(Power power) {
  uint8_t empty[1];
  SendVendorSetup(SET_RADIO_POWER, power, 0, empty, 0);
}


void ofxCrazyradio::SetArc(uint16_t arc) {
  uint8_t empty[1];
  SendVendorSetup(SET_RADIO_ARC, arc, 0, empty, 0);
  arc_ = arc;
}


void ofxCrazyradio::SetArdBytes(uint16_t nbytes) {
  uint8_t empty[1];
  SendVendorSetup(SET_RADIO_ARD, 0x80 | nbytes, 0, empty, 0);
}


void ofxCrazyradio::SetContCarrier(bool active) {
  uint8_t empty[1];
  SendVendorSetup(SET_CONT_CARRIER, active ? 1 : 0, 0, empty, 0);
}


int ofxCrazyradio::ScanChannels(uint16_t start, uint16_t stop, uint8_t *channels, uint16_t length) {
  uint8_t packet[] = {0xff};
  SendVendorSetup(SCANN_CHANNELS, start, stop, packet, 1);
  return GetVendorSetup(SCANN_CHANNELS, 0, 0, channels, length);
}


ofxCrazyradio::Ack *ofxCrazyradio::SendPacket(uint8_t *data_out, uint8_t length) {
//  std::cout << (int)data_out[0] << std::endl;
  int transfered = 0;
  int ret = libusb_bulk_transfer(handle_, 1 | LIBUSB_ENDPOINT_OUT, data_out, length, &transfered, 1000);
//  int ret = libusb_interrupt_transfer(handle_, 1 | LIBUSB_ENDPOINT_OUT, data_out, length, &transfered, 1000);
  if (ret || transfered < length)
    std::cerr << "send: " << libusb_error_name(ret) << ", " << (int)length << ", " << transfered << std::endl;

  Ack *ack = NULL;
  uint8_t data_in[64];
  transfered = 0;
  ret = libusb_bulk_transfer(handle_, 1 | LIBUSB_ENDPOINT_IN, data_in, 64, &transfered, 1000);
//  ret = libusb_interrupt_transfer(handle_, 1 | LIBUSB_ENDPOINT_IN, data_in, 64, &transfered, 1000);
  if (ret)
    std::cerr << "receive: " << libusb_error_name(ret) << ", " << transfered << std::endl;
  if (ret == 0 && transfered > 0) {
    ack = new Ack();
    memset(ack, 0, sizeof(Ack));
    if (data_in[0] != 0) {
      ack->ack_received = (data_in[0] & 0x01) != 0;
      ack->power_detector = (data_in[0] & 0x02) != 0;
      ack->num_retransmission = data_in[0] >> 4;
      ack->length = transfered - 1;
      memcpy(ack->data, &data_in[1], ack->length);
    } else {
      ack->num_retransmission = arc_;
    }
  }
  
  return ack;
}


void ofxCrazyradio::SendVendorSetup(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *data, uint16_t wLength) {
  uint8_t empty[1];
  int ret = libusb_control_transfer(handle_, LIBUSB_REQUEST_TYPE_VENDOR, bRequest, wValue, wIndex, data, wLength, 1000);
}


int ofxCrazyradio::GetVendorSetup(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t *data, uint16_t wLength) {
  return libusb_control_transfer(handle_, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, bRequest, wValue, wIndex, data, wLength, 1000);
}

