#include <iostream>
#include <vector>
#include <algorithm>

#define DEFAULT_TEMPERATURE 0.0
#define DEFAULT_HUMIDITY    0.0

class ObserverInterface
{
public:
    virtual void update(double temperature, double humidity) = 0;
    virtual void display() = 0;
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
private:
    std::vector<ObserverInterface*> _ObserverPtrArray;
    double _Temperature;
    double _Humidity;

public:
    WeatherData(double default_temperature = DEFAULT_TEMPERATURE, double default_humidity = DEFAULT_HUMIDITY)
        : _Temperature(default_temperature),
          _Humidity(default_humidity)
    {}

    virtual void RegisterObserver(ObserverInterface* observer) override {
        _ObserverPtrArray.push_back(observer);
    }

    virtual void RemoveObserver(ObserverInterface *observer) override {
        // Method 1
        // std::find 找对应位置的 iterator
        // auto it = std::find(_ObserverPtrArray.begin(), _ObserverPtrArray.end(), observer);
        // if(it != _ObserverPtrArray.end()){
        //     _ObserverPtrArray.erase(it);
        // }

        // Method 2
        // std::remove 和 std::erase 结合
        // std::move: 在给定的范围内找到对应的 observer, 并将其移至末尾
        // std::erase: 使用重载, 在给定范围内删除所有元素
        _ObserverPtrArray.erase(std::remove(_ObserverPtrArray.begin(), _ObserverPtrArray.end(), observer), _ObserverPtrArray.end());
    }

    void SetWeatherData(double default_temperature = DEFAULT_TEMPERATURE, double default_humidity = DEFAULT_HUMIDITY){
        _Temperature = default_temperature;
        _Humidity = default_humidity;
    }

    virtual void NotifyObserver(){
        for(auto it : _ObserverPtrArray){
            it->update(_Temperature, _Humidity);
        }
    }
};

class CurrentConditionDisplay
:public ObserverInterface
{
private:
    double _Temperature;
    double _Humidity;

public:

    virtual void display() override {
        std::cout << "Current Condition Display \t" << std::endl;
        std::cout << "The current temperature is " << _Temperature << ", and the humidity is " << _Humidity << "." << std::endl;
    }

    virtual void update(double Temperature, double Humidity) override {
        _Temperature = Temperature;
        _Humidity = Humidity;
        display();
    }
};