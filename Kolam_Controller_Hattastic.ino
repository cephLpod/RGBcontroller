/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <bluefruit.h>
#include <string.h>

#define PACKET_COLOR_LEN 6
#define IN_RED  A0
#define IN_GREEN  A1
#define IN_BLUE  A2


int redValue = 0;  // variable to store the value coming from the sensor
int greenValue = 0; 
int blueValue = 0; 

char strRed[21]  = "0xC770000";
char strBlue[21] = "0xC000077";
// uint8_t hatName[10] = "Hattastic";



/* For a list of EIR data types see:
 *    https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 *    Matching enum: cores/nRF5/SDK/components/softdevice/s132/headers/ble_gap.h */


int connFlag = 0; // Flag to tell Central to keep connected instead of continuing scan

// BLEClientDis  clientDis;  // device information client
// BLEClientUart clientUart; // bleuart client
// BLEUart bleuart;
// // OTA DFU service

BLEDfu bledfu;
BLEClientDis  clientDis;  // device information client
BLEClientUart clientUart; // bleuart client
// Uart over BLE service
BLEUart bleuart;

void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 Central ADV Scan Example");
  Serial.println("------------------------------------\n");

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  /* Set the device name */
  Bluefruit.setName("RGBcontroller");

  /* Set the LED interval for blinky pattern on BLUE LED */
  Bluefruit.setConnLedInterval(250);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

// Configure DIS client
  clientDis.begin();
  
  // Configure and start the BLE Uart service
  bleuart.begin();

  // Set up and start advertising
  startAdv();
  


  // Configure DIS client
  // clientDis.begin();

  // Init BLE Central Uart Serivce
  clientUart.begin();
  // clientUart.setRxCallback(bleuart_rx_callback);


  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Filter out packet with a min rssi
   * - Interval = 100 ms, window = 50 ms
   * - Use active scan (used to retrieve the optional scan response adv packet)
   * - Start(0) = will scan forever since no timeout is given
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);
  Bluefruit.Scanner.filterUuid(BLEUART_UUID_SERVICE); // only invoke callback if detect bleuart service
  Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);        // Request scan response data
  Bluefruit.Scanner.start(0);                   // 0 = Don't stop scanning after n seconds

  Serial.println("Scanning ...");
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(clientDis);
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}


void scan_callback(ble_gap_evt_adv_report_t* report)
{
  PRINT_LOCATION();
  uint8_t len = 0;
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));
  
  /* Display the timestamp and device address */
  if (report->type.scan_response)
  {
    Serial.printf("[SR%10d] Packet received from ", millis());
  }
  else
  {
    Serial.printf("[ADV%9d] Packet received from ", millis());
  }
  // MAC is in little endian --> print reverse
  Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  Serial.print("\n");

  /* Raw buffer contents */
  Serial.printf("%14s %d bytes\n", "PAYLOAD", report->data.len);
  if (report->data.len)
  {
    Serial.printf("%15s", " ");
    Serial.printBuffer(report->data.p_data, report->data.len, '-');
    Serial.println();
  }

  /* RSSI value */
  Serial.printf("%14s %d dBm\n", "RSSI", report->rssi);

  /* Adv Type */
  Serial.printf("%14s ", "ADV TYPE");
  if ( report->type.connectable ) 
  {
    Serial.print("Connectable ");
  }else
  {
    Serial.print("Non-connectable ");
  }
  
  if ( report->type.directed )
  {
    Serial.println("directed");
  }else
  {
    Serial.println("undirected");
  }

  /* Shortened Local Name */
  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, buffer, sizeof(buffer)))
  {
    Serial.printf("%14s %s\n", "SHORT NAME", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  /* Complete Local Name */
  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer)))
  {
    Serial.printf("%14s %s\n", "COMPLETE NAME", buffer);
    // Serial.printf("%14s %s\n", buffer);

    if(buffer[0] = 'H')
    { 
      Serial.println("H found!");
      Bluefruit.Scanner.resume();
    }
    else
    {
      // For Softdevice v6: after received a report, scanner will be paused
      // We need to call Scanner resume() to continue scanning
      // Resume scanning if H is not found
      Bluefruit.Scanner.resume();
    }
    

  }


 /* TX Power Level */
 if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_TX_POWER_LEVEL, buffer, sizeof(buffer)))
 {
   Serial.printf("%14s %i\n", "TX PWR LEVEL", buffer[0]);
   memset(buffer, 0, sizeof(buffer));
 }

 /* Check for UUID16 Complete List */
 len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, buffer, sizeof(buffer));
 if ( len )
 {
   printUuid16List(buffer, len);
 }

 /* Check for UUID16 More Available List */
 len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, buffer, sizeof(buffer));
 if ( len )
 {
   printUuid16List(buffer, len);
 }

 /* Check for UUID128 Complete List */
 len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, buffer, sizeof(buffer));
 if ( len )
 {
   printUuid128List(buffer, len);
 }

 /* Check for UUID128 More Available List */
 len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE, buffer, sizeof(buffer));
 if ( len )
 {
   printUuid128List(buffer, len);
 }  

 /* Check for BLE UART UUID */
 if ( Bluefruit.Scanner.checkReportForUuid(report, BLEUART_UUID_SERVICE) )
 {
   Serial.printf("%14s %s\n", "BLE UART", "UUID Found!");
 }

 /* Check for DIS UUID */
 if ( Bluefruit.Scanner.checkReportForUuid(report, UUID16_SVC_DEVICE_INFORMATION) )
 {
   Serial.printf("%14s %s\n", "DIS", "UUID Found!");
 }

 /* Check for Manufacturer Specific Data */
 len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer, sizeof(buffer));
 if (len)
 {
   Serial.printf("%14s ", "MAN SPEC DATA");
   Serial.printBuffer(buffer, len, '-');
   Serial.println();
   memset(buffer, 0, sizeof(buffer));
 }  

  Bluefruit.Central.connect(report);
  Serial.println("Possibly connected");
  
  /* Check for BLE UART UUID */
  if ( Bluefruit.Scanner.checkReportForUuid(report, BLEUART_UUID_SERVICE) )
  {
    Serial.printf("%14s %s\n", "BLE UART", "UUID Found!");
    Serial.println("Enable TXD's notify");
    // clientUart.enableTXD();
  }

  if ( clientUart.discovered() )
  {
    Serial.println("clientUart has been discovered!!");
  }
  else
  {
    Serial.println("No clientUart");
  }



 Serial.println();

