#ifndef _TESTSCENARIO_H
#    define _TESTSCENARIO_H
#    include "Message.h"
#    include "platform.h"



class TestScenario
{
public:
  static const int SCENARIO_ALL_IS_OK                     = 1;

  static void Start(const int scenario_idP);
  static void notifyRxMessage(Message*);

private:
  TestScenario ();
  ~TestScenario ();
  static int        s_step;
  static int        s_scenario_id;
};
#    endif

