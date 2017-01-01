/*
 * usb.h
 *
 * Created: 23.12.2016 22:42:15
 *  Author: Sync
 */ 


#ifndef USB_H_
#define USB_H_

#include <avr/interrupt.h>

extern "C" void USB_GEN_vect(void) __attribute__ ((signal));
extern "C" void USB_COM_vect(void) __attribute__ ((signal));

#define GET_STATUS			  0
#define CLEAR_FEATURE		  1
#define SET_FEATURE			  3
#define SET_ADDRESS			  5
#define GET_DESCRIPTOR		6
#define SET_DESCRIPTOR		7
#define GET_CONFIGURATION	8
#define SET_CONFIGURATION	9
#define GET_INTERFACE		  10
#define SET_INTERFACE		  11

#define GET_DESCRIPTOR_DEVICE     0x01
#define GET_DESCRIPTOR_CONFIG     0x02
#define GET_DESCRIPTOR_STRING     0x03
#define GET_DESCRIPTOR_QUALIFER   0x06

typedef uint8_t (*functptr)();
#pragma pack(0)

struct USB_Request_Header{
  uint8_t bmRequestType;   /**< Type of the request. */
  uint8_t bRequest;        /**< Request command code. */
  uint8_t wValueL;          /**< wValue parameter of the request. */
  uint8_t wValueH;          /**< wValue parameter of the request. */
  uint8_t wIndexL;          /**< wIndex parameter of the request. */
  uint8_t wIndexH;          /**< wIndex parameter of the request. */
  uint8_t wLengthL;         /**< Length of the data to transfer in bytes. */
  uint8_t wLengthH;         /**< Length of the data to transfer in bytes. */
};

struct USB_Device_Descriptor{
  uint8_t   len;
  uint8_t   dtype;
  uint16_t  usbVersion;
  uint8_t	  deviceClass;
  uint8_t	  deviceSubClass;
  uint8_t	  deviceProtocol;
  uint8_t	  packetSize0;
  uint16_t	idVendor;
  uint16_t	idProduct;
  uint16_t	deviceVersion;
  uint8_t	  iManufacturer;
  uint8_t	  iProduct;
  uint8_t	  iSerialNumber;
  uint8_t	  bNumConfigurations;
};

#define nop() do { __asm__ __volatile__ ("nop"); } while (0)

class USB{
  enum upType{CONTROL,ISOCHR,BULK,INT};
  enum upDirection{OUT,IN};
  enum upBank{ONE,TWO};
  enum upSize {SIZE8, SIZE16, SIZE32, SIZE64, SIZE128, SIZE256, SIZE512};

  protected:
    USB_Request_Header controlHeader;
    USB_Device_Descriptor device;
    inline void sof(void);
    inline void eorst(void);    
    void ControlRequest(void);
    bool initInternalEndpoint(uint8_t num, upType type, upDirection dir,upSize size, upBank bank);
    bool getStatus();
    bool clearFeature();
    bool setFeature();
    bool setAddres();
    bool getDescriptor(USB_Request_Header *request);
    bool setDescriptor();
    bool getConfiguration();
    bool setConfiguration();
    bool getInterface();
    bool setInterface();
  private:    
    bool isEnum;
    uint8_t currentEndpoins;
    uint8_t sendBufPos;
    uint8_t stringCounter;
    bool useStr;
    functptr epEvents[7];
    wchar_t *strings[50];
    void initPLL(void);    
    void waitIn(void);
    void clearIn(void);
    void stall(void);
    void selectEndPoint(uint8_t num);
    void readBuf(uint8_t *buffer, uint8_t size);
    void writeBuf(volatile const uint8_t *buffer, uint8_t bufSize, uint8_t maxSize);
    inline uint8_t getStrId();
    void initControl(void);
  public:
    USB(uint16_t vid, uint16_t pid, float usbVer, bool useString);
    void init();
    bool isEnumerate();

    void onComEvent(void);
    void onGenEvent(void);

    friend void USB_COM_vect( void );
    friend void USB_GEN_vect( void );
};




#endif /* USB_H_ */