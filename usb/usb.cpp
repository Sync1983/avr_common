/*
 * usb.cpp
 *
 * Created: 23.12.2016 22:42:26
 *  Author: Sync
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usb.h"
#include "../common/common.h"
#include <string.h>

USB::USB(uint16_t vid, uint16_t pid, float usbVer, bool useString){  
  this->useStr = useString;
  uint8_t hiVer = (uint8_t)usbVer;
  uint8_t loVer = (uint8_t)((usbVer - (float)hiVer)*10)<<4;

  this->device.len                    = sizeof(device);
  this->device.dtype                  = 0x01;
  this->device.usbVersion             = 0x0110,//(hiVer<<8) | loVer;
  this->device.deviceClass            = 0x00;
  this->device.deviceSubClass         = 0x00;
  this->device.deviceProtocol         = 0x00;
  this->device.packetSize0            = 64;
  this->device.idVendor               = vid;
  this->device.idProduct              = pid;
  this->device.deviceVersion          = 0x0101;
  this->device.iManufacturer          = this->getStrId();
  this->device.iProduct               = this->getStrId();
  this->device.iSerialNumber          = this->getStrId();
  this->device.bNumConfigurations     = 1;
}

void USB::initPLL(){
  PLLFRQ   = 0b01001010;     //Set PLL freq 96MHz and PLL USB divider 2
  PLLCSR  |= (1<<PINDIV);    //Set PLL div for 16MHz
  PLLCSR  |= (1<<PLLE);

  while( (PLLCSR & 0x01) == 0)
    ;
}

void USB::init(){
  cli();

  this->isEnum = false;
  this->initPLL();
  UHWCON   = 0b00000001;
  USBCON  |= (1<<OTGPADE) | (1<<USBE) | (1<<FRZCLK);
  USBCON &= ~(1<<FRZCLK);               // UNFREEZE;

  _delay_ms(1);

  UDCON &= ~(1<<DETACH);                // ATTACH;

  UDIEN = (1<<EORSTE);

  sei();
}

bool USB::isEnumerate(){
  return this->isEnum;
}

uint8_t USB::getStrId(){
  return (this->useStr?++this->stringCounter:0);
}

inline void USB::sof(void){

}

inline void USB::eorst(void){
  this->initControl();  
  UDIEN &= ~(1<<EORSTE);
}

void USB::waitIn(void){
  while (!(UEINTX & (1<<TXINI))) 
    ;
}

void USB::clearIn(void){
  UEINTX &= ~((1<<FIFOCON)|(1<<TXINI)); 
  this->sendBufPos = 0;
}

inline void USB::stall(void){
  UECONX |= (1<<STALLRQ);
}

inline void USB::readBuf(uint8_t *buffer, uint8_t size){
  while(size--){
    *(buffer++) = UEDATX;
  }
}

void USB::writeBuf(volatile const uint8_t *buffer, uint8_t bufSize, uint8_t maxSize){  
volatile  uint8_t buf[50],pos = 0;
this->selectEndPoint(0);
memcpy((void*)&buf,(void*)buffer,bufSize);
  //if( (this->sendBufPos + bufSize) > maxSize ) {
  //  return;
  //}
  volatile uint8_t b;
  //this->clearIn();
  while(bufSize--){    
    b = buf[pos++];//*buffer++;
    UEDATX = b;
    this->sendBufPos++;
  }
}

void USB::initControl(void){
  
  this->initInternalEndpoint(0, CONTROL,OUT,SIZE64,ONE);
  UEIENX = (1 << RXSTPE);

}

void USB::selectEndPoint(uint8_t num){
  UENUM  = ((num) & 0x0F);
}

bool USB::initInternalEndpoint(uint8_t num, upType type, upDirection dir,upSize size, upBank bank){
  this->selectEndPoint(0);  
  UECONX |= (1<<EPEN);

  UECFG1X = 0;
  UECFG0X = (type << EPTYPE0) | (dir);
  UECFG1X = (1<<ALLOC) | (size << 4) | ((bank==TWO)?0b100:0);

  return (UESTA0X & (1<<CFGOK))?true:false;
}

void USB::ControlRequest(){
  this->selectEndPoint(0);
  volatile uint8_t intStatus = UEINTX;
  USB_Request_Header head;
  if( !IS_SET(intStatus,RXSTPI) ){
    return;
  }
  UEIENX &= ~(1<<RXSTPE);     

  this->readBuf((uint8_t*)&head,sizeof(USB_Request_Header));
  UEINTX = ~( (1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI) );
  
  if( (head.bmRequestType & 0x80) == 0x80 ) { this->waitIn(); } else { this->clearIn(); };    

  bool accepted = true;

  switch(head.bRequest){
    case GET_STATUS:      
        accepted = this->getStatus();        
        nop();
      break;
    case CLEAR_FEATURE:
        accepted = this->clearFeature();
      break;
    case SET_FEATURE:
        accepted = this->setFeature();
      break;
    case SET_ADDRESS:
        accepted = this->setAddres();
      break;
    case GET_DESCRIPTOR:
        accepted = this->getDescriptor(&head);
      break;
    case SET_DESCRIPTOR:
        accepted = this->setDescriptor();
        nop();
      break;
    case GET_CONFIGURATION:
        accepted = this->getConfiguration();
        nop();
      break;
    case SET_CONFIGURATION:         
        accepted = this->setConfiguration();
        nop();
      break;
    case GET_INTERFACE:
        accepted = this->getInterface();
      break;
    case SET_INTERFACE:
        accepted = this->setInterface();
      break;
    default:
      accepted = false;
      nop();
  }
  //if( accepted ){
  //  this->clearIn(); 
  //} else {
    this->stall();
  //}  
  
  UEIENX |= (1 << RXSTPE);  
}

void USB::onGenEvent(){
  uint8_t state = UDINT;
  UDINT = 0;
  
  if( IS_SET(state,EORSTI) ){
    this->eorst();
  }

  if( IS_SET(state, SOFE) ){
    this->sof();
  }
  sei();
}

void USB::onComEvent(){  
  uint8_t pos, mask;
  //this->currentEndpoins = UENUM;
  this->selectEndPoint(0);

  if( IS_SET(UEINTX,RXSTPI) ){    
    this->ControlRequest();
    sei();
    return;
  }

  this->clearIn();
  return;

  

  for(pos = 0;pos<7;pos++){
    mask = (1<<pos);
    if( ((UENUM & mask) == mask) && (this->epEvents[pos]!= 0)) {
      this->epEvents[pos]();
    }
  }

  this->selectEndPoint(this->currentEndpoins);
}

bool USB::getStatus(){
  return false;
}

bool USB::clearFeature(){
  return false;
}

bool USB::setFeature(){
  return false;
}

bool USB::setAddres(){
  return false;
}

bool USB::getDescriptor(USB_Request_Header *request){
  switch( request->wValueH ){
    case GET_DESCRIPTOR_DEVICE:
      writeBuf((const uint8_t*)&this->device,sizeof(USB_Device_Descriptor),this->controlHeader.wLengthL);  
      UEINTX &= ~((1<<TXINI));
      nop();
      //this->clearIn();  
    break;
    //case GET_DESCRIPTOR_CONFIG:
      //sendControl((const uint8_t*)&config,  sizeof(config), head.wLengthL);
      //sendControl((const uint8_t*)&iface0,  sizeof(iface0), head.wLengthL);
      //sendControl((const uint8_t*)&ep1,     sizeof(ep1),    head.wLengthL);
      //sendControl((const uint8_t*)&ep2,     sizeof(ep2),    head.wLengthL);
      //sendControl((const uint8_t*)&ep3,     sizeof(ep3),    head.wLengthL);
    //break;
    //case GET_DESCRIPTOR_STRING:
      //switch(head.wValueL){
      //  case 0:
      //    sendControl((const uint8_t*)&lang0,4,head.wLengthL);
      //  break;
      //  default:
      //    getString();
      //}
    //break;
    //case GET_DESCRIPTOR_QUALIFER:
      //sendControl((const uint8_t*)&qal,qal.bLength,head.wLengthL);
    //break;
    default:
      return false;
  }
  return true;
}

bool USB::setDescriptor(){
  return false;
}

bool USB::getConfiguration(){
  return false;
}

bool USB::setConfiguration(){
  return false;
}

bool USB::getInterface(){
  return false;
}

bool USB::setInterface(){
  return false;
}
