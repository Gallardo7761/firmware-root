/**
 * @file    power.h
 * @author  Jose
 * @date    23/9/26
 * @brief   Energy saving handling
 * @project tfg
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Make the chip enter STOP mode only if the flag is false
 * @param seconds The seconds the chip is going to stay in STOP mode
 * @param abort_flag If it is true, then the chip will not enter STOP mode
 * @return bool If the chip has entered/has not entered STOP mode
 */
bool Power_Enter_Stop_Mode(uint32_t seconds, volatile bool *abort_flag);

#endif //POWER_H
