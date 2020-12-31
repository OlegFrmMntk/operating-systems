#ifndef INC_2_CALCULATOR_H
#define INC_2_CALCULATOR_H

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include "Producer.h"

using namespace std;
using namespace chrono;

struct Point {
    double x;
    double y;
};

class SystemInfoPoint {

public:

    SystemInfoPoint(Point p) : point(p) {

    }
    
    Point point;

    system_clock::time_point accept_time;
    system_clock::time_point write_time;
};

void produceFuncValues(const function<void(const Point & p)>& func) {
    
    for (int i = 0; i < 10; i++) {
        Point p { i, 7 * i * i + 12 * i - 2 };
        func(p);
    }
}

class Calculator {
    
    Producer <Point> calculator = Producer <Point>(1);
    Producer <SystemInfoPoint> system_info = Producer <SystemInfoPoint>(1);
    
    thread values_writer;
    thread logging_tr;
    
    function <void (function < void (const Point & p)>)> calc_function;

public:

    Calculator(function < void(function < void (const Point & p)>)> calc_function) : calc_function(move(calc_function)) {
    }

    void process(ostream& value_write_os = cout, ostream& logging_write_os = cout) {

        values_writer = thread(calculatorSubscribe, ref(calculator), ref(system_info), ref(value_write_os));
        logging_tr = thread(logging, ref(system_info), ref(logging_write_os));
        
        calc_function([&](const Point& p) { calculator.publish(p); });
        calculator.stopPublishing();
        
        values_writer.join();
        system_info.stopPublishing();
        
        logging_tr.join();
    }

private:

    static void calculatorSubscribe(Producer <Point>& subscribe, Producer <SystemInfoPoint>& publish, ostream& fout) {
        auto foo = [&publish, &fout](Point& v) {
         
            SystemInfoPoint pointWithSystemInfo(v);
            pointWithSystemInfo.accept_time = system_clock::now();
            
            fout << v.y << " " << v.x << "\n";
            
            pointWithSystemInfo.write_time = system_clock::now();
            publish.publish(pointWithSystemInfo);
        };
        subscribe.subscribe(foo);
    }

    static void logging(Producer <SystemInfoPoint>& cq, ostream& fout) {
        auto foo = [&fout](SystemInfoPoint& v) {
          
            auto accept_time = system_clock::to_time_t(v.accept_time);
            auto write_time = system_clock::to_time_t(v.write_time);
            
            fout << "point: y: " << v.point.y 
                 << ", x: " << v.point.x 
                 << ", accept time: " << ctime(&accept_time) 
                 << "write to file time: " << ctime(&write_time) << "\n";
        };

        cq.subscribe(foo);
    }
};


#endif