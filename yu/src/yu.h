#ifndef YU_H
#define YU_H


/*TODO:
  
renderer:
* render pass
* material interface
* state object
* metal

Sound:
* try fix wasapi


 */

namespace yu
{
enum YU_STATE
{
	YU_INIT = 0,
	YU_RUNNING = 1,
	YU_EXITING = 2,
	YU_STOPPED = 3,	
};
int YuState();
	
void SetYuExit();
int YuMain();
}

#endif
