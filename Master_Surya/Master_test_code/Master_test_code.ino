#include <SPI.h>
#include <Ethernet.h>
#include <Udp.h>
#include <GSM.h>
#include <SD.h>


#define PINNUMBER "" //Pin Number for the SIM
#define LOW_BATTERY_VAL 140

int leaf_master_current_pin = A0;    // Leaf 1 current
int leaf_one_current_pin = A1;   // Leaf 2 current
int leaf_two_current_pin = A2; // Leaf 3 current
int leaf_three_current_pin = A3; // Leaf 4 current
int master_current_pin = A4; // Current measurement master
int master_voltage_pin = A5; // Voltage measurement master
int leaf_three = 9;      // Leaf3
int ethernet_router = 8; // Neatgear
int leaf_two = 7; // Leaf 2
int leaf_one = 6;  // Leaf 1
int leaf_master = 5;  //  Master
int shield_init_led=22; // LED FOR GSM SHIELD
int master_balance_led=24; //LED TO SHOW THAT MASTER HAS BALANCE
boolean leaf_one_on = false;
boolean leaf_two_on = false;
boolean leaf_three_on = false;
boolean leaf_master_on=false;
boolean complete_reset_needed=false;
boolean exit_flag=false;
boolean low_battery_warning;

// Variables for Balance
float leaf_one_balance=0;
float leaf_two_balance=0;
float leaf_three_balance=0;
float leaf_master_balance=0;

unsigned long start_time = 0;
unsigned elapsed_time = 0;

// Variables for current measured from 
float leaf_master_current=0;
float leaf_one_current=0;
float leaf_two_current=0;
float leaf_three_current=0;

// Variables for master current and voltage
float master_current=0;
float master_voltage=0;

float kwh_rate=140.0; // In Rs per KWH
int coupon_code[7]; // [balance_1][balance_2][balance_3][leaf_number][validity_1][validity_2][validity_3]
int software_clock=0;

//order of sms
//v1 v2 v3 l b1 b1 b2

File used_numbers_database;
File leaf_master_balance_file;
File leaf_one_balance_file;
File leaf_two_balance_file;
File leaf_three_balance_file;

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
boolean file_already_present=false;

// initialize the GSM Library
GSM gsmAccess; // include a 'true' parameter for debug enabled
GSM_SMS sms;

char remoteNumber[20];  // Holds the emitting number sms received

// Really only needs to have length = 2, since will either send "H" or "L"
char UDPMessageBuffer[80]; 

EthernetUDP Udp;

void blink_warning_lights(){

  while(true){
    pinMode(shield_init_led,HIGH);
    pinMode(master_balance_led,HIGH);
    
    digitalWrite(shield_init_led,HIGH); // LED FOR GSM SHIELD
    digitalWrite(master_balance_led,HIGH);

    delay(1000);

    digitalWrite(shield_init_led,LOW); // LED FOR GSM SHIELD
    digitalWrite(master_balance_led,LOW);

    delay(1000);
  } 
}

void turn_off_all_leaves(){
  
  digitalWrite(leaf_one,LOW);
  digitalWrite(leaf_two,LOW);
  digitalWrite(leaf_three,LOW);
  digitalWrite(leaf_master,LOW);
  Serial.println("Turn off all leaves");
  
}

void check_for_low_battery() {

  Serial.println("Master voltage is : " +String(master_voltage));
  if (master_voltage <= LOW_BATTERY_VAL){

    low_battery_warning = true ;
  }
  
}

void remove_all_files_from_SD(){
  if(SD.remove("leaf1b.txt")){
    Serial.println("Leaf 1 balance file removed");
    }
  if(SD.remove("leaf2b.txt")){
     Serial.println("Leaf 2 balance file removed");
    }
  if(SD.remove("leaf3b.txt")){
    Serial.println("Leaf 3 balance file removed");
  }
  if(SD.remove("leafmb.txt")){
      Serial.println("Master leaf balance file removed");
  }
  if(SD.remove("used.txt")){
     Serial.println("Used Codes");
  }
  }