//  // For Softdevice v6: after received a report, scanner will be paused
//  // We need to call Scanner resume() to continue scanning
 Bluefruit.Scanner.resume();
}




void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected from callback");

  Serial.print("Dicovering Device Information ... ");
  if ( clientDis.discover(conn_handle) )
  {
    Serial.println("Found it");
    char buffer[32+1];
    
    // read and print out Manufacturer
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getManufacturer(buffer, sizeof(buffer)) )
    {
      Serial.print("Manufacturer: ");
      Serial.println(buffer);
    }

    // read and print out Model Number
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getModel(buffer, sizeof(buffer)) )
    {
      Serial.print("Model: ");
      Serial.println(buffer);
    }

    Serial.println();
  }else
  {
    Serial.println("Found NONE");
  }


  Serial.print("Discovering BLE Uart Service ... ");
  if ( clientUart.discover(conn_handle) )
  {
    Serial.println("Found it");

    Serial.println("Enable TXD's notify");
    clientUart.enableTXD();

    Serial.println("Ready to receive from peripheral");
  }else
  {
    Serial.println("Found NONE");
    
    // disconnect since we couldn't find bleuart service
    Bluefruit.disconnect(conn_handle);
  }  
}


void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

/**
 * Callback invoked when uart received data
 * @param uart_svc Reference object to the service where the data 
 * arrived. In this example it is clientUart
 */
void bleuart_rx_callback(BLEClientUart& uart_svc)
{
  Serial.print("[RX]: ");
  
  while ( uart_svc.available() )
  {
    Serial.print( (char) uart_svc.read() );
  }

  Serial.println();
}




void printUuid16List(uint8_t* buffer, uint8_t len)
{
 Serial.printf("%14s %s", "16-Bit UUID");
 for(int i=0; i<len; i+=2)
 {
   uint16_t uuid16;
   memcpy(&uuid16, buffer+i, 2);
   Serial.printf("%04X ", uuid16);
 }
 Serial.println();
}

void printUuid128List(uint8_t* buffer, uint8_t len)
{
 (void) len;
 Serial.printf("%14s %s", "128-Bit UUID");

 // Print reversed order
 for(int i=0; i<16; i++)
 {
   const char* fm = (i==4 || i==6 || i==8 || i==10) ? "-%02X" : "%02X";
   Serial.printf(fm, buffer[15-i]);
 }

  Serial.println();  
}





void loop() 
{
  Serial.println("I'm in a loop!!");
  delay(2000);
  Serial.println("Loopdy loop!!");

  // read the value from the sensor:
  redValue = analogRead(IN_RED);
  greenValue = analogRead(IN_GREEN);
  blueValue = analogRead(IN_BLUE);

  redValue = redValue * 255/1024;
  greenValue = greenValue * 255/1024;
  blueValue = blueValue * 255/1024;

  
  Serial.print("RED = ");
  Serial.println(redValue);
  Serial.print("GREEN = ");
  Serial.println(greenValue);
  Serial.print("BLUE = ");
  Serial.println(blueValue);

  char redC = char(redValue);
  char greenC = char(greenValue);
  char blueC = char(blueValue);

  char colorstr[20+1] = { 0 };
  // colorstr = char(redC) + char(greenC) + char(blueC);

  char str[20+1] = { 0 };
  // clientUart.print( colorstr );
  clientUart.println( "0xC00424200000000000000");
  
  // bleuart.print( colorstr );
  // delay(2000);
  bleuart.print(str);
  delay(2000);
  bleuart.println("0xC005555");
  // bleuart.println( colorstr );
  delay(2000);

  if (BLEClientUart().discovered() )
  {
    Serial.println("Discovered!!");
    
    BLEClientUart().print( colorstr );
    BLEClientUart().print( "[!C][00][FF][00]00000" );

    BLEClientUart().println( "0xC004242");
    bleuart.println("[!C][00][FF][00]00000");
    bleuart.print( colorstr );


    Serial.println("strRed printing to client Uart");
    delay(1000);
    BLEClientUart().print( strRed);
    delay(1000);
    Serial.println("strBlue printing to client Uart");
    BLEClientUart().print( strBlue );
    delay(1000);
    BLEClientUart().println( "0xC004200");
  }
  else
  {
    Serial.println("No client Uart discovered :( :( ");
    Bluefruit.Scanner.resume();
  }


}
