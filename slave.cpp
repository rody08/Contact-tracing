#include "mbed.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <iostream>

DigitalIn  echo(p11); // sensor echo
DigitalOut trigger(p12); // sensor trigger
DigitalOut myled(LED1); // bluetooth transmit
DigitalOut myled2(LED2); // bluetooth recieve 
Serial rn42(p9, p10); // define serial link to rn42
BusOut led(LED4, LED3, LED2, LED1); // led bus

// Initialize methods
void initialise_connection_slave(void); // bluetooth
void write_to_master(int id, int record); // bluetooth
void read_from_master(void); //bluetooth
char * string_to_char_arr(string emessage); // Bluetooth
void send_data(char * arr, int arrS); // Bluetooth
void encryption_driver(int attribute, int key); // Encryption
string encryptString(string message, int changeBy); // Encryption
string decryptString(string message, int changeBy); // Decryption
void printString(string s); // Encryption
string int_to_string(int x); // Encryption
int string_to_int(string str, int attribute); // bluetooth
string char_to_string(char c); // Decryption

// Ultrasonic Sensor variables
int dist = 0;
int correction = 0;
Timer sonar;

// Bluetooth variables
int x;
int knt = 0;
int index = 0;
int arrSize = 5;
int key;


// Mbed object attributes
int mId;
int mDist;
int mRecord;
int mTime;

// Character array variables
string s;
string str;
char a;
int n;
int temp_num;

// Decryption driver variables
string temp;
char temp_char = ' ';

// Mbed slave object
class m_mbedObj {
    private:
        int id, record;
    public:
        m_mbedObj(int a, int b) { // mbed object constructor
            this->id = a;
            this->record = b;
        }
        // get value for ID
        int getID() {
            return id;
        }
        // get value for record
        int getRecord() {
            return record;
        }


};

// Mbed object
class mbedObj {
    private:
        int id, dist, record, time;
    public:
        mbedObj(int a, int b, int c, int d) { // mbed object constructor 
            this->id = a;
            this->dist = b;
            this->record = c;
            this->time = d;
        }
        // get value for ID
        int getID() {
            return id;
        }
        // get value for distance
        int getDist() {
            return dist;
        }
        // get value for record
        int getRecord() {
            return record;
        }
        // get value for time
        int getTime() {
            return time;
        }


};

// Initialize object array
mbedObj* mbedConnections = (mbedObj*)malloc(sizeof(mbedObj) * arrSize); // array for foreign connections
m_mbedObj* mbedOrigin = (m_mbedObj*)malloc(sizeof(m_mbedObj) * 1); // mbed for slave mbedObj

int main()
{
    // construct slave mbedObj
    m_mbedObj* slave = new m_mbedObj(5, 1);

    sonar.reset();
// measure actual software polling timer delays
// delay used later in time correction
// start timer
    sonar.start();
// min software polling delay to read echo pin
    while (echo==2) {};
    myled2 = 0;
// stop timer
    sonar.stop();
// read timer
    correction = sonar.read_us();
    printf("Approximate software overhead timer delay is %d uS\n\r",correction);

//Loop to read Sonar distance values, scale, and print
    while(1) {
// trigger sonar to send a ping
        trigger = 1;
        myled = 1;
        myled2 = 0;
        sonar.reset();
        wait_us(10.0);
        trigger = 0;
        myled = 0;
//wait for echo high
        while (echo==0) {};
        myled2=echo;
//echo high, so start timer
        sonar.start();
//wait for echo low
        while (echo==1) {};
//stop timer and read value
        sonar.stop();
//subtract  timer delay and scale to cm
        dist = (sonar.read_us()-correction)/58.0;
        myled2 = 0;
        printf(" %d cm \n\r",dist);
        // At 20 cm being open connection for pairing
        if (dist < 20) {
            printf("Pairing... \n\r");
            rn42.baud(115200); // set baud rate
            initialise_connection_slave(); // initialize connection as slave device (open connection)
            while (1){
                read_from_master(); // read data from master device
                if (x == 16) { // when the value of 16 is recieve master device is now reading
                    break;
                }
            }
            mDist = dist; // store distance value of master
            mTime = knt; // store internal clock time (knt)
            led = 0;
            mbedConnections[index] = mbedObj(mId, mDist, mRecord, mTime); // create mbedObj for master and store into mbedConnectiosn array
            // send results to terminal
            printf("id = %d \n\r", mbedConnections[index].getID());
            printf("dist = %d \n\r", mbedConnections[index].getDist());
            printf("record = %d \n\r", mbedConnections[index].getRecord());
            printf("time = %d \n\r", mbedConnections[index].getTime());
            index++; // increment array index
            int sId = slave->getID(); // get slave ID to be sent
            int sRecord = slave->getRecord(); // get slave record to be sent
            wait(20.0); // wait for master to connect
            led = 15;
            while (1) {
                write_to_master(sId, sRecord); // send slave data to master
                break;
            }
        }
//wait so that any echo(s) return before sending another ping
        wait_us(200000);
    }
}

