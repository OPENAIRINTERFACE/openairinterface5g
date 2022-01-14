# interthread interface (ITTI)

A Middleware called “itti” (interthread interface) provides classical send/receive message queues between threads.

A simpler thread safe message queue is available in the thread-pool, as it is a required lower level feature.

ITTI is providing more feature, to be used as the thread event main loop.

It is also made for strong type definition is source code.

It is able to add in the managed events: external sockets, timers

All queues are created in .h files, as static permanent queues, nevertheless the possibility to add queues during runtime is available.

The queues are called tasks because it is often read in a single thread that have the same name.

# API definition

## itti_send_msg_to_task, itti_receive_msg, itti_poll_msg

Standard messages queues, thread safe
The reader is responsible of freeing the message

## itti_subscribe_event_fd, itti_get_events

Add external sockets to itti based main loop

## timer_setup timer_remove

When a timer expire, it adds a message in a itti queue

## itti_terminate_tasks

Calling this function push a “terminate” message on each itti queue

# Example code of a thread using ITTI

```
Thread_aLayer_main(void * context) {
      // initialize the layer data
      // and register in itti the external sockets (like S1AP, GTP)
      aLayer_init();
      while (1) {
      MessageDef * msg
            itti_receive_msg(TASK_aLayer, &msg);
            // itti receive released, so we have incoming data
            // Can be a itti message
            if ( msg !=  NULL) {
                 switch(ITTI_MSG_ID(msg)) {
                      case …:
                           processThisMsgType1(msg);
                           break;
                      case …:
                           processThisMsgType2(msg);
                           break;
                 }
                 Itti_free(msg);
  	      }
            // or data/event on external entries (sockets)
            Struct eppol_events* events;
            int nbEvents=itti_get_events(TASK_aLayer, &events);
            if ( nbEvents > 0) 
               processLayerEvents(events, nbEvents);
      }
}


```

# Current usage in OpenAir

## UDP, SCTP

Simple threads that interface itti internal messages to Linux

## GTPV1-U, S1AP

Threads to implement S1AP and GTP protocols

## PDCP

4G Pdcp is not following itti design pattern, but it uses itti internally , 5G PDCP uses ad hoc design

## RRC

Implements 3GPP Rrc features
Interfaces the layer 1 and the above itti tasks 

## X2, ENP_APP (configuration), L1L2

Are declared itti queues with a dedicated thread, but they are almost empty