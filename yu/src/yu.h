#ifndef YU_H
#define YU_H

namespace yu
{
enum YU_STATE
{
	YU_STOPPED = 0,
	YU_RUNNING = 1,
	YU_EXITING = 2,
};
int YuState();
	
void SetYuExit();
int YuMain();
}

#endif
