#include <SPI.h>
#include <Ethernet.h>
#include <Udp.h>
#include <GSM.h>

#define PINNUMBER "" //Pin Number for the SIM

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
int leaf_master = 8;  //  Master
boolean leaf_one_on = false;
boolean leaf_two_on = false;
boolean leaf_three_on = false;
boolean leaf_master_on=false;

// Variables for Balance
float leaf_one_balance=0;
float leaf_two_balance=0;
float leaf_three_balance=0;
float leaf_master_balance=0;

// Variables for current measured from 
float leaf_master_current=0;
float leaf_one_current=0;
float leaf_two_current=0;
float leaf_three_current=0;

// Variables for master current and voltage
float master_current=0;
float master_voltage=0;

int kwh_rate=1; // In Rs per KWH
int coupon_code[7]; // [balance_1][balance_2][balance_3][leaf_number][validity_1][validity_2][validity_3]

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
  
  //local variables 
  char inchar;
  char c;
  int i=0;
  int sms_code;
  
  // variables for coupon code decoding
  
  int balance_1=0;
  int balance_2=0;
  int balance_3=0;
  
  int leaf_number=0;
  
  int validity_1=0;
  int validity_2=0;
  int validity_3=0;  


  // READ ALL VOLTAGES AND CURRENTS FIRST
  leaf_one_current=analogRead(leaf_one_current_pin);
  leaf_two_current=analogRead(leaf_two_current_pin);
  leaf_three_current=analogRead(leaf_three_current_pin);
  leaf_master_current=analogRead(leaf_master_current_pin);
  master_current=analogRead(master_current_pin);
  master_voltage=analogRead(master_voltage_pin);
  
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
  
  // Read message bytes and store in coupon code
  while(c=sms.read()){
    coupon_code[i]=int(c-'0');
    Serial.println(coupon_code[i]);
    i=i+1;
  }
  
  //decode the coupon code just received
  
  balance_1 = coupon_code[6];
  balance_2 = coupon_code[5];
  balance_3 = coupon_code[4];
  
  leaf_number = coupon_code[3];
  
  validity_1 = coupon_code[2];
  validity_2 = coupon_code[1];
  validity_3 = coupon_code[0];  
 
  int validation_number = validity_3 * 100 + validity_2 *10 + validity_1; 
  Serial.println(validation_number);
  Serial.println(validation_number);
  Serial.println(validation_number); 

  
  // TODO:sms_code validation goes here
  
  if (validation_number > 100){
  
    Serial.println("\nVALID MESSAGE");
    Serial.println(leaf_number);
      switch (leaf_number) {
          case 1:
            leaf_one_balance= leaf_one_balance + balance_3*100 + balance_2*10 + balance_1*1;
            Serial.println(leaf_one_balance);
            break;
          case 2:
            leaf_two_balance= leaf_two_balance + balance_3*100 + balance_2*10 + balance_1*1;
            break;
          case 3:
            leaf_three_balance= leaf_three_balance + balance_3*100 + balance_2*10 + balance_1*1;
            break;
          case 4:
            leaf_master_balance= leaf_master_balance + balance_3*100 + balance_2*10 + balance_1*1;
            break;
          default: 
            Serial.println("Not a valid leaf number");
          break;
        }
    
   
    Serial.println(leaf_one_balance);
    Serial.println(leaf_two_balance);
    Serial.println(leaf_three_balance);
    Serial.println(leaf_master_balance);
  }
  
  else {
    Serial.println("Invalid SMS RECEIVED");
  }
  
  // delete message from modem memory
  sms.flush();
  }

  
  /***
  
  Turn leafs on or off depending on their individual bool switchges
  
  ***/
  
  if (leaf_one_on){
    pinMode(leaf_one,OUTPUT);
    Serial.println("turning leaf oneon");
    digitalWrite(leaf_one,HIGH);
  }
  
  else {
    pinMode(leaf_one,OUTPUT);
    Serial.println("turning leaf 1 off");
    digitalWrite(leaf_one,LOW);
  }
  
  if (leaf_two_on){
    pinMode(leaf_two,OUTPUT);
    Serial.println("turning leaf 2 on");
    digitalWrite(leaf_two,HIGH);
  }
  
  else {
    pinMode(leaf_two,OUTPUT);
    Serial.println("turning leaf 2 off");
    digitalWrite(leaf_two,LOW);
  }
  
  if (leaf_three_on){
    pinMode(leaf_three,OUTPUT);
    Serial.println("turning leaf 3 on");
    digitalWrite(leaf_three,HIGH);
  }
  
  else {
    pinMode(leaf_three,OUTPUT);
    Serial.println("turning leaf 3 off");
    digitalWrite(leaf_three,LOW);
  }
  
  if (leaf_master_on){
    pinMode(leaf_master,OUTPUT);
    Serial.println("turning master leaf on");
    digitalWrite(leaf_master,HIGH);
  }
  
  else {
    pinMode(leaf_master,OUTPUT);
    Serial.println("turning master leaf off");
    digitalWrite(leaf_master,LOW);
  }
  
 /***
 Balance updating, and turning leafs off if they run out of balance happens in the next section
 ***/
  
  //checking for balance
  if (leaf_one_balance >0){ 
    leaf_one_on=true; // turn leaf one on
    Serial.println(leaf_one_current);
    leaf_one_balance= leaf_one_balance - (float)(leaf_one_current*9*kwh_rate)/(60*60);  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    Serial.println(leaf_one_balance);
  }
  
  else {  
    Serial.println("LEAF 1 OUT OF BALANCE");
    leaf_one_on=false;  
  }
  
   if (leaf_two_balance >0){ 
    leaf_two_on=true; // turn leaf one on
    Serial.println(leaf_two_current);
    leaf_two_balance= leaf_two_balance - (float)(leaf_two_current*9*kwh_rate)/(60*60);  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    Serial.println(leaf_two_balance);
  }
  
  else {  
    Serial.println("LEAF 2 OUT OF BALANCE");
    leaf_two_on=false;  
  }
  
   if (leaf_three_balance >0){ 
    leaf_three_on=true; // turn leaf one on
    Serial.println(leaf_three_current);
    leaf_three_balance= leaf_three_balance - (float)(leaf_three_current*9*kwh_rate)/(60*60);  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    Serial.println(leaf_three_balance);
  }
  
  else {  
    Serial.println("LEAF 3 OUT OF BALANCE");
    leaf_three_on=false;  
  }
  
   if (leaf_master_balance >0){ 
    leaf_master_on=true; // turn leaf one on
    Serial.println(leaf_master_current);
    leaf_master_balance= leaf_master_balance - (float)(leaf_master_current*9*kwh_rate)/(60*60);  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    Serial.println(leaf_master_balance);
  }
  
  else {  
    Serial.println("MASTER LEAF OUT OF BALANCE");
    leaf_master_on=false;  
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

