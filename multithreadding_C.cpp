//============================================================================
// Name        : Multi-threadding 
// Author      : Sam Seaberry 10367663
// Version     :
// Copyright   : Your copyright notice
// Description : multithreading with dual access mutexes
//============================================================================


#include <iostream>
#include <thread>
#include <mutex>
#include <stdlib.h>
#include <ctime>
#include <vector>
#include <random>
#include <chrono>
#include <map>
#include <condition_variable>


using namespace std;

std::map<std::thread::id, int> threadIDs;

int const MAX_NUM_OF_THREADS = 6;

int const NUM_OF_LINKS = 2;

std::mutex mu;

class Semaphore { //thought it might be useful
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    Semaphore(int count_ = 6)
        : count(count_) {}

    void signal() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;

        //notify the waiting thread
        cv.notify_one();
    }
    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        while (count == 0) {

            //wait on the mutex until notify is called
            cv.wait(lock);

        }
        count--;
    }


};


class Sensor {
public:
    Sensor(string& type) : sensorType(type) {}

    string sensorType;
    virtual double getValue() = 0;  // getvalue initalization 

    string getType() {
        return sensorType; //returns the sensor type

    }


}; //end abstract class Sensor

class SensorData { //Utility class to store sensor data
public:
    SensorData(string type) //Constructor
        : sensor_type(type) {}

    string sensor_type;

    string getSensorType() {
        return sensor_type; //return the sensor type
    }

    std::vector<double> getSensorData() {
        return sensor_data;
    }

    void addData(double newData) {
        sensor_data.push_back(newData);

    }
private:

    std::vector<double> sensor_data;
};


class BC {
public:

    BC(std::vector<Sensor*>& sensors) : theSensors(sensors) {}
    void requestBC() {

        while (lock == true) { //spin lock if BC is in use
            ;
        }
        //locks
        std::unique_lock<std::mutex> lck(BC_mu);
        lock = true;

    }
    double getSensorValue(int selector) {
        return (*theSensors[selector]).getValue();
    }
    string getSensorType(int selector) {
        return (*theSensors[selector]).getType();

    }
    void releaseBC() {
        std::unique_lock<std::mutex> lck(BC_mu); //unlocks
        lock = false;

    }
private:
    bool lock = false; //'false' means that the BC is not locked
    std::vector<Sensor*>& theSensors; //reference to vector of Sensor pointers
    std::mutex BC_mu; //mutex


}; //end class BC


class TempSensor : public Sensor { //syntax declares a derived class

public:

    TempSensor(string& s) : Sensor(s) {

    } //constructor

    virtual double getValue() {

        double value = rand() % 20 + 10.0;

        return value;
    } //end getValue
}; //end class TempSensor

class CapSensor : public Sensor { //syntax declares a derived class

public:

    CapSensor(string& s) : Sensor(s) {

    } //constructor

    virtual double getValue() {

        double value = rand() % 6;

        return value;
    } //end getValue
}; //end class capSensor

class PressureSensor : public Sensor { //syntax declares a derived class

public:

    PressureSensor(string& s) : Sensor(s) {

    } //constructor

    virtual double getValue() {

        double value = rand() % 11 + 90.0;

        return value;
    } //end getValue
}; //end class PressureSensor

class Receiver {
public:
    Receiver() { } //constructor
    //Receives a SensorData object:
    void receiveTempData(SensorData sd) {  // each sensor type given a Receive function
        std::unique_lock<std::mutex> lock(mu);
        t++; //incremented for the print function 
        temp_data = sd.getSensorData();

    }
    void receivePresData(SensorData sd) {
        std::unique_lock<std::mutex> lock(mu);//locked so only one thread can write at a time
        p++;
        pres_data = sd.getSensorData();

    }
    void receiveCapData(SensorData sd) {
        std::unique_lock<std::mutex> lock(mu);
        c++;
        cap_data = sd.getSensorData();

    }
    // print out all data for each sensor:
    void printTempSensorData() {             // each sensor given a print fuction 
        std::unique_lock<std::mutex> lock(mu);
        while (!t == 0) {
            cout << "Temperature " << temp_data[t] << endl;
            t--;
        }
    }
    void printCapSensorData() {
        std::unique_lock<std::mutex> lock(mu);//locked so only one thread can read at a time
        while (!c == 0) {
            cout << "Capacitance " << cap_data[c] << endl;
            c--;
        }
    }
    void printPresSensorData() {
        std::unique_lock<std::mutex> lock(mu);
        while (!p == 0) {
            cout << "Pressure " << pres_data[p] << endl;
            p--;
        }
    }
private:
    //mutex:
    std::mutex mu;
    //vectors to store sensor numeric data received from threads:
    std::vector<double> temp_data;
    std::vector<double> cap_data;
    std::vector<double> pres_data;
    int t = 0, c = 0, p = 0;


}; //end class Receiver

class Link {
public:
    Link(Receiver& r, int linkNum) //Constructor
        : inUse(false), myReceiver(r), linkId(linkNum)
    {}
    //check if the link is currently in use
    bool isInUse() {
        return inUse;
    }
    //set the link status to busy
    void setInUse() {
        inUse = true;
    }
    //set the link status to idle
    void setIdle() {
        inUse = false;
    }
    //write data to the receiver
    void writeToDataLink(SensorData sd, int i) { //int i tells which sensor type is receiving
        sd.getSensorData();
        switch (i) {
        case (0):
            myReceiver.receiveTempData(sd);
            break;
        case (1):
            myReceiver.receivePresData(sd);
            break;
        case (2):
            myReceiver.receiveCapData(sd);
            break;
        }

    }
    //returns the link Id
    int getLinkId() {
        return linkId;
    }
private:
    bool inUse = false;
    Receiver& myReceiver; //Receiver reference
    int linkId;
}; //end class Link

