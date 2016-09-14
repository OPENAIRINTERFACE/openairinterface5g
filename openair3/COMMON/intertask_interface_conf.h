/*
 * intertask_interface_conf.h
 *
 *  Created on: Oct 21, 2013
 *      Author: winckel
 */

#ifndef INTERTASK_INTERFACE_CONF_H_
#define INTERTASK_INTERFACE_CONF_H_

/*******************************************************************************
 * Intertask Interface Constants
 ******************************************************************************/

#define ITTI_PORT                (10007)

/* This is the queue size for signal dumper */
#define ITTI_QUEUE_MAX_ELEMENTS  (200 * 1024)
#define ITTI_DUMP_MAX_CON        (5)    /* Max connections in parallel */

#endif /* INTERTASK_INTERFACE_CONF_H_ */
