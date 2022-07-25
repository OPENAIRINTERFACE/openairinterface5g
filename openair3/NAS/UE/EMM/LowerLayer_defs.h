#ifndef _LOWER_LAYER_DEFS_H
#define _LOWER_LAYER_DEFS_H

/*
 * Type of EMM procedure callback function executed whenever data are
 * successfully delivered to the network
 */
typedef int (*lowerlayer_success_callback_t)(void *);

/*
 * Type of EMM procedure callback function executed when data are not
 * delivered to the network because a lower layer failure occurred
 */
typedef int (*lowerlayer_failure_callback_t)(bool, void *);

/*
 * Type of EMM procedure callback function executed when NAS signalling
 * connection is released
 */
typedef int (*lowerlayer_release_callback_t)(void *);

/*
 * Data structure used to handle EMM procedures executed by the UE upon
 * receiving lower layer notifications
 */
typedef struct {
  lowerlayer_success_callback_t success; /* Successful data delivery  */
  lowerlayer_failure_callback_t failure; /* Lower layer failure   */
  lowerlayer_release_callback_t release; /* NAS signalling release    */
  void *args;         /* EMM procedure argument parameters    */
} lowerlayer_data_t;

#endif
