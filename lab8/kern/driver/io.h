//
// Created by lumin on 2020/11/4.
//

#ifndef LAB8_IO_H
#define LAB8_IO_H

#define GPIOHS_BASE_ADDR    (0x38001000U)

/* Under APB1 32 bit */
#define SPI_SLAVE_BASE_ADDR (0x50240000U)
#define FPIOA_BASE_ADDR     (0x502B0000U)

/* Under APB2 32 bit */
#define SYSCTL_BASE_ADDR    (0x50440000U)

/* Under APB3 32 bit */
#define SPI0_BASE_ADDR      (0x52000000U)
#define SPI1_BASE_ADDR      (0x53000000U)
#define SPI3_BASE_ADDR      (0x54000000U)

#endif //LAB8_IO_H
