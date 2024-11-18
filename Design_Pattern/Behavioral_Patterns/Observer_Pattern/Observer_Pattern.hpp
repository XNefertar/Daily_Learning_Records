#include <iostream>

class ObserverInterface
{
public:
    virtual void update(double temperature, double humidity) = 0;
    virtual void display(double temperature, double humidity) = 0;
};

class SubjectInterface
{
public:
    virtual void RegisterObserver(ObserverInterface* observer) = 0;
    virtual void RemoveObserver(ObserverInterface* observer) = 0;
    virtual void NotifyObserver() = 0;
};

class WeatherData
:public SubjectInterface
{
public:
    virtual void RegisterObserver(ObserverInterface* observer) override {

    }
    virtual void RemoveObserver(ObserverInterface *observer) override {

    }
};