void initialize_balance_files_to_zero(){
   leaf_one_balance_file = SD.open("leaf1b.txt", FILE_WRITE);
  if(leaf_one_balance_file){
    leaf_one_balance_file.println("000"); //initialize balance to 0 in SD CARD
    Serial.println("Leaf 1 balance file init'ed to 0");
    leaf_one_balance_file.close();
  }
  // this text file contains balance of leaf two
  leaf_two_balance_file = SD.open("leaf2b.txt", FILE_WRITE);
  if(leaf_two_balance_file){
    leaf_two_balance_file.println("000"); //initialize balance to 0 in SD CARD
    Serial.println("Leaf 2 balance file init'ed to 0");
    leaf_two_balance_file.close();
  }
  // this text file contains balance of leaf three

  leaf_three_balance_file = SD.open("leaf3b.txt", FILE_WRITE);
  if(leaf_three_balance_file){
    leaf_three_balance_file.println("000"); //initialize balance to 0 in SD CARD
    Serial.println("Leaf 3 balance file init'ed to 0");
    leaf_three_balance_file.close();
  }
  
  leaf_master_balance_file = SD.open("leafmb.txt", FILE_WRITE);
  if(leaf_master_balance_file){
    Serial.println("File for Master balance succesfully opened");
    leaf_master_balance_file.println("000"); //initialize balance to 0 in SD CARD
    Serial.println("Leaf 4 balance file init'ed to 0");
    leaf_master_balance_file.close();
  }

  used_numbers_database = SD.open("used.txt", FILE_WRITE);
  if (used_numbers_database) {
    used_numbers_database.println("START OF FILE");
   Serial.println("used init'ed with START OF FILE");
    used_numbers_database.close();
  }
    
  }

void close_all_files(){

 leaf_one_balance_file = SD.open("leaf1b.txt", FILE_WRITE);
  if(leaf_one_balance_file){
    leaf_one_balance_file.close();
    Serial.println("Leaf one file closed");
    
  }
  // this text file contains balance of leaf two
  leaf_two_balance_file = SD.open("leaf2b.txt", FILE_WRITE);
  if(leaf_two_balance_file){
    leaf_three_balance_file.close();
    Serial.println("Leaf two file closed");
    
  }
  // this text file contains balance of leaf three

  leaf_three_balance_file = SD.open("leaf3b.txt", FILE_WRITE);
  if(leaf_three_balance_file){
    leaf_three_balance_file.close();
    Serial.println("Leaf three file closed");
    
  }
  
  leaf_master_balance_file = SD.open("leafmb.txt", FILE_WRITE);
  if(leaf_master_balance_file){
    leaf_master_balance_file.close();
    Serial.println("Leaf master file closed");
    
  }

 used_numbers_database = SD.open("used.txt", FILE_WRITE);
  if(used_numbers_database){
    used_numbers_database.close();
    Serial.println("Useful numbers database closed");
    
  }

  
}

void setup()
{
   //Setup Ethernet
   Ethernet.begin(mac, ip);
   Udp.begin(localPort);
   
   //Setup SErial Comms
   Serial.begin(9600);
   
   Serial.println(Ethernet.localIP());
   Serial.println(packetBuffer);

   //Initialize SD  CARD
   pinMode(53,OUTPUT);
   digitalWrite(53, HIGH); 
   if (!SD.begin(4)) {
    Serial.println("SD CARD Initialization failed!");
    return;
    }
  Serial.println("SD CARD Initialization done.");

  
  if(complete_reset_needed){
    remove_all_files_from_SD();
    initialize_balance_files_to_zero();
  
  }

  else{

    close_all_files();
  }
   // connection state initialize to false
  boolean notConnected = true;

  // Turn off GSM init LED
  pinMode(shield_init_led,OUTPUT);
  digitalWrite(shield_init_led,LOW); 
  
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

  //turn GSM INIT LED ON
  pinMode(shield_init_led,OUTPUT);
  digitalWrite(shield_init_led,HIGH); 

  if(complete_reset_needed){
    turn_off_all_leaves();
    Serial.println("Turn off all leaves");
  }

     
}

boolean check_if_file_present(){
  
  String inputString = "";
  char inputChar;
  int stringIndex = 0; // String stringIndexing int;
  String sms_string_temp="";
  int counter=0;

  Serial.println("Checking if code has been used before");
  while(true){

    sms_string_temp=sms_string_temp+String(coupon_code[counter]);
    Serial.println(sms_string_temp);
    if(counter==6){
      break;
    }
    counter=counter+1;
    
  }
  used_numbers_database=SD.open("used.txt",FILE_READ);
  while(used_numbers_database.available()){

  inputChar = used_numbers_database.read(); // Gets one byte from serial buffer
  
    if (inputChar != '\n') // define breaking char here (\n isnt working for some reason, i will follow this up later)
    {
      inputString= inputString + String(inputChar);
    }
    else  
    { 
      if ((inputString.substring(0,6)).equals(sms_string_temp.substring(0,6))){
        Serial.println("Code already used");
          used_numbers_database.close();
          return true;
        
      }

      inputString="";
    }
  }

  Serial.println("Code not already used");
  used_numbers_database.close();
  return false;
}

