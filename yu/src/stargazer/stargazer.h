
namespace yu
{
struct WorkItem;

extern struct StarGazer* gStarGazer;

void Clear(StarGazer* starGazer);
void SubmitWork(StarGazer* starGazer);

void InitStarGazer(WindowManager* winMgr, Allocator* allocator);
void FreeStarGazer(Allocator* allocator);

}
