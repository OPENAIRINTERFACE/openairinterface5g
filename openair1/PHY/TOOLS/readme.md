
To use the scope, run the xNB or the UE with option "-d"  

Usage in gdb
In gdb, when you break, you can refresh immediatly the scope view by calling the display function.
The first paramter is the graph context, nevertheless we keep the last value for a dirty call in gdb (so you can use '0')  

Example with no variable known
phy_scope_nrUE(0, PHY_vars_UE_g[0][0], 0, 0, 0)

or
phy_scope_gNB(0, phy_vars_gnb, phy_vars_ru, UE_id)
