#include <iostream>
#include <memory>

class DiscountInterface
{
public:
    virtual ~DiscountInterface() = default;
    virtual double CalculateDiscount(double price) = 0;
};

class Discount1
:public DiscountInterface
{
public:
    virtual double CalculateDiscount(double price) override{
        return price -= 20;
    }
};

class Discount2
:public DiscountInterface
{
public:
    virtual double CalculateDiscount(double price) override{
        return price *= 0.75;
    } 
};
using ClassSharedPtr = std::shared_ptr<DiscountInterface>;

class DiscountContext
{
private:
    ClassSharedPtr _ManagerPtr;

public:
    DiscountContext(ClassSharedPtr ManagerPtr)
        : _ManagerPtr(ManagerPtr)
    {}

    void SetDiscountFunction(ClassSharedPtr ManagerPtr) {
        _ManagerPtr = ManagerPtr;
    }

    void CallDiscountFunction(double price) const {
        std::cout << "Before Price is " << price << "." << std::endl;
        std::cout << "After Price is " << _ManagerPtr->CalculateDiscount(price) << "." << std::endl;
    }
};