void update_balance_from_sd_card(){

  String inputString_4="";
  String temp_str="";
  char inputChar;
  int counter=0;
  int len=0;

  Serial.println("Updating Balances from SD Card");

  leaf_master_balance_file=SD.open("leafmb.txt",FILE_READ);
  while(leaf_master_balance_file.available()){

  inputChar = leaf_master_balance_file.read(); // Gets one byte from serial buffer
  
    if (inputChar != '\n') // define breaking char here (\n isnt working for some reason, i will follow this up later)
    {
      inputString_4= inputString_4 + String(inputChar);
    }
    else{
      len=inputString_4.length();
      temp_str=inputString_4.substring(0,len-1);
      leaf_master_balance=(temp_str.toInt())/1000000.0;
      inputString_4 = "";
      len=0;
    }   

}
    leaf_master_balance_file.close();
    SD.remove("leafmb.txt");
    if(!SD.exists("leafmb.txt")){
      Serial.println("Leaf Master File Removed");
    }


  leaf_one_balance_file=SD.open("leaf1b.txt",FILE_READ);
  while(leaf_one_balance_file.available()){

  inputChar = leaf_one_balance_file.read(); // Gets one byte from serial buffer
  
    if (inputChar != '\n') // define breaking char here (\n isnt working for some reason, i will follow this up later)
    {
      inputString_4= inputString_4 + String(inputChar);
    }
    else{
      len=inputString_4.length();
      temp_str=inputString_4.substring(0,len-1);
      leaf_one_balance=(temp_str.toInt())/1000000.0;
      inputString_4 = "";
      len=0;
    }   

}
    leaf_one_balance_file.close();
    SD.remove("leaf1b.txt");
    if(!SD.exists("leaf1b.txt")){
      Serial.println("Leaf One File Removed");
    }

  leaf_two_balance_file=SD.open("leaf2b.txt",FILE_READ);
  while(leaf_two_balance_file.available()){

  inputChar = leaf_two_balance_file.read(); // Gets one byte from serial buffer
  
    if (inputChar != '\n') // define breaking char here (\n isnt working for some reason, i will follow this up later)
    {
      inputString_4= inputString_4 + String(inputChar);
    }
    else{
      len=inputString_4.length();
      temp_str=inputString_4.substring(0,len-1);
      leaf_two_balance=(temp_str.toInt())/1000000.0;
      inputString_4 = "";
      len=0;
    }   

}
    leaf_two_balance_file.close();
    SD.remove("leaf2b.txt");
    if(!SD.exists("leaf2b.txt")){
      Serial.println("Leaf Two File Removed");
    }


  leaf_three_balance_file=SD.open("leaf3b.txt",FILE_READ);
  while(leaf_three_balance_file.available()){

  inputChar = leaf_three_balance_file.read(); // Gets one byte from serial buffer
  
    if (inputChar != '\n') // define breaking char here (\n isnt working for some reason, i will follow this up later)
    {
      inputString_4= inputString_4 + String(inputChar);
    }
    else{
      len=inputString_4.length();
      temp_str=inputString_4.substring(0,len-1);
      leaf_three_balance=(temp_str.toInt())/1000000.0;
      inputString_4 = "";
      len=0;
    }   

}
    leaf_three_balance_file.close();
    SD.remove("leaf3b.txt");
    if(!SD.exists("leaf3b.txt")){
      Serial.println("Leaf three balance file Removed");
    }

    Serial.println("BALANCES AFTER BEING UPDATED FROM SD ARE :");
    Serial.println(leaf_one_balance,6);
    Serial.println(leaf_two_balance,6);
    Serial.println(leaf_three_balance,6);
    Serial.println(leaf_master_balance,6);
    
}


void update_latest_balance_to_sd_card(){

  Serial.println("Writing balance to SD card");
  leaf_one_balance_file = SD.open("leaf1b.txt", FILE_WRITE);
   if(leaf_one_balance_file){
    leaf_one_balance_file.println(String(leaf_one_balance*1000000.0,6));
    Serial.println("Writing to leaf one balance file");
    leaf_one_balance_file.close();
  }

  leaf_two_balance_file = SD.open("leaf2b.txt", FILE_WRITE);
   if(leaf_two_balance_file){
    leaf_two_balance_file.println(String(leaf_two_balance*1000000.0,6));
    Serial.println("Writing to leaf two balance file");
    leaf_two_balance_file.close();
  }

  leaf_three_balance_file = SD.open("leaf3b.txt", FILE_WRITE);
   if(leaf_three_balance_file){
    leaf_three_balance_file.println(String(leaf_three_balance*1000000.0,6)); ////POSSIBLE OVERFLOW
    Serial.println("Writing to leaf three balance file");
    leaf_three_balance_file.close();
  }

  leaf_master_balance_file = SD.open("leafmb.txt", FILE_WRITE);
   if(leaf_master_balance_file){
    leaf_master_balance_file.println(String(leaf_master_balance*1000000.0,6));
    Serial.println("Writing to leaf master balance file");
    leaf_master_balance_file.close();
  }
  
}

