#ifndef HELPER_FUNC_H_
#define HELPER_FUNC_H_
#include "commands_xcore.h"
#include "../Application/slib/s_serial_port.hpp"
#include "build_command.h"
#include "commands_vpc_ext.h"






// current_level: okuma komutundan gelen mevcut seviye (1–5)
// cmd: +5 veya -5 şeklindeki gelen komut
// Dönüş: 1 ile 5 arasında güncellenmiş seviye
uint8_t updateLevel(uint8_t& current_level, bool pos_)
{
    // cmd pozitif ise yukarı, negatif ise aşağı hareket
    if (pos_ == 1)
        current_level++;
    else
        current_level--;

    // Alt ve üst sınırları koru
    if (current_level < 1)
        current_level = 1;
    if (current_level > 5)
        current_level = 5;

    return current_level;
}

uint8_t func_param_save(uint8_t buff,uint8_t* cmd_byte_,uint8_t quantity__,uint8_t& save_param)
{
	uint8_t temp_br_val=save_param;
	if(buff<=1)
	{
		updateLevel(save_param,(bool)buff);
	}
	if(save_param!=temp_br_val)
	{
		buff=save_param;
	}
	return save_param;
}

bool transmit_mcu_to_ports_direct(uint8_t* array_, size_t len, simple::serial_port* temp_obj)
{
	for(int i=0;i<len;++i) // Yeni kameraya ait paket boyutu sabit 12 byte
	{
		temp_obj->push_from_ll(static_cast<char>(array_[i]));
	}
	return 0;
}

#endif /* HELPER_FUNC_H_ */
