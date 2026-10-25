#pragma once
#include "Net manager.hpp"




namespace event {





namespace detail {

bool running = false;
void watch_loop(){
   
            while(running)
    auto result = Network::poll_and_receive() ;
    if(result.size()>0){
      //TODO trigger the handler;
      
    }



    //TODO : sleep for few seconds;if the wifi is broken;
   

}


} ;




//TODO: this file is where we handle the server command;


inline void Register_server_command(){



    // Create the application loop task with a safe stack size (e.g., 8192 bytes) and priority 5
    xTaskCreate(
        app_run_loop,          // Task function
        "app_run_loop_task",   // Task name for debugging
        magicStackNumber,                  // Stack depth (bytes)
        NULL,                  // Parameters to pass
        5,                     // Task priority
        NULL                   // Task handle output
    );





}









} ;