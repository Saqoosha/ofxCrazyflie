#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
  crazyflie_.init();
  pitch_ = 0.0f;
  roll_ = 0.0f;
  yaw_ = 0.0f;
  thrust_ = 0.0f;
  memset(key_state_, 0, sizeof(key_state_));
}

//--------------------------------------------------------------
void testApp::update(){
  float d = 0.0f;
  if (key_state_['w'] || key_state_[',']) {
    d += 0.03f;
  }
  if (key_state_['s'] || key_state_['o']) {
    d -= 0.03f;
  }
  if (d == 0.0f) {
    pitch_ *= 0.99f;
  } else {
    pitch_ = ofClamp(pitch_ + d, -10.0f, 10.0f);
  }
  
  d = 0.0f;
  if (key_state_['a']) {
    d -= 0.03f;
  }
  if (key_state_['d'] || key_state_['e']) {
    d += 0.03f;
  }
  if (d == 0.0f) {
    roll_ *= 0.99f;
  } else {
    roll_ = ofClamp(roll_ + d, -10.0f, 10.0f);
  }
  
  d = -0.0001f;
  if (key_state_[OF_KEY_UP]) {
    d = 0.0001f;
  }
  if (key_state_[OF_KEY_DOWN]) {
    d = -0.0002f;
  }
  thrust_ = ofClamp(thrust_ + d, 0.15f, 0.7f);
  
  crazyflie_.command(roll_, pitch_, yaw_, thrust_);
}

//--------------------------------------------------------------
void testApp::draw(){
  ofBackground(30);

  ofDrawBitmapString(string("Control:\n") +
                     "  Pitch: " + ofToString(pitch_, 3) + "\n" +
                     "   Roll: " + ofToString(roll_, 3) + "\n" +
                     "    Yaw: " + ofToString(yaw_, 3) + "\n" +
                     " Thrust: " + ofToString(thrust_, 3)
                     , 10, 20);
  
  ofDrawBitmapString(string("Status:\n") +
                     " Battely level: " + ofToString(crazyflie_.getBatteryLevel(), 3) + "V\n" +
                     "  Actual pitch: " + ofToString(crazyflie_.getActualPitch(), 3) + "\n" +
                     "   Actual roll: " + ofToString(crazyflie_.getActualRoll(), 3) + "\n" +
                     "    Actual yaw: " + ofToString(crazyflie_.getActualYaw(), 3) + "\n" +
                     " Actual thrust: " + ofToString(crazyflie_.getActualThrust(), 3) + "\n" +
                     "       Motor 1: " + ofToString(crazyflie_.getMotorPWM(0), 3) + "\n" +
                     "       Motor 2: " + ofToString(crazyflie_.getMotorPWM(1), 3) + "\n" +
                     "       Motor 3: " + ofToString(crazyflie_.getMotorPWM(2), 3) + "\n" +
                     "       Motor 4: " + ofToString(crazyflie_.getMotorPWM(3), 3) + "\n"
                     , 10, 100);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
  key_state_[key] = true;
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
  key_state_[key] = false;
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}