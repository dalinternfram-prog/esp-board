#include "wifi-state.hpp" 
#include <string>







namespace wifi {







    class wifi_state_handler{


private:
std::string stored_ssid;
std::string stored_pass;


  public:



void PassErrorHandler();

void SSIDErrorHandler();

void TimeOutHandler();


void ConnectedHandler();

void SignalWeakHandler();


void MaxRetriesExceededHandler();





void try_connect(std::string ssid,std::string pass);








    };
} ;