void read_voltages_and_currents(){

  leaf_one_current=analogRead(leaf_one_current_pin);
  leaf_two_current=analogRead(leaf_two_current_pin);
  leaf_three_current=analogRead(leaf_three_current_pin);
  leaf_master_current=analogRead(leaf_master_current_pin);
  master_current=analogRead(master_current_pin);
  master_voltage=analogRead(master_voltage_pin);

 
}

void update_leaf_states(){
  pinMode(leaf_one,OUTPUT);
  pinMode(leaf_two,OUTPUT);
  pinMode(leaf_three,OUTPUT);
  pinMode(leaf_master,OUTPUT);
  pinMode(master_balance_led,OUTPUT);
 
 if (leaf_one_on){
    Serial.println("turning leaf one on");
    digitalWrite(leaf_one,HIGH);
  }
  
  else {
    Serial.println("turning leaf 1 off");
    digitalWrite(leaf_one,LOW);
  }
  
  if (leaf_two_on){
    Serial.println("turning leaf 2 on");
    digitalWrite(leaf_two,HIGH);
  }
  
  else {
    Serial.println("turning leaf 2 off");
    digitalWrite(leaf_two,LOW);
  }
  
  if (leaf_three_on){
    Serial.println("turning leaf 3 on");
    digitalWrite(leaf_three,HIGH);
  }
  
  else {
    Serial.println("turning leaf 3 off");
    digitalWrite(leaf_three,LOW);
  }
  
  if (leaf_master_on){
    Serial.println("turning master leaf on");
    digitalWrite(leaf_master,HIGH);
    digitalWrite(master_balance_led,HIGH); 
  }
  
  else {
    Serial.println("turning master leaf off");
    digitalWrite(leaf_master,LOW);

    // Turn off the LED As well so he knows hes on
    digitalWrite(master_balance_led,LOW);   
  }
  
}

void soft_reset(){
  asm volatile ("  jmp 0");  
}

void update_leaf_balances(){

  float leaf_master_actual_current = 0;
  float leaf_one_actual_current = 0;
  float leaf_two_actual_current = 0;
  float leaf_three_actual_current = 0;
  
   if (leaf_one_balance >0){ 
    leaf_one_on=true; // turn leaf one on
    if(leaf_one_current>20){
      leaf_one_actual_current=(leaf_one_current*4+200)/1000;
    }
    else{
      leaf_one_actual_current=0;
    }
    Serial.println("Leaf one is on consuming" + String(leaf_one_current));
    leaf_one_balance= leaf_one_balance - (float)(leaf_one_actual_current*12*kwh_rate/1000.0)*(elapsed_time/(60.0*60.0*1000.0));  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    if (leaf_one_balance < 0){
      leaf_one_balance=0;
    }
    Serial.println("Leaf one is on with remaining balance: " + String(leaf_one_balance));
  }
  
  else {  
    Serial.println("LEAF 1 OUT OF BALANCE");
    leaf_one_on=false;  
  }
  
  if (leaf_two_balance >0){ 
    leaf_two_on=true; 
    if(leaf_two_current>35){
      leaf_two_actual_current=(leaf_two_current*4+200)/1000;
    }
    else{
      leaf_two_actual_current=0;
    }// turn leaf one on
    Serial.println("Leaf two is on consuming" + String(leaf_two_current));
    leaf_two_balance= leaf_two_balance - (float)(leaf_two_actual_current*12*kwh_rate/1000.0)*(elapsed_time/(60.0*60.0*1000.0));
     if (leaf_two_balance < 0){
      leaf_two_balance=0;
    }
    // update balance, based on power consumption (TODO: replace with accurate power logic) 
    Serial.println("Leaf two is on with remaining balance: " + String(leaf_two_balance));
  }
  
  else {  
    Serial.println("LEAF 2 OUT OF BALANCE");
    leaf_two_on=false;  
  }
  
   if (leaf_three_balance >0){ 
    leaf_three_on=true; 
    if(leaf_three_current>20){
      leaf_three_actual_current=(leaf_three_current*4+200)/1000;
    }
    else{
      leaf_three_actual_current=0;
    }// turn leaf one on
    Serial.println("Leaf three is on consuming" + String(leaf_three_current));
    leaf_three_balance= leaf_three_balance - (float)(leaf_three_actual_current*12*kwh_rate/1000.0)*(elapsed_time/(60.0*60.0*1000.0));  // update balance, based on power consumption (TODO: replace with accurate power logic) 
    if (leaf_three_balance <0){
      leaf_three_balance=0;
    }
    Serial.println("Leaf three is on with remaining balance: " + String(leaf_three_balance));
  }
  
  else {  
    Serial.println("LEAF 3 OUT OF BALANCE");
    leaf_three_on=false;  
  }
  
   if (leaf_master_balance >0){ 
    leaf_master_on=true; // turn leaf one on
    if(leaf_master_current>20){
      leaf_master_actual_current=(leaf_master_current*4+200)/1000;
    }
    else{
      leaf_master_actual_current=0;
    }
    Serial.println("Leaf master is on consuming " + String(leaf_master_actual_current));
    Serial.println((float)((leaf_master_actual_current*12.0*kwh_rate/1000.0)));
    leaf_master_balance= leaf_master_balance - (float)((leaf_master_actual_current*12.0*kwh_rate/1000.0)*(elapsed_time)/(60.0*60.0*1000.0)); // update balance, based on power consumption (TODO: replace with accurate power logic) 
    if (leaf_master_balance <0){
      leaf_master_balance=0;
    }
    Serial.println(leaf_master_balance,6);
    Serial.println("Leaf master is on with remaining balance: " + String(leaf_master_balance));
  }
  
  else {  
    Serial.println("MASTER LEAF OUT OF BALANCE");
    leaf_master_on=false;  
  }
  
}


