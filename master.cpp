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
void initialise_connection_master(void); // bluetooth
void write_to_slave(int id, int record); // bluetooth
void read_from_slave(void); // bluetooth
char * string_to_char_arr(string emessage); // Bluetooth
void send_data(char * arr, int arrS); // Bluetooth
void encryption_driver(int attribute, int key); // Encryption
string encryptString(string message, int changeBy); // Encryption
string decryptString(string message, int changeBy); // Decryption
void printString(string s); // Encryption
string int_to_string(int x); // Encryption
int string_to_int(string str, int attribute); // Bluetooth
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

// Mbed object atttributes
int sId;
int sDist;
int sRecord;
int sTime;

// Character array variables
string s;
string str;
char a;
int n;
int temp_num;

// Decryption driver variables
string temp;
char temp_char = ' ';

// Mbed master object
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
        // get value of ID
        int getID() {
            return id;
        }
        // get value of Distance
        int getDist() {
            return dist;
        }
        // get value of Record
        int getRecord() {
            return record;
        }
        // get value of Time
        int getTime() {
            return time;
        }


};

mbedObj* mbedConnections = (mbedObj*)malloc(sizeof(mbedObj) * arrSize); // initialize array for foreign mbedObj on heap
m_mbedObj* mbedOrigin = (m_mbedObj*)malloc(sizeof(m_mbedObj) * 1); // initialize array for master device mbedObj on heap

int main()
{
    // Construct Master device mbedObj 
    m_mbedObj* master = new m_mbedObj(6, 9);
    int mId = master->getID();;
    int mRecord = master->getRecord();

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
        // At 20 cm, being the pairing of bluetooth device.
        if (dist < 20) {
            printf("Pairing... \n\r");
            rn42.baud(115200); // set buad rate
            wait(1.0); // can only enter Command mode 500ms after power-up
            initialise_connection_master(); // set device as master and search for open connections
            write_to_slave(mId, mRecord); // transfer data to slave device if connected
            led = 0;
            wait(0.5);
            led = 15;
            // Master switching to read from the slave device
            while (1){ 
                read_from_slave(); // pull data from the slave device and store id and record values
                if (x == 16) { // Recieved data of 16, signalling the master  device to break from reading
                    break;
                }
            }
            sDist = dist; // store distance recorded from ultrasonic sensor
            sTime = knt; // store internal clock time (knt)
            led = 0;
            mbedConnections[index] = mbedObj(sId, sDist, sRecord, sTime); // create mbedObj with stored values from slave and store into mbedConnections array
            // Display results to terminal
            printf("id = %d \n\r", mbedConnections[index].getID()); 
            printf("dist = %d \n\r", mbedConnections[index].getDist());
            printf("record = %d \n\r", mbedConnections[index].getRecord());
            printf("time = %d \n\r", mbedConnections[index].getTime());
            index++; // move to next spot in mbedConnections array
            wait(10.0);
            led = 15;
            break; // begin searching again using sensor
        }
//wait so that any echo(s) return before sending another ping
        wait_us(200000);
    }
}

// Connect as master device
void initialise_connection_master(){
    rn42.putc('$'); // Enter command mode
    rn42.putc('$'); //
    rn42.putc('$'); //
    wait(1.2);
    rn42.putc('S'); // enter Master mode
    rn42.putc('M'); 
    rn42.putc(',');
    rn42.putc('3'); // Auto-connect master mode
    rn42.putc(0x0D); // CR
    wait(0.1);
    rn42.putc('C'); // attempt connection
    rn42.putc(','); // Send MAC address of target slave device
    rn42.putc('0');
    rn42.putc('0');
    rn42.putc('0');
    rn42.putc('6');
    rn42.putc('6');
    rn42.putc('6');
    rn42.putc('E');
    rn42.putc('3');
    rn42.putc('F');
    rn42.putc('2');
    rn42.putc('5');
    rn42.putc('B');
    rn42.putc(0x0D);
    wait(1.0);
    rn42.putc('-'); // Exit command mode
    rn42.putc('-');
    rn42.putc('-');
    rn42.putc(0x0D);
    wait(0.5);
}

void write_to_slave(int id, int record) {
    key = id; // set key for encryption
    rn42.putc(key); // send key to slave
    wait(0.5);

    encryption_driver(id, key); // encrypt and send id 
    encryption_driver(record, key); // encrypt and send record

    // send 16 to signal slave device that master is done transmitting
    x = 16;
    led = x;
    rn42.putc(x);
    wait(0.5);
    return;
}

void read_from_slave(){
    if (rn42.readable()) { // see if there is data to be read
        x = rn42.getc(); // being recieving data from slave
        printf("x = %d \n\r", x);
        led = 15;
        knt++; // increment clock
        if (knt > 16){ // after master recieves 16 chars from slave begin to store values
            switch(knt){
                case 17:
                    key = x; // store key
                    break;
                case 18:
                    // reformat, decrypt and store slave ID
                    temp = int_to_string(x);
                    temp_char = (char) x;
                    cout << temp_char << endl;
                    temp = char_to_string(temp_char);
                    printString(temp + "\n\r");
                    temp = decryptString(temp, key);
                    printString(temp + "\n\r");
                    x = string_to_int(temp, x);
                    sId = x;
                    break;
                case 19:
                    // reformat, decrypt and store slave record
                    temp = int_to_string(x);
                    temp_char = (char) x;
                    cout << temp_char << endl;
                    temp = char_to_string(temp_char);
                    printString(temp + "\n\r");
                    temp = decryptString(temp, key);
                    printString(temp + "\n\r");
                    x = string_to_int(temp, x);
                    sRecord = x;
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

// print string method
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

// send data through bluetooth
void send_data(char * arr, int arrS){
    for(int i = 0; i < arrS; i++){
        temp_num = arr[i];
        rn42.putc(temp_num);
        wait(0.5);
    }   
}

// driver for encryption and sending data to slave
void encryption_driver(int attribute, int key){
    x = attribute;
    led = x;
    s = int_to_string(x);
    str = encryptString(s, key); // encryption
    n = str.length();
    char char_array[n + 1];
    strcpy(char_array, str.c_str()); // create char array from encrypted message
    for (int i = 0; i < str.length(); i++){
        printf("returned char %d \n\r", char_array[i]);
    }
    send_data(char_array, n); // send char one by one from array
}

// convert strings to integers
int string_to_int(string str, int attribute){
    stringstream attNum(str);
    attNum >> attribute;
    printf("New int: %d \n\r", attribute);
    return attribute;
}

// convert chars to strings
string char_to_string(char c){
    string strX;
    stringstream ss;
    ss << c;
    ss >> strX;
    return strX;
}