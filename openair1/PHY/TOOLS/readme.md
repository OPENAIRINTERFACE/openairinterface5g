## xForms-based Scope
To use the scope, run the xNB or the UE with option "-d"

Usage in gdb
In gdb, when you break, you can refresh immediatly the scope view by calling the display function.
The first paramter is the graph context, nevertheless we keep the last value for a dirty call in gdb (so you can use '0')

Example with no variable known
phy_scope_nrUE(0, PHY_vars_UE_g[0][0], 0, 0, 0)

or
phy_scope_gNB(0, phy_vars_gnb, phy_vars_ru, UE_id)

# Qt-based Scope
## Building Instuctions
For the new qt-based scopo designed for NR, please consider the following:

1. run the gNB or the UE with the option '--dqt'.
2. make sure to install the Qt5 packages before running the scope. Otherwise, the scope will NOT be displayed!
3. if you need only to build the new scope, then add 'nrqtscope' after the '--lib-build' option. So, the complete
   command would be

   ```
   ./build_oai --gNB -w USRP --nrUE --build-lib nrqtscope
   ```

## New Features
1. New KPIs for both gNB and UE, e.g., BLER, MCS, throughout, and number of scheduled RBs.
2. For each of the gNB and UE, a main widget is created with a 3x2 grid of sub-widgets, each to dispaly one KPI.
3. Each of the sub-widgets has a drop-down list to choose the KPI to show in that sub-widget.
4. Both of the gNB and UE scopes can be resized using the mouse movement.
