Notes: 

	The OMG module should be updated every one millisecond. This periodic update allows to check whether at the given moment there exists a node that should move (sleep_duration is elapsed) or sleep (it has already reached its destination). It allows also to compute the current position of the nodes, namely when they are in the move pahse. Therefore, to get all the nodes positions at any moment, the function get_current_positions might be used. Note that it gives the positions at the time specified by the last update_nodes function. This being said, a call to get_current_positions or get_node_position(to get the position of a specific node identified by its ID) should always be preceded by a call to update_nodes which takes for argument the desired time in milliseconds. 

	In order to avoid instability, make sure the following parameters are diffrent from zero: (min_speed, min_journey_time and min_sleep).