class LinkAccessController {
private:
    Receiver& myReceiver; //Receiver reference
    int numOfAvailableLinks;
    std::vector<Link> commsLinks;
    std::condition_variable cv;
    std::mutex LAC_mu; //mutex




public:
    LinkAccessController(Receiver& r) //Constructor
        : myReceiver(r), numOfAvailableLinks(NUM_OF_LINKS - 1)//numOfAvailableLinks(NUM_OF_LINKS-1) -1 becasue there are only 2 commLinks but 3 Avaliable links 
    {
        for (int i = 0; i < NUM_OF_LINKS; i++) {
            commsLinks.push_back(Link(myReceiver, i)); //creates a vector of class link 
        }
    }
    //Request a comm's link: returns a reference to an available Link.
    //If none are available, the calling thread is suspended.


    Link& requestLink(int i) { //int i used to print out thread number                        
        std::unique_lock<std::mutex> lock(LAC_mu);
        if (numOfAvailableLinks < 0) {
            ;
        }
        while (commsLinks[numOfAvailableLinks].isInUse()) { //locked if in use
            std::cout << "thread " << i << " about to suspend...\n";
            cv.wait(lock);
        }
        commsLinks[numOfAvailableLinks].setInUse();//sets in use if not in use 


        cout << "thread " << i << " unlocked" << endl;
        return std::ref(commsLinks[numOfAvailableLinks]);//decremneted after the return statment 
    }
    //Release a comms link:
    void releaseLink(Link& releasedLink) {
        std::unique_lock<std::mutex> unlock(LAC_mu);



        commsLinks[numOfAvailableLinks].setIdle();
        cv.notify_all();



    }



}; //end class LinkAccessController

void run(BC& theBC, int idx, LinkAccessController& lac, SensorData& Temp, SensorData& caps, SensorData& pres) {


    unique_lock<mutex> map_locker(mu);
    pair<thread::id, int> p = make_pair(this_thread::get_id(), idx); //get and set thread ids to ints 
    threadIDs.insert(p);
    map_locker.unlock();


    // request use of the BC:
    int pressure = 0, cap = 0, temp = 0, sensor, NUM_OF_SAMPLES = 50;
    int times = p.second;
    srand((unsigned)time(NULL) + times); //without adding the thread id rand will give the same values 
    for (int i = 0; i < NUM_OF_SAMPLES; i++) {
        theBC.requestBC();
        string type;
        Link* link;
        //double data;
        sensor = rand() % 3; // rand gives values between 0-2
        switch (sensor) {
        case(0):
            type = theBC.getSensorType(0);//gets type and writes to type
            link = &lac.requestLink(p.second);
            Temp.addData(theBC.getSensorValue(0));
            lac.releaseLink(*link);
            temp++;
            break;
        case(1):
            type = theBC.getSensorType(1);
            link = &lac.requestLink(p.second);//request link and send the time id 
            pres.addData(theBC.getSensorValue(1));//write data to sensor data vector 
            lac.releaseLink(*link);
            pressure++;
            break;
        case(2):
            type = theBC.getSensorType(2);
            link = &lac.requestLink(p.second);
            caps.addData(theBC.getSensorValue(2));
            lac.releaseLink(*link);
            cap++;
            break;
        }

        theBC.releaseBC();

        int timea = rand() % 11;

        std::this_thread::sleep_for(std::chrono::milliseconds(timea));


    } // end of for
    Link* link = &lac.requestLink(p.second);
    link->writeToDataLink(Temp, 0); //writes to each link 
    link->writeToDataLink(pres, 1);
    link->writeToDataLink(caps, 2);
    lac.releaseLink(*link);

} // end of run



int main()
{
    Receiver t;
    Receiver* r;

    r = &t;

    LinkAccessController linkas(*r);



    Link link(*r, NUM_OF_LINKS);



    //declare a vector of Sensor pointers:
    std::vector<Sensor*> sensors;
    //initialise each sensor and insert into the vector:
    string s = "temperature sensor";
    sensors.push_back(new TempSensor(s));

    string p = "pressure sensor";
    sensors.push_back(new PressureSensor(p));

    string c = "Capacitive sensor";
    sensors.push_back(new CapSensor(c));


    BC theBC(std::ref(sensors));



    SensorData cap("Capacitive");
    SensorData temp("Temperature");
    SensorData pres("Pressure");


    std::thread the_threads[MAX_NUM_OF_THREADS];

    /*the_threads[0] = std::thread(run, &theBC, 0, &linkas);
    the_threads[1] = std::thread(run, &theBC, 1, &linkas);
    the_threads[2] = std::thread(run, &theBC, 2, &linkas);
    the_threads[3] = std::thread(run, &theBC, 3, &linkas);
    the_threads[4] = std::thread(run, &theBC, 4, &linkas);
    the_threads[5] = std::thread(run, &theBC, 5, &linkas);
    */

    for (int i = 0; i < MAX_NUM_OF_THREADS; i++) {

        the_threads[i] = std::thread(run, std::ref(theBC), i, std::ref(linkas), std::ref(cap), std::ref(temp), std::ref(pres));//initalize threads and run 


    }
    // do some other stuff

    // loop again to join the threads
    for (auto& t : the_threads) {
        t.join(); //join once all threads have finished
    }

    t.printCapSensorData(); //print data from each sensor
    t.printPresSensorData();
    t.printTempSensorData();


    cout << "All threads terminated" << endl;


    return 0;


}
