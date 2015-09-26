#include <SPI.h>
#include <Ethernet.h>
#include <Udp.h>
#include <GSM.h>

#define PINNUMBER "" //Pin Number for the SIM

// Master pin assignments
// AD0: Main battery current
// AD1: Main battery voltage
// AD2,4,6,8: Leaf(i) current
// AD3,5,7,9: Leaf(i) voltage

int leaf_master_current_pin = A0;    // Leaf 1 current
int leaf_one_current_pin = A1;   // Leaf 2 current
int leaf_two_current_pin = A2; // Leaf 3 current
int leaf_three_current_pin = A3; // Leaf 4 current
int master_current_pin = A4; // Current measurement master
int master_voltage_pin = A5; // Voltage measurement master
int leaf_three = 12;      // Leaf3
int ethernet_router = 11; // Neatgear
int leaf_two = 10; // Leaf 2
int leaf_one = 9;  // Leaf 1
int master = 8;  //  Master
boolean leaf_one_on = false;
float leaf_one_balance=0;
float leaf_one_current=0;

//Ethernet setup definitions
byte mac[] = {0x90, 0xa2, 0xda, 0x00, 0x11, 0x07};
byte ip[]  = {10, 10, 5, 123};
const unsigned int localPort = 9631;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE+1]; //buffer to hold incoming packet,
byte remoteIp1[4] = {10, 10, 5, 124};
unsigned int remote_Port1 = 1369;
byte remoteIp2[4] = {10, 10, 5, 125};
unsigned int remote_Port2 = 1370;
byte remoteIp3[4] = {10, 10, 5, 126};
unsigned int remote_Port3 = 1371;
byte         recvdIp[4];
unsigned int recvdPort;

// initialize the GSM Library
GSM gsmAccess; // include a 'true' parameter for debug enabled
GSM_SMS sms;

char remoteNumber[20];  // Holds the emitting number sms received

// Really only needs to have length = 2, since will either send "H" or "L"
char UDPMessageBuffer[80]; 

EthernetUDP Udp;

void setup()
{
   //Setup Ethernet
   Ethernet.begin(mac, ip);
   Udp.begin(localPort);
   
   //Setup SErial Comms
   Serial.begin(9600);
   
   Serial.println(Ethernet.localIP());
   Serial.println(packetBuffer);
   
   // connection state
   boolean notConnected = true;
   
  //Setup ability to send/rcv SMS
  while(notConnected)
  {
    if(gsmAccess.begin(PINNUMBER)==GSM_READY)
      notConnected = false;
    else
    {
      Serial.println("GSM Not connected");
      delay(1000);
    }
  }

  Serial.println("GSM initialized");
  Serial.println("Waiting for messages");   
   
}

void loop(){
   
  char inchar;
  char c;
  int i=0;
  int sms_code;
  int kwh_rate=1; // In Rs per KWH
  int coupon_code[3];

  // READ ALL VOLTAGES AND CURRENTS FIRST

  //pinMode(leaf_one_current_pin,INPUT);
  leaf_one_current=analogRead(leaf_one_current_pin);
  
  // If there are any SMSs available()  
  if (sms.available())
  {
  Serial.println("Message received from:");
  
  // Get remote number
  sms.remoteNumber(remoteNumber, 20);
  Serial.println(remoteNumber);
  
  // This is just an example of message disposal    
  // Messages starting with # should be discarded
  if(sms.peek()=='#')
  {
  Serial.println("Discarded SMS");
  sms.flush();
  }
  
  // Read message bytes and print them
  while(c=sms.read()){
    Serial.println(c);
    coupon_code[i]=int(c-'0');
    Serial.println(coupon_code[i])  ;
    i=i+1;
  }
  
  
  // TODO:sms_code validation goes here
  
  if ((int)coupon_code[0] > 1){
  
    Serial.println("\nVALID MESSAGE");
    leaf_one_on=true;
    leaf_one_balance= (int)coupon_code[0]*1; // set the balance value here
    Serial.print(leaf_one_balance);
  }
  
  else {
    Serial.println("Invalid SMS RECEIVED");
  }
  
  // delete message from modem memory
  sms.flush();
  }


  // READ ALL VOLTAGES AND CURRENTS FIRST
  
  // turn leaf one or off depending on various logic
  if (leaf_one_on){
    pinMode(leaf_one,OUTPUT);
    Serial.println("turning leaf on");
    digitalWrite(leaf_one,HIGH);
  }
  
  else {
    pinMode(leaf_one,OUTPUT);
    Serial.print("turning leaf off");
    digitalWrite(leaf_one,LOW);
  }
  
  //checking for balance
  if (leaf_one_balance >0){ 
    Serial.println(leaf_one_current);
    leaf_one_balance= leaf_one_balance - (float)(leaf_one_current*9*kwh_rate)/(60*60);  //TODO: replace with accurate power logic  
    Serial.println(leaf_one_balance);
  }
  
  else {  
    Serial.println("LEAF OUT OF BALANCE");
    leaf_one_on=false;  
  }
  
  
  // TODO: ethernet communication to prevent theft, and data transmission
  
  
   if (Serial.available() > 0) 
   {
   inchar = Serial.read();
   if(inchar == 'P')
   {
     Serial.println("Sending Ping to Leaf1");
     strcpy(UDPMessageBuffer, "P");
     Udp.beginPacket(remoteIp1, remote_Port1);
     Udp.write(UDPMessageBuffer);
     Udp.endPacket();
   }
   else if(inchar == 'Q')
   {
     Serial.println("Sending Ping to Leaf2");
     strcpy(UDPMessageBuffer, "Q");
     Udp.beginPacket(remoteIp2, remote_Port2);
     Udp.write(UDPMessageBuffer);
     Udp.endPacket();
   }
   else if(inchar == 'R')
   {
     Serial.println("Sending Ping to Leaf3");
     strcpy(UDPMessageBuffer, "R");
     Udp.beginPacket(remoteIp3, remote_Port3);
     Udp.write(UDPMessageBuffer);
     Udp.endPacket();
   }
   }
   
   int packetSize = Udp.parsePacket(); // note that this includes the UDP header
   if(packetSize) {
  // read the packet into packetBufffer
   Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
   Serial.println("Contents:");
   Serial.println(packetBuffer);
   }

   delay(1000);

}

