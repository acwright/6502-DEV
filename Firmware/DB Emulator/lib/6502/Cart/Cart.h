#ifndef _CART_H
#define _CART_H

#include <Arduino.h>
#include "constants.h"

class Cart {
  private:
    uint8_t *data;
    
  public:
    Cart();
    ~Cart();
    
    String file = "None";

    uint8_t read(uint32_t index);
    void write(uint32_t index, uint8_t value);
    void load(uint32_t index, uint8_t value);
};

#endif