// connect as slave device (open connection)
void initialise_connection_slave(){
    rn42.putc('$'); // Enter command mode
    rn42.putc('$'); //
    rn42.putc('$'); //
    wait(1.2);
    rn42.putc('S'); // set authentication to 0
    rn42.putc('A');
    rn42.putc(',');
    rn42.putc('0');
    rn42.putc(0x0D);
    rn42.putc('-'); // Exit command mode
    rn42.putc('-');
    rn42.putc('-');
    rn42.putc(0x0D);
    wait(0.5);
}

// send data to master device
void write_to_master(int id, int record) {
    int key = id; // store key value
    rn42.putc(key); // send key value
    wait(0.5);
    
    encryption_driver(id, key); // ecnrypt and send slave ID
    encryption_driver(record, key); // encrypt and send slave record
    // Send value of 16 to signal master that slave is done transmitting
    x = 16; 
    led = x;
    rn42.putc(x);
    wait(0.5);
    return;
}

// read data from master device
void read_from_master(){ 
    if (rn42.readable()) { // if data is available
        x = rn42.getc(); // get data
        printf("x = %d \n\r", x);
        led = 15;
        knt++; // increment clock
        if (knt > 10){
            switch(knt){
                case 11: // store master key value
                    key = x;
                    break;
                case 12: // format, decrypt and store master ID
                    temp = int_to_string(x);
                    temp_char = (char) x;
                    cout << temp_char << endl;
                    temp = char_to_string(temp_char);
                    printString(temp + "\n\r");
                    temp = decryptString(temp, key);
                    printString(temp + "\n\r");
                    x = string_to_int(temp, x);
                    mId = x;
                    break;
                case 13: // format, decrypt and store master record
                    temp = int_to_string(x);
                    temp_char = (char) x;
                    cout << temp_char << endl;
                    temp = char_to_string(temp_char);
                    printString(temp + "\n\r");
                    temp = decryptString(temp, key);
                    printString(temp + "\n\r");
                    x = string_to_int(temp, x);
                    mRecord = x;
                    break;

            }
        }
    }
}

// cypher encryption algorithm
string encryptString(string message, int changeBy)
{
    for (int i = 0; i < message.length(); i++)
    {
        message[i] = (message[i] + changeBy) % 256;
    }
    printString(message + "\n\r");
    return message;
}
// cypher decryption algorithm
string decryptString(string message, int changeBy)
{
    for (int i = 0; i < message.length(); i++)
    {
        int temp =  message[i] - changeBy;
        while (temp < 0)
        {
            temp += 256;
        }
        message[i] = temp;
    }
    return message;
}
// print string
void printString(string s)
{
    for (int i = 0; i < s.length(); i++)
    {
        printf("%c", s[i]);
    }
}
// convert integers to strings
string int_to_string(int x){
    ostringstream str1;
    str1 << x;
    string str = str1.str();
    return str;
}
// send data to master
void send_data(char * arr, int arrS){
    for(int i = 0; i < arrS; i++){
        temp_num = arr[i];
        rn42.putc(temp_num);
        wait(0.5);
    }   
}
// driver to encrypt and send data
void encryption_driver(int attribute, int key){
    x = attribute;
    led = x;
    s = int_to_string(x);
    str = encryptString(s, key); // encrypt values
    n = str.length();
    char char_arrayId[n + 1];
    strcpy(char_arrayId, str.c_str()); // create char array from encrypted message
    for (int i = 0; i < str.length(); i++){
        printf("returned char %d \n\r", char_arrayId[i]);
    }
    send_data(char_arrayId, n); // send char to master  
}
// convert strings to integers
int string_to_int(string str, int attribute){
    stringstream attNum(str);
    attNum >> attribute;
    printf("New int: %d \n\r", attribute);
    return attribute;
}
// convert char to strings
string char_to_string(char c){
    string strX;
    stringstream ss;
    ss << c;
    ss >> strX;
    return strX;
}