void receive_sms_and_verify(){

  //local variables 
  char inchar;
  char c;
  int i=0;
  int sms_code;
  String sms_string;
  
  // variables for coupon code decoding
  
  int balance_1=0;
  int balance_2=0;
  int balance_3=0;
  int leaf_number=0;
  int validity_1=0;
  int validity_2=0;
  int validity_3=0;  
  int counter_new=0;

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
    i=i+1;
    if(i>6)
    {break;}
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

  
  // TODO:sms_code validation goes here
  
  file_already_present= check_if_file_present();
  if (validation_number > 100 && !(file_already_present)){
  
    Serial.println("\nVALID MESSAGE");

    /**
     * write valid code to SD CARD to prevent repeat usage
     */

    while(true){
      Serial.println(String(coupon_code[counter_new]));
      sms_string=sms_string+String(coupon_code[counter_new]);
      //Serial.println(sms_string);
      if(counter_new==6){
        break;
    }
      counter_new=counter_new+1;
    }

    used_numbers_database = SD.open("used.txt", FILE_WRITE);
    if(used_numbers_database){
      used_numbers_database.println(sms_string);
      used_numbers_database.close();
      }

      /*
       * Update balances based on valid message received
       */

    switch (leaf_number) {
        case 1:
          leaf_one_balance= leaf_one_balance + balance_3*100 + balance_2*10 + balance_1*1;
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
    
  }
  
  else if (coupon_code[0]==0 && coupon_code[1]==0 && coupon_code[2]==0 && coupon_code[3]==0 && coupon_code[4]==0 && coupon_code[5] == 0 && coupon_code[6]== 1){

    exit_flag=true;
  }
  
  else{
    Serial.println("Invalid SMS RECEIVED");
  }
  
  // delete message from modem memory
  sms.flush();
  }
  

  
}


void loop(){
char inchar;

  // READ ALL VOLTAGES AND CURRENTS FIRST

  start_time=millis();
  read_voltages_and_currents();

  
 // UPDATE BALANCES FROM SD CARD
  update_balance_from_sd_card();

 // RECIEVE SMS AND VERIFY
  receive_sms_and_verify();
  update_leaf_balances();
  update_leaf_states();
  unsigned long new_time = millis();
  elapsed_time=new_time-start_time;
  
  update_latest_balance_to_sd_card();

  if (exit_flag){
    turn_off_all_leaves();
    blink_warning_lights();
  }

 check_for_low_battery();

  if (low_battery_warning){
    turn_off_all_leaves();
    blink_warning_lights();
  }
  // TODO: ethernet communication to prevent theft, and data transmission

  if (software_clock > 20){
    delay(100);
    
    soft_reset();
    digitalWrite(shield_init_led,LOW); 
  }

  software_clock=software_clock+1;
  
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

