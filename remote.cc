#include <SDL.h>
#include <iostream>
#include <map>
#include <cstdint>

#include "packet_control_interface.h"

#define KEY_UP_ARROW    0
#define KEY_DOWN_ARROW  1
#define KEY_LEFT_ARROW  2
#define KEY_RIGHT_ARROW 3

typedef SDL_Scancode EScancode;

std::map<EScancode, uint8_t> mapKeyEncodings = {
   { SDL_SCANCODE_UP, KEY_UP_ARROW },
   { SDL_SCANCODE_DOWN, KEY_DOWN_ARROW },
   { SDL_SCANCODE_LEFT, KEY_LEFT_ARROW },
   { SDL_SCANCODE_RIGHT, KEY_RIGHT_ARROW },
};

int main(int argc, char *argv[]) {
   CPacketControlInterface* m_pcUplink = new CPacketControlInterface("uplink", "/dev/ttyUSB0", 57600);
   
   if(!m_pcUplink->Open()) {
         std::cerr << "Error" << std::endl << "Could not open the device" << std::endl;
         return -ENODEV;
   }
   
   m_pcUplink->SendPacket(CPacketControlInterface::CPacket::EType::SET_DDS_ENABLE, true);

   SDL_Window *win = NULL;
   SDL_Renderer *renderer = NULL;

   SDL_Init(SDL_INIT_VIDEO);

   win = SDL_CreateWindow("BeBot Remote", 100, 100, 200, 0, 0);

   renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

   bool bExitReq = false;
   
   uint8_t unRobotCtrl = 0, unRobotCtrlLast = 0;
   

   while(!bExitReq) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
         switch (e.type) {
         case SDL_QUIT:
            bExitReq = true;
            break;
         case SDL_KEYDOWN:
            if(mapKeyEncodings.find(e.key.keysym.scancode) != std::end(mapKeyEncodings)) {
               unRobotCtrl |= (1 << mapKeyEncodings[e.key.keysym.scancode]); 
            }
            break;
         case SDL_KEYUP:
            if(mapKeyEncodings.find(e.key.keysym.scancode) != std::end(mapKeyEncodings)) {
               unRobotCtrl &= ~(1 << mapKeyEncodings[e.key.keysym.scancode]); 
            }
            break;
         }
      }
      
      /* only upddate the motors if unRobotCtrl has changed */
      if((unRobotCtrl ^ unRobotCtrlLast) != 0) {
         int8_t pnSpeeds[2];
         
         switch(unRobotCtrl) {
         case (1 << KEY_UP_ARROW):
            pnSpeeds[0] = 15; pnSpeeds[1] = 15;
            break;
         case (1 << KEY_DOWN_ARROW):
            pnSpeeds[0] = -15; pnSpeeds[1] = -15;
            break;
         case (1 << KEY_RIGHT_ARROW):
            pnSpeeds[0] = 10; pnSpeeds[1] = -10;
            break;
         case (1 << KEY_LEFT_ARROW):
            pnSpeeds[0] = -10; pnSpeeds[1] = 10;
            break;
         case (1 << KEY_UP_ARROW) | (1 << KEY_RIGHT_ARROW):
            pnSpeeds[0] = 20; pnSpeeds[1] = 10;
            break;
         case (1 << KEY_UP_ARROW) | (1 << KEY_LEFT_ARROW):
            pnSpeeds[0] = 10; pnSpeeds[1] = 20;
            break;
         case (1 << KEY_DOWN_ARROW) | (1 << KEY_RIGHT_ARROW):
            pnSpeeds[0] = -10; pnSpeeds[1] = -20;
            break;
         case (1 << KEY_DOWN_ARROW) | (1 << KEY_LEFT_ARROW):
            pnSpeeds[0] = -20; pnSpeeds[1] = -10;
            break;
         default:
            pnSpeeds[0] = 0; pnSpeeds[1] = 0;
            break;
         }
         m_pcUplink->SendPacket(CPacketControlInterface::CPacket::EType::SET_DDS_SPEED,
                                reinterpret_cast<const uint8_t*>(pnSpeeds),
                                sizeof(pnSpeeds));
         unRobotCtrlLast = unRobotCtrl;
      }
   
      SDL_RenderClear(renderer);
      SDL_RenderPresent(renderer);
   }

   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(win);

   SDL_Quit();
   
   m_pcUplink->SendPacket(CPacketControlInterface::CPacket::EType::SET_DDS_ENABLE, false);  
   delete m_pcUplink;
   
   return 0;
}

