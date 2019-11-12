#ifndef __READMYMIND_H
#define __READMYMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define READMYMIND_IOC_MAGIC  'k'
#define MMIND_REMAINING    _IO(READMYMIND_IOC_MAGIC, 0)
#define MMIND_ENDGAME     _IO(READMYMIND_IOC_MAGIC, 1)
#define MMIND_NEWGAME      _IO(READMYMIND_IOC_MAGIC, 2)
#define READMYMIND_IOC_MAXNR 2